/*
 * main.cpp
 *
 *  Created on: 9 maj 2012
 *      Author: johan
 */

#include <Helix.h>

#include <iostream>
#include <algorithm>
#include <iterator>

int main(int argc, const char **argv) {
	/*
	 * Parse a .ma file
	 */

	Helix::Scene scene;

	try {
		scene.parse(argv[1]);
	}
	catch(Helix::parse_exception & e) {
		std::cerr << "Parsing failed: \"" << e.what() << "\"" << std::endl;
		return 1;
	}

	/*
	 * Iterating over available helices and bases
	 * and print out information about all the bases
	 */

	for(Helix::Scene::HelixList::iterator it = scene.begin_helices(); it != scene.end_helices(); ++it) {
		/*
		 * Notice that the list is of weak_ptr<Node> elements
		 */
		Helix::Helix & helix = static_cast<Helix::Helix &> (*it->lock());

		/*
		 * Note that rotation is in *degrees*
		 */
		std::cerr << "Helix: " << helix.getName() << ", translation: " << helix.getTranslation() << ", rotation: " << helix.getRotation() << std::endl << "\tBases: " << std::endl;

		/*
		 * Iterate over the bases under the helix
		 */
		for(Helix::Helix::List::iterator b_it = helix.begin_children(); b_it != helix.end_children(); ++b_it) {
			Helix::Node & node = *b_it->lock();
			if (node.getType() != Helix::Node::BASE)
				continue;

			Helix::Base & base = static_cast<Helix::Base &> (node);

			// Base_id  |  base's position in 3d coordinates | id of the strand and helix it belongs to |   connected to which Base (in which helix) |
			std::cerr << "\t" << base.getName() << " | " << (base.getWorldTransform() * Helix::Vector()) << " | " << helix.getName() << std::endl << "\t\tforward: ";

			if (base.hasForwardConnectedBase()) {
				Helix::Base & forward_base = base.getForwardConnectedBase();
				std::cerr << forward_base.getName() << " with parent: ";

				/*
				 * Prints a list of its parents, bases will always have only one parent, but as Maya technically supports multiple parents, i implemented it too, thus the iterator
				 */

				for(Helix::Base::List::iterator fwp_it = forward_base.begin_parents(); fwp_it != forward_base.end_parents(); ++fwp_it)
					std::cerr << fwp_it->lock()->getName() << " ";
			}
			else
				std::cerr << "<none>";

			std::cerr << std::endl << "\t\tbackward: ";

			if (base.hasBackwardConnectedBase()) {
				Helix::Base & backward_base = base.getBackwardConnectedBase();
				std::cerr << backward_base.getName() << " with parent: ";

				/*
				 * Prints a list of its parents, bases will always have only one parent, but as Maya technically supports multiple parents, i implemented it too, thus the iterator
				 */

				for(Helix::Base::List::iterator bwp_it = backward_base.begin_parents(); bwp_it != backward_base.end_parents(); ++bwp_it)
					std::cerr << bwp_it->lock()->getName() << " ";
			}
			else
				std::cerr << "<none>";

			std::cerr << std::endl << "\t\topposite: ";

			if (base.hasOppositeConnectedBase()) {
				Helix::Base & opposite_base = base.getOppositeConnectedBase();
				std::cerr << opposite_base.getName() << " with parent: ";

				/*
				 * Prints a list of its parents, bases will always have only one parent, but as Maya technically supports multiple parents, i implemented it too, thus the iterator
				 */

				for(Helix::Base::List::iterator op_it = opposite_base.begin_parents(); op_it != opposite_base.end_parents(); ++op_it)
					std::cerr << op_it->lock()->getName() << " ";
			}
			else
				std::cerr << "<none>";

			std::cerr << std::endl;
		}
	}


	/*
	 * A really unformatted dump of the scene
	 */

	//std::cerr << scene << std::endl;

	return 0;
}
