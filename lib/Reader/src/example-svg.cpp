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
#include <limits>

/*
 * Colors used for each line
 */

const char *colors[] = { "blue", "blueviolet", "brown", "cadetblue", "darkred", "cyan", "gray", "green", "magenta", "orange", "purple", "red", "yellow", NULL };

/*
 * Used with std::ostream_iterator for writing polylines
 */

struct point {
public:
	point(const std::pair<double, double> & p_) : p(p_) { }
	std::pair<double, double> p;
};

std::ostream & operator<<(std::ostream & stream, const point & p) {
	stream << p.p.first << " " << p.p.second;

	return stream;
}

int main(int argc, const char **argv) {
	/*
	 * Parse a .ma file
	 */

	Helix::Scene scene;

	try {
		scene.parse(argv[1]);

		/*
		 * New procedure, generates strand information and ids
		 */

		scene.generate_strands();
	}
	catch(Helix::parse_exception & e) {
		std::cerr << "Parsing failed: \"" << e.what() << "\"" << std::endl;
		return 1;
	}

	/*
	 * Figure out the dimensions of the svg
	 * Note that we use a coordinate system as: (Z, -X)
	 * Because that's the one used in the screenshots from Maya
	 */

	double min_x = std::numeric_limits<double>::max(),
			max_x = std::numeric_limits<double>::min(),
			min_y = std::numeric_limits<double>::max(),
			max_y = std::numeric_limits<double>::min();

	for(Helix::Scene::HelixList::iterator it = scene.begin_helices(); it != scene.end_helices(); ++it) {
		Helix::Helix & helix = static_cast<Helix::Helix &> (*it->lock());

		for(Helix::Helix::List::iterator b_it = helix.begin_children(); b_it != helix.end_children(); ++b_it) {
			Helix::Node & node = *b_it->lock();
			if (node.getType() != Helix::Node::BASE)
				continue;

			Helix::Base & base = static_cast<Helix::Base &> (node);

			Helix::Vector worldCoordinates(base.getWorldTransform() * Helix::Vector());

			// Coordinates as (Z, -X)
			min_x = std::min(min_x, worldCoordinates.z);
			max_x = std::max(max_x, worldCoordinates.z);

			min_y = std::min(min_y, -worldCoordinates.x);
			max_y = std::max(max_y, -worldCoordinates.x);
		}
	}

	std::cout << "<svg width=\"" << round((max_x - min_x) * 100) << "\" height=\"" << round((max_y - min_y) * 100) << "\" viewBox=\"" << min_x << " " << min_y << " " << (max_x - min_x) << " " << (max_y - min_y) << "\">" << std::endl;

	/*
	 * Now iterate over all bases and pick the ones that have no backward connection
	 * then iterate over the strand defined by it and output a <polyline>
	 */
	int color_index = -1;

	for(Helix::Scene::HelixList::iterator it = scene.begin_helices(); it != scene.end_helices(); ++it) {
		Helix::Helix & helix = static_cast<Helix::Helix &> (*it->lock());

		for(Helix::Helix::List::iterator b_it = helix.begin_children(); b_it != helix.end_children(); ++b_it) {
			Helix::Node & node = *b_it->lock();
			if (node.getType() != Helix::Node::BASE)
				continue;

			Helix::Base & base = static_cast<Helix::Base &> (node);

			if (base.hasBackwardConnectedBase())
				continue;

			/*
			 * This base is an end base
			 */

			//std::cerr << "End base: " << base.getName() << std::endl;

			std::list<point> points;

			std::cout << "\t<!-- helix: " << helix.getName() << ", end base: " << base.getName() << " -->" << std::endl << "\t<polyline points=\"";

			Helix::Vector worldCoordinates(base.getWorldTransform() * Helix::Vector());
			//std::cerr << worldCoordinates.z << " " << (-worldCoordinates.x);
			points.push_back(std::make_pair(worldCoordinates.z, -worldCoordinates.x));

			if (base.hasForwardConnectedBase()) {
				Helix::Base *s_it(&base);
				do {
					s_it = &s_it->getForwardConnectedBase();
					Helix::Vector worldCoordinates(s_it->getWorldTransform() * Helix::Vector());
					points.push_back(std::make_pair(worldCoordinates.z, -worldCoordinates.x));
				} while (s_it->hasForwardConnectedBase());
			}

			std::copy(points.begin(), points.end(), std::ostream_iterator<point> (std::cout, ", "));

			color_index = colors[color_index + 1] ? color_index + 1 : 0;
			std::cout << "\" style=\"stroke: " << colors[color_index] << "; stroke-width: 0.03; fill: none;\"/>" << std::endl << std::endl;
		}
	}

	/*
	 * Iterating over available helices and bases
	 * and print out information about all the bases
	 */

	/*for(Helix::Scene::HelixList::iterator it = scene.begin_helices(); it != scene.end_helices(); ++it) {
		 *
		 * Notice that the list is of weak_ptr<Node> elements
		 *
		Helix::Helix & helix = static_cast<Helix::Helix &> (*it->lock());

		 *
		 * Note that rotation is in *degrees*
		 *
		std::cerr << "Helix: " << helix.getName() << ", translation: " << helix.getTranslation() << ", rotation: " << helix.getRotation() << std::endl << "\tBases: " << std::endl;

		 *
		 * Iterate over the bases under the helix
		 *
		for(Helix::Helix::List::iterator b_it = helix.begin_children(); b_it != helix.end_children(); ++b_it) {
			Helix::Node & node = *b_it->lock();
			if (node.getType() != Helix::Node::BASE)
				continue;

			Helix::Base & base = static_cast<Helix::Base &> (node);

			// Base_id  |  base's position in 3d coordinates | id of the strand and helix it belongs to |   connected to which Base (in which helix) |
			std::cerr << "\t" << base.getName();

			{
				shared_ptr<Helix::Strand> strand = base.getStrand().lock();

				if (strand)
					std::cerr << " | " << strand->getName();
			}

			std::cerr << " | " << (base.getWorldTransform() * Helix::Vector()) << " | " << helix.getName() << std::endl << "\t\tforward: ";

			if (base.hasForwardConnectedBase()) {
				Helix::Base & forward_base = base.getForwardConnectedBase();
				std::cerr << forward_base.getName() << " with parent: ";

				 *
				 * Prints a list of its parents, bases will always have only one parent, but as Maya technically supports multiple parents, i implemented it too, thus the iterator
				 *

				for(Helix::Base::List::iterator fwp_it = forward_base.begin_parents(); fwp_it != forward_base.end_parents(); ++fwp_it)
					std::cerr << fwp_it->lock()->getName() << " ";
			}
			else
				std::cerr << "<none>";

			std::cerr << std::endl << "\t\tbackward: ";

			if (base.hasBackwardConnectedBase()) {
				Helix::Base & backward_base = base.getBackwardConnectedBase();
				std::cerr << backward_base.getName() << " with parent: ";

				 *
				 * Prints a list of its parents, bases will always have only one parent, but as Maya technically supports multiple parents, i implemented it too, thus the iterator
				 *

				for(Helix::Base::List::iterator bwp_it = backward_base.begin_parents(); bwp_it != backward_base.end_parents(); ++bwp_it)
					std::cerr << bwp_it->lock()->getName() << " ";
			}
			else
				std::cerr << "<none>";

			std::cerr << std::endl << "\t\topposite: ";

			if (base.hasOppositeConnectedBase()) {
				Helix::Base & opposite_base = base.getOppositeConnectedBase();
				std::cerr << opposite_base.getName() << " with parent: ";

				 *
				 * Prints a list of its parents, bases will always have only one parent, but as Maya technically supports multiple parents, i implemented it too, thus the iterator
				 *

				for(Helix::Base::List::iterator op_it = opposite_base.begin_parents(); op_it != opposite_base.end_parents(); ++op_it)
					std::cerr << op_it->lock()->getName() << " ";
			}
			else
				std::cerr << "<none>";

			std::cerr << std::endl;
		}
	}*/


	/*
	 * A really unformatted dump of the scene
	 */

	//std::cerr << scene << std::endl;

	std::cout << "</svg>" << std::endl;

	return 0;
}
