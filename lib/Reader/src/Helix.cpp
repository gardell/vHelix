/*
 * Helix.cpp
 *
 *  Created on: 9 maj 2012
 *      Author: johan
 */

#include <Helix.h>

/* Visual Studio 2010+ includes the tr1 or C++0x regex functionality while for gcc or other compilers we need the boost library */
#ifdef _MSC_VER

#include <regex>
using std::regex;
using std::regex_search;
using std::regex_match;
using std::cmatch;
using std::sregex_iterator;
using std::match_results;

#else

#include <boost/regex.hpp>
using boost::regex;
using boost::regex_search;
using boost::regex_match;
using boost::cmatch;
using boost::sregex_iterator;
using boost::match_results;

#endif

#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>

namespace Helix {
	/*
	 * Recursive helper method for finding a node name in the tree
	 */
	bool Scene_getNodeByName_Recursive(weak_ptr<Node> & weak_target, const char *name, shared_ptr<Node> & result) {
		shared_ptr<Node> target = weak_target.lock();

		if (strcmp(target->getName(), name) == 0) {
			result = target;
			return true;
		}

		for(Node::List::iterator it = target->begin_children(); it != target->end_children(); ++it) {
			if (Scene_getNodeByName_Recursive(*it, name, result))
				return true;
		}

		return false;
	}

	bool Scene::getNodeByName(const char *uri, shared_ptr<Node> & result) {
		/*
		 * It seems it is safe to make the assumption that two cases will occur
		 * 1. Local name, ex: 'node1' references to a node anywhere in any tree and its name is unique
		 * 2. Full path name, ex: '|group1|vhelix1|node1', always starts with a | refering the scene root
		 * The other case of a 'vhelix1|node1' does not seem to occur
		 */

		if (!uri)
			return false;

		if (uri[0] == '|') {
			/*
			 * Full path uri, example: '|group1|vHelix1|base1'
			 */

			const char *last_bar = uri, *bar;
			shared_ptr<Node> target(Root);

			while(true) {
				if (last_bar == NULL)
					break;

				bar = strstr(last_bar + 1, "|");

				unsigned int name_length;
				name_length = last_bar != NULL ? (bar != NULL ? bar - 1 - last_bar : strlen(last_bar + 1)) : strlen(bar + 1);

				bool found = false;

				for(Node::List::iterator it = target->begin_children(); it != target->end_children(); ++it) {
					shared_ptr<Node> node = it->lock();

					if (strncmp(node->getName(), last_bar + 1, name_length) == 0) {
						target = node;
						found = true;
						break;
					}
				}

				if (!found)
					return false;

				last_bar = bar;
			}

			result = target;
			return true;
		}
		else {
			/*
			 * Local node name, just iterate over all objects recursively until we find a node with this name.
			 * Example: 'base1'
			 */

			weak_ptr<Node> root(Root);
			return Scene_getNodeByName_Recursive(root, uri, result);
		}
	}

