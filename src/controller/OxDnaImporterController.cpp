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
#include <map>

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
			std::ifstream vhelix_file(vhelix_filename);

			if (!topology_file) {
				MGlobal::displayError(MString("Unable to open file \"") + topology_filename + "\" for reading.");
				return MStatus::kFailure;
			}

			if (!configuration_file) {
				MGlobal::displayError(MString("Unable to open file \"") + configuration_filename + "\" for reading.");
				return MStatus::kFailure;
			}

			HPRINT("1");
			std::tr1::unordered_map<int, Base> bases;

			unsigned int numBases, numStrands;
			topology_file >> numBases >> numStrands;
			onProcessStart(numBases * 5);

			int baseIndex = 0;
			std::map<unsigned int, int> strandBaseIndexOffset; // Stores the global base index each strand starts at.
			unsigned int previousStrandIndex = 0;

			while (topology_file.good()) {
				std::string line;
				std::getline(topology_file, line);

				if (line[0] == '#' || line.length() == 0)
					continue;

				Base base;
				char name;

				if (sscanf(line.c_str(), "%u %c %d %d", &base.strand, &name, &base.forward, &base.backward) == 4) {
					if (previousStrandIndex != base.strand) {
						previousStrandIndex = base.strand;
						strandBaseIndexOffset.insert(std::make_pair(previousStrandIndex, baseIndex));
					}

					base.label = name;

					std::cerr << "Inserting baseIndex: " << baseIndex << std::endl;
					bases.insert(std::make_pair(baseIndex, base));
					++baseIndex;

					onProcessStep();
				}
			}

			HPRINT("2");
			topology_file.close();

			// TODO: Parse bounding box

			baseIndex = 0;
			while (configuration_file.good()) {
				std::string line;
				std::getline(configuration_file, line);

				if (line[0] == '#' || line.length() == 0)
					continue;

				MVector baseTranslation, baseVector, normalVector, velocity, angularVelocity; // Only translation is used.
				// TODO: Use vectors for direction? How would that work with constraints?

				if (sscanf(line.c_str(), "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
						&baseTranslation.x, &baseTranslation.y, &baseTranslation.z,
						&baseVector.x, &baseVector.y, &baseVector.z,
						&normalVector.x, &normalVector.y, &normalVector.z,
						&velocity.x, &velocity.y, &velocity.z,
						&angularVelocity.x, &angularVelocity.y, &angularVelocity.z) == 15) {
					std::tr1::unordered_map<int, Base>::iterator it = bases.find(baseIndex);
					if (it == bases.end()) {
						HPRINT("Error when parsing file, base index %d out of bounds.", baseIndex);
						return MStatus::kFailure;
					}
					it->second.translation = baseTranslation;

					++baseIndex;
				}

				onProcessStep();
			}

			std::tr1::unordered_map<std::string, Helix> helices;

			HPRINT("3");
			while (vhelix_file.good()) {
				std::string line;
				std::getline(vhelix_file, line);

				if (line[0] == '#' || line.length() == 0)
					continue;

				int base;
				char name[1024], helixName[1024], material[1024];
				Helix helix;

				if (sscanf(line.c_str(), "base %d %s %s %s", &base, name, helixName, material) == 4) {
					std::tr1::unordered_map<int, Base>::iterator it = bases.find(base);
					if (it == bases.end()) {
						HPRINT("Error when parsing file, base index %d out of bounds.", base);
						return MStatus::kFailure;
					}

					it->second.name = name;
					it->second.helixName = helixName;
					it->second.material = material;

					HPRINT("Named base at index: %d %s, helix: %s", base, name, helixName);
					onProcessStep();
				} else if (sscanf(line.c_str(), "helix %s %lf %lf %lf %lf %lf %lf", name, &helix.translation.x, &helix.translation.y, &helix.translation.z, &helix.normal.x, &helix.normal.y, &helix.normal.z) == 7) {
					helices.insert(std::make_pair(name, helix));
				} else {
					HPRINT("Unknown line \"%s\"", line.c_str());
				}
			}

			MStatus status;

			for (std::tr1::unordered_map<std::string, Helix>::iterator it = helices.begin(); it != helices.end(); ++it) {
				MTransformationMatrix transform;
				const MVector & normal(it->second.normal);
				transform.rotateTo(MQuaternion(normal.angle(MVector::zAxis), MVector::zAxis ^ normal));
				HMEVALUATE_RETURN(status = transform.setTranslation(it->second.translation, MSpace::kWorld), status);

				HMEVALUATE_RETURN(status = Model::Helix::Create(it->first.c_str(), transform, it->second.helix), status);
			}

			// Create the bases...
			for (std::tr1::unordered_map<int, Base>::iterator it = bases.begin(); it != bases.end(); ++it) {
				std::tr1::unordered_map<std::string, Helix>::iterator helix = helices.find(it->second.helixName.asChar());
				if (helix == helices.end()) {
					HPRINT("Error when parsing file, unknown helix \"%s\" out of bounds for base \"%s\" at index %d.", it->second.helixName.asChar(), it->second.name.asChar(), it->first);
					return MStatus::kSuccess;
				}

				HMEVALUATE_RETURN(status = Model::Base::Create(helix->second.helix, it->second.name, it->second.translation, it->second.base, MSpace::kWorld), status);

				Model::Material material;
				if (!(status = Model::Material::Find(it->second.material, material))) {
					if (status != MStatus::kNotFound) {
						HMEVALUATE_RETURN_DESCRIPTION("Failed to obtain the material", status);
					} else {
						HPRINT("Warning: Can't find material \"%s\" for base \"%s\"", it->second.material.asChar(), it->second.base.getDagPath(status).fullPathName().asChar());
						HMEVALUATE_RETURN(material = *Model::Material::AllMaterials_begin(status), status);
					}
				}
				it->second.base.setMaterial(material);

				HMEVALUATE_RETURN(status = it->second.base.setLabel(it->second.label), status);

				onProcessStep();
			}

			// Make forward connections...
			for (std::tr1::unordered_map<int, Base>::iterator it = bases.begin(); it != bases.end(); ++it) {
				if (it->second.forward == -1)
					continue;

				std::tr1::unordered_map<int, Base>::iterator forward = bases.find(it->second.forward);
				if (forward == bases.end()) {
					HPRINT("Error when parsing file, base index %d out of bounds.", it->second.forward);
					return MStatus::kFailure;
				}

				HPRINT("From base %s forward to %s (indices %d and %d)", it->second.name.asChar(), forward->second.name.asChar(), it->first, forward->first);

				HMEVALUATE_RETURN(status = it->second.base.connect_forward(forward->second.base, true), status);

				onProcessStep();
			}

			return MStatus::kSuccess;
		}
	}
}


