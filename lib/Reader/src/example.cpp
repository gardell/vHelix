/*
 * main.cpp
 *
 *  Created on: 9 maj 2012
 *      Author: johan
 */

#include <Helix.h>

#include <iostream>

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
	 */

	for(Helix::Scene::HelixList::iterator it = scene.begin_helices(); it != scene.end_helices(); ++it) {
		/*
		 * Notice that the list is of weak_ptr<Node> elements
		 */
		Helix::Helix & helix = static_cast<Helix::Helix &> (*it->lock());

		/*
		 * Note that rotation is in *degrees*
		 */

		std::cerr << "Helix: " << helix.getName() << ", translation: " << helix.getTranslation() << ", rotation: " << helix.getRotation() << std::endl;

		/*
		 * Iterate over the bases under the helix
		 */

		for(Helix::Helix::List::iterator b_it = helix.begin_children(); b_it != helix.end_children(); ++b_it) {
			Helix::Base & base = static_cast<Helix::Base &> (*b_it->lock());
			std::cerr << "\tBase: " << base.getName() << ", translation: " << base.getTranslation() << std::endl;

			/*
			 * To get the world coordinates for a base, including the helix rotation + translation, use:
			 */

			std::cerr << "\tWorld coordinates: " << std::endl << (base.getWorldTransform() * Helix::Vector()) << std::endl;
		}
	}


	/*
	 * A really unformatted dump of the scene
	 */

	std::cerr << scene << std::endl;

	return 0;
}