	void Scene::parse(const char *filename) {
		std::ifstream stream(filename);

		/*
		 * Setup the regex objects
		 */

		// FIXME: Do we have to take into consideration if the helix is parented under something else?
		// In that we need to recursively figure out its path and then generate a full unique path name
		// that we can use when matching bases to helices?

		const static regex
					/*
					 * Match all setAttr (.t|.translate|.r|.rotate) -type "<type>" <x> <y> <z>
					 */
					Regex_setAttr_t_r("setAttr[[:blank:]]+\"\\.(t|(?:translate)|r|(?:rotate))\".+-type[[:blank:]]+\"([\\w_|]+)\"(?:[[:blank:]]+([\\-\\d\\.]+))(?:[[:blank:]]+([\\-\\d\\.]+))(?:[[:blank:]]+([\\-\\d\\.]+))"), // Incomplete, does not allow other arguments than type and data
					/*
					 * Match all setAttr .lb <label>
					 */
					Regex_setAttr_lb("setAttr[[:blank:]]+\"\\.(?:lb|(?:label))\".+-type[[:blank:]]+\"[\\w_|]+\"(?:[[:blank:]]+([\\d]))"),
					/*
					 * Match all connectAttr "<base 1 name>.(.fw|.forward|.bw|.backward)" "<base 1 name>.(.fw|.forward|.bw|.backward)"
					 */
					Regex_connectAttr("connectAttr[[:blank:]]+\"([\\w_|]+)\\.((?:fw)|(?:bw)|(?:forward)|(?:backward)|(?:lb)|(?:label))\"[[:blank:]]+\"([\\w_|]+)\\.((?:fw)|(?:bw)|(?:forward)|(?:backward)|(?:lb)|(?:label))\""),

					/*
					 * The boost regex does not support repeated groups, so we can only parse the node type here..
					 */
					Regex_createNode("createNode[[:blank:]]+([\\w_]+)[[:blank:]]+"),
					/*
					 * And we must thus have an extra regex that we iterate over for multiple arguments
					 */
					Regex_createNode_argument("-(\\w+)(?:[[:blank:]]+\"([\\w\\|_]+)\")?[[:blank:]]*");
		/*
		 * The current_node will point to the last added node using the 'createNode' command
		 * it is the target to all 'setAttr' commands
		 */

		shared_ptr<Node> current_node;

		while (stream.good()) {
			std::string line;
			cmatch match;

			getline(stream, line, ';');

			/*
			 * Try all the regular expressions defined above
			 */

			if (regex_search(line.c_str(), match, Regex_connectAttr)) {
				// Maya promises all objects have already been created, thus we can assume they all exist

				/*
				 * Look up the backward and forward nodes
				 */

				std::string source = match[1].str(), destination = match[3].str();
				shared_ptr<Node> source_node, destination_node;

				if (!getNodeByName(source.c_str(), source_node)) {
					std::stringstream stream;
					stream << "Couldn't find source node: " << source;
					throw parse_exception(stream.str());
				}

				if (!getNodeByName(destination.c_str(), destination_node)) {
					std::stringstream stream;
					stream << "Couldn't find destination node: " << destination;
					throw parse_exception(stream.str());
				}

				/*
				 * Figure out what type of connection we are doing
				 */

				if (match[2].str() == "bw" && match[4].str() == "fw") {
					/*
					 * Strand connection
					 */

					Base & source_base = static_cast<Base &> (*source_node), & destination_base = static_cast<Base &> (*destination_node);

					source_base.setForwardConnectedBase(destination_node);
					destination_base.setBackwardConnectedBase(source_node);
				}
				else if (match[2].str() == "lb" && match[4].str() == "lb") {
					/*
					 * Opposite base connection
					 */

					Base & source_base = static_cast<Base &> (*source_node), & destination_base = static_cast<Base &> (*destination_node);

					source_base.setOppositeConnectedBase(destination_node, false);
					destination_base.setOppositeConnectedBase(source_node, true);
				}
			}
			else if (regex_search(line.c_str(), match, Regex_setAttr_t_r)) {
				/*
				 * Parsing either a setAttr for translation or for rotation
				 */

				std::string x = match[3].str(), y = match[4].str(), z = match[5].str();

				Vector vector(atof(x.c_str()), atof(y.c_str()), atof(z.c_str()));

				if (current_node.get() != NULL) {
					if (match[1].str() == "t" || match[1].str() == "translate")
						current_node->setTranslation(vector);
					else
						current_node->setRotation(vector);
				}
				else
					throw parse_exception("Error, there is no node available for transformation");
			}
			else if (regex_search(line.c_str(), match, Regex_setAttr_lb)) {
				/*
				 * Setting the label value, this is the base type, (A,T,G,C or Invalid)
				 */

				std::string label = match[1].str();

				if (current_node && current_node->getType() == Node::BASE)
					static_cast<Base *>(current_node.get())->setLabel(atoi(label.c_str()));
				else
					throw parse_exception("Error, setAttr .lb on an element that is not a Base");
			}
			else if (regex_search(line.c_str(), match, Regex_createNode)) {
				/*
				 * Adding a new node to the scene, either vHelix, HelixBase or another transform node
				 */

				// Could save some memory/CPU by not copying these
				std::string name, parent, type(match[1].first, match[1].second), arguments(match[1].second);

				/*
				 * Iterate over the createNode arguments
				 */

				sregex_iterator it(arguments.begin(), arguments.end(), Regex_createNode_argument), end;

				for(; it != end; ++it) {
					if ((*it)[1].str() == "p" || (*it)[1].str() == "parent")
						parent = (*it)[2].str();
					else if ((*it)[1].str() == "n" || (*it)[1].str() == "name")
						name = (*it)[2].str();
				}

				if (match[1].str() == "vHelix") {
					/*
					 * Parsing new vHelix structure
					 */

					shared_ptr<Helix> helix(new Helix(name.c_str()));

					append_helix(helix);
					current_node = helix;
				}
				else if (match[1].str() == "HelixBase") {
					/*
					 * Parsing new HelixBase structure
					 */

					shared_ptr<Base> base(new Base(name.c_str()));

					current_node = base;
					append_node(current_node);
				}
				else {
					/*
					 * Unknown node type, but we still register it,
					 * it could be a transform node that will contain helices
					 * Also, further setAttr will be applied to this node and not the last added helix/base which would be wrong
					 */

					current_node = shared_ptr<Node>(new Node(name.c_str()));
					append_node(current_node);
				}

				if (parent.length() > 0) {
					shared_ptr<Node> parent_node;

					if (getNodeByName(parent.c_str(), parent_node)) {
						parent_node->addChild(current_node);
						current_node->addParent(parent_node);
					}
					else {
						throw parse_exception("Couldn't find parent");
					}
				}
				else {
					/*
					 * Has no parent, make it owned by the Root element
					 */

					Root->addChild(current_node);
					current_node->addParent(Root);
				}
			}
		}
	}

