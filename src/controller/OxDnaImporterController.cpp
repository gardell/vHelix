/*
 * OxDnaImporterController.cpp
 *
 *  Created on: Jan 29, 2014
 *      Author: johan
 */

#include <controller/OxDnaImporter.h>
#include <Utility.h>

#include <fstream>
#include <string>
#include <cstdio>

#if defined(WIN32) || defined(WIN64)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif /* N Windows */

#include <maya/MGlobal.h>
#include <maya/MQuaternion.h>

namespace Helix {
	namespace Controller {
		MStatus OxDnaImporter::read(const char *topology_filename, const char *configuration_filename, const char *vhelix_filename) {
			std::ifstream topology_file(topology_filename);
			std::ifstream configuration_file(configuration_filename);
			std::ifstream vhelix_file(configuration_filename);

			if (!topology_file) {
				MGlobal::displayError(MString("Unable to open file \"") + topology_filename + "\" for reading.");
				return MStatus::kFailure;
			}

			if (!configuration_file) {
				MGlobal::displayError(MString("Unable to open file \"") + configuration_filename + "\" for reading.");
				return MStatus::kFailure;
			}

			std::tr1::unordered_map<int, Base> bases;

			unsigned int numBases, numStrands;
			topology_file >> numBases >> numStrands;
			std::cerr << "numBases: " << numBases << ", " << numStrands << std::endl;

			int baseIndex = 0;
			while (topology_file.good()) {
				std::string line;
				std::getline(topology_file, line);

				if (line[0] == '#' || line.length() == 0)
					continue;

				Base base;
				char name;

				topology_file >> base.strand >> name >> base.forward >> base.backward;
				base.label = name;

				bases.insert(std::make_pair(baseIndex, base));
				++baseIndex;
			}

			topology_file.close();

			baseIndex = 0;
			while (configuration_file.good()) {
				std::string line;
				std::getline(configuration_file, line);

				if (line[0] == '#' || line.length() == 0)
					continue;

				Base & base = bases[baseIndex];
				MVector baseVector, normalVector; // Neither are used.

				configuration_file >> base.translation.x >> base.translation.y >> base.translation.z >>
						baseVector.x >> baseVector.y >> baseVector.z >>
						normalVector.x >> normalVector.y >> normalVector.z;

				++baseIndex;
			}

			std::tr1::unordered_map<std::string, Helix> helices;

			while (vhelix_file.good()) {
				std::string line;
				std::getline(configuration_file, line);

				if (line[0] == '#' || line.length() == 0)
					continue;

				int base;
				char name[1024], helixName[1024];
				Helix helix;

				if (sscanf(line.c_str(), "base %d %s %s", &base, name, helixName) == 2) {
					std::tr1::unordered_map<int, Base>::iterator it = bases.find(base);
					if (it == bases.end()) {
						HPRINT("Error when parsing file, base index %d out of bounds.", base);
						return MStatus::kFailure;
					}

					it->second.name = name;
					it->second.helixName = helixName;
				} else if (sscanf(line.c_str(), "helix %s %lf %lf %lf %lf %lf %lf", name, &helix.translation.x, &helix.translation.y, &helix.translation.z, &helix.normal.x, &helix.normal.y, &helix.normal.z) == 7) {
					helices.insert(std::make_pair(name, helix));
				}
			}

			MStatus status;

			// Create the helices...

			for (std::tr1::unordered_map<std::string, Helix>::iterator it = helices.begin(); it != helices.end(); ++it) {
				MTransformationMatrix transform;
				const MVector & normal(it->second.normal);
				transform.rotateTo(MQuaternion(normal.angle(MVector::zAxis), normal));

				HMEVALUATE_RETURN(status = Model::Helix::Create(it->first.c_str(), transform.asMatrix(), it->second.helix), status);
			}

			// Create the bases...
			for (std::tr1::unordered_map<int, Base>::iterator it = bases.begin(); it != bases.end(); ++it) {
				std::tr1::unordered_map<std::string, Helix>::iterator helix = helices.find(it->second.helixName.asChar());
				if (helix == helices.end()) {
					HPRINT("Error when parsing file, unknown helix \"%s\" out of bounds.", it->second.helixName.asChar());
					return MStatus::kSuccess;
				}

				HMEVALUATE_RETURN(status = Model::Base::Create(helix->second.helix, it->second.name, it->second.translation, it->second.base), status);
			}

			// Make forward connections...
			for (std::tr1::unordered_map<int, Base>::iterator it = bases.begin(); it != bases.end(); ++it) {
				std::tr1::unordered_map<int, Base>::iterator forward = bases.find(it->second.forward);
				if (forward == bases.end()) {
					HPRINT("Error when parsing file, base index %d out of bounds.", it->second.forward);
					return MStatus::kFailure;
				}

				HMEVALUATE_RETURN(status = it->second.base.connect_forward(forward->second.base, true), status);
			}

			return MStatus::kSuccess;
		}
	}
}