	bool Strand::contains_base(Base & base) const {
		/*
		 * Using the base defining the strand, iterate over all of the forward and backward references and try to find the given 'base'
		 */

		/* Forward */

		shared_ptr<Node> thisBase = m_base.lock();
		Base b = static_cast<Base &> (*thisBase);

		if (b == base)
			return true;

		bool found = true;

		do {
			if (!b.hasForwardConnectedBase()) {
				found = false;
				break;
			}

			b = b.getForwardConnectedBase();
		} while (b != base);

		if (found) {
			//std::cerr << "Forward loop, found: " << b.getName() << std::endl;
			//std::cerr << base.getName() << " = " << b.getName() << std::endl;
			return true;
		}

		/* Backward */

		b = static_cast<Base &> (*thisBase);
		found = true;

		do {
			if (!b.hasBackwardConnectedBase()) {
				found = false;
				break;
			}

			b = b.getBackwardConnectedBase();
		} while (b != base);

		if (found) {
			//std::cerr << "Backward loop, found: " << b.getName() << std::endl;
			//std::cerr << base.getName() << " = " << b.getName() << std::endl;
			return true;
		}

		//std::cerr << "Couldn't find base: " << base.getName() << std::endl;

		return false;
	}

	void Scene::generate_strands() {
		/*
		 * Iterate over all bases in the Scene and find out whether their strand is already in the m_strands list
		 */

		unsigned int last_id = 0;

		for(HelixList::iterator it = begin_helices(); it != end_helices(); ++it) {
			shared_ptr<Node> node = it->lock();
			Helix & helix = static_cast<Helix &> (*node.get());

			for(Helix::List::iterator b_it = helix.begin_children(); b_it != helix.end_children(); ++b_it) {
				shared_ptr<Node> b_node = b_it->lock();

				if (b_node->getType() != Node::BASE)
					continue;

				Base & base = static_cast<Base &> (*b_node.get());

				bool found = false;

				for(StrandList::iterator s_it = m_strands.begin(); s_it != m_strands.end(); ++s_it) {
					if (s_it->get()->contains_base(base)) {
						base.setStrand(*s_it);
						found = true;
						break;
					}
				}

				if (!found) {
					std::stringstream sstream;
					sstream << "strand_" << ++last_id;
					std::string id = sstream.str();

					shared_ptr<Strand> strand = std::make_shared<Strand> (id.c_str(), b_node);
					base.setStrand(strand);
					m_strands.push_back(strand);
				}
			}
		}
	}
}

/*
 * All for debugging
 */

std::string labelToString(int label) {
	switch(label) {
	case Helix::Base::A:
		return "A";
	case Helix::Base::T:
		return "T";
	case Helix::Base::G:
		return "G";
	case Helix::Base::C:
		return "C";
	default:
		return "Invalid";
	}
}

std::ostream & operator<< (std::ostream & stream, const Helix::Node & node) {
	static const char *str_types[] = { "node", "helix", "base" };

	stream << str_types[node.getType()] << " name: \"" << node.getName() << "\", translation: " << node.getTranslation() << " rotation: " << node.getRotation();

	switch(node.getType()) {
	case Helix::Node::BASE:
		// Dump label and forward/backward connections if available
		{
			const Helix::Base & base = static_cast<const Helix::Base &> (node);

			stream << " label: " << base.getLabel();
			
			{
				shared_ptr<Helix::Strand> strand = base.getStrand().lock();
				if (strand)
					stream << " strand: " << strand->getName();
			}

			if (base.hasForwardConnectedBase())
				stream << " forward connected base: " << base.getForwardConnectedBase().getName();
			else
				stream << " has no forward connected base";

			if (base.hasBackwardConnectedBase())
				stream << " backward connected base: " << base.getBackwardConnectedBase().getName();
			else
				stream << " has no backward connected base";

			if (base.hasOppositeConnectedBase())
				stream << " opposite connected base: " << base.getOppositeConnectedBase().getName();
			else
				stream << " has no opposite connected base";
		}
		break;
	default:
		// Dump child nodes

		stream << " children: ";
		for(Helix::Node::List::const_iterator it = node.begin_children(); it != node.end_children(); ++it)
			stream << *it->lock() << std::endl;
		stream << " end list of children for " << node.getName() << std::endl;
	}

	return stream;
}


std::ostream & operator<< (std::ostream & stream, const weak_ptr<Helix::Node> & node) {
	shared_ptr<Helix::Node> locked_node = node.lock();

	if (locked_node)
		stream << *locked_node;
	else
		stream << "(null)";

	return stream;
}

std::ostream & operator<< (std::ostream & stream, const Helix::Scene & scene) {
	for(Helix::Scene::NodeList::const_iterator it = scene.begin_nodes(); it != scene.end_nodes(); ++it)
		stream << **it << std::endl;

	return stream;
}
