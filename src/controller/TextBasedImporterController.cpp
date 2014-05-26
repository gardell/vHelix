#include <Definition.h>

#include <controller/Connect.h>
#include <controller/PaintStrand.h>
#include <controller/TextBasedImporter.h>
#include <model/Material.h>
#include <Creator.h>

#include <cstdio>
#include <fstream>
#include <string>

#include <maya/MQuaternion.h>

#define BUFFER_SIZE 1024

namespace Helix {
	namespace Controller {
		TextBasedImporter::Connection::Type TextBasedImporter::Connection::TypeFromString(const char *type) {
			if (strcmp("f5'", type) == 0)
				return kForwardFivePrime;
			else if (strcmp("f3'", type) == 0)
				return kForwardThreePrime;
			else if (strcmp("b5'", type) == 0)
				return kBackwardFivePrime;
			else if (strcmp("b3'", type) == 0)
				return kBackwardThreePrime;
			else
				return kNamed;
		}

		MStatus getBaseFromConnectionType(Model::Helix & helix, TextBasedImporter::Connection::Type type, Model::Base & base) {
			MStatus status;
			switch (type) {
			case TextBasedImporter::Connection::kBackwardFivePrime:
				HMEVALUATE_RETURN(status = helix.getBackwardFivePrime(base), status);
				break;
			case TextBasedImporter::Connection::kBackwardThreePrime:
				HMEVALUATE_RETURN(status = helix.getBackwardThreePrime(base), status);
				break;
			case TextBasedImporter::Connection::kForwardFivePrime:
				HMEVALUATE_RETURN(status = helix.getForwardFivePrime(base), status);
				break;
			case TextBasedImporter::Connection::kForwardThreePrime:
				HMEVALUATE_RETURN(status = helix.getForwardThreePrime(base), status);
				break;
			default:
				return MStatus::kUnknownParameter;
			}

			return MStatus::kSuccess;
		}

		MStatus TextBasedImporter::read(const char *filename) {
			MStatus status;
			std::ifstream file(filename);

			if (file.fail()) {
				HPRINT("Failed to open file \"%s\"", filename);
				return MStatus::kFailure;
			}

			MVector position;
			MQuaternion orientation;
			unsigned int bases;
			char nameBuffer[BUFFER_SIZE], helixNameBuffer[BUFFER_SIZE], materialNameBuffer[BUFFER_SIZE], targetNameBuffer[BUFFER_SIZE], targetHelixNameBuffer[BUFFER_SIZE];
			char label;
			bool autostaple(false);
			std::vector< std::pair<std::string, std::string> > paintStrands;
			std::vector<Model::Base> paintStrandBases, disconnectBackwardBases;

			while (file.good()) {
				std::string line;
				std::getline(file, line);

				if (sscanf(line.c_str(), "h %s %lf %lf %lf %lf %lf %lf %lf", nameBuffer, &position.x, &position.y, &position.z, &orientation.x, &orientation.y, &orientation.z, &orientation.w) == 8)
					helices.push_back(Helix(position, orientation, nameBuffer));
				else if (sscanf(line.c_str(), "hb %s %u %lf %lf %lf %lf %lf %lf %lf", nameBuffer, &bases, &position.x, &position.y, &position.z, &orientation.x, &orientation.y, &orientation.z, &orientation.w) == 9)
					helices.push_back(Helix(position, orientation, nameBuffer, bases));
				else if (sscanf(line.c_str(), "b %s %s %lf %lf %lf %s %c", nameBuffer, helixNameBuffer, &position.x, &position.y, &position.z, materialNameBuffer, &label) == 7) {
					std::vector<TextBasedImporter::Helix>::iterator helix_it(std::find(helices.begin(), helices.end(), helixNameBuffer));
					explicitBases.push_back(TextBasedImporter::Base(nameBuffer, position, materialNameBuffer, label));
				}
				else if (sscanf(line.c_str(), "c %s %s %s %s", helixNameBuffer, nameBuffer, targetHelixNameBuffer, targetNameBuffer) == 4)
					connections.push_back(Connection(helixNameBuffer, nameBuffer, targetHelixNameBuffer, targetNameBuffer, Connection::TypeFromString(nameBuffer), Connection::TypeFromString(targetNameBuffer)));
				else if (sscanf(line.c_str(), "l %s %c", nameBuffer, &label) == 2)
					explicitBaseLabels.insert(std::make_pair(nameBuffer, label));
				else if (sscanf(line.c_str(), "ps %s %s", helixNameBuffer, nameBuffer) == 2)
					paintStrands.push_back(std::make_pair(std::string(helixNameBuffer), std::string(nameBuffer)));
				else if (line == "autostaple")
					autostaple = true;
			}

			// Now create the helices, bases and make the connections.
			Creator creator;
#if defined(WIN32) || defined(WIN64)
			typedef std::unordered_map<std::string, Model::Helix> string_helix_map_t;
#else
			typedef std::tr1::unordered_map<std::string, Model::Helix> string_helix_map_t;
#endif /* N Windows */

			string_helix_map_t helixStructures;
			for (std::vector<Helix>::iterator it(helices.begin()); it != helices.end(); ++it) {
				Model::Helix helix;
				MTransformationMatrix transform;
				transform.setTranslation(it->position, MSpace::kTransform);
				transform.rotateTo(it->orientation);
				HMEVALUATE_RETURN(status = creator.create(it->bases, transform, helix, it->name.c_str()), status);
				helixStructures.insert(std::make_pair(it->name, helix));

				if (autostaple && it->bases > 1) {
					// Disconnect base at backward 5' + floor(<num bases> / 2)
					Model::Base base;
					HMEVALUATE_RETURN(status = helix.getBackwardFivePrime(base), status);
					for (unsigned int i = 0; i < (it->bases - 1) / 2 + 1; ++i)
						base = base.forward();
					disconnectBackwardBases.push_back(base);
					paintStrandBases.push_back(base);
				}
			}

			Controller::PaintMultipleStrandsWithNewColorFunctor functor;
			HMEVALUATE_RETURN(status = functor.loadMaterials(), status);

			// Create explicit bases
#if defined(WIN32) || defined(WIN64)
			typedef std::unordered_map<std::string, Model::Base> string_base_map_t;
#else
			typedef std::tr1::unordered_map<std::string, Model::Base> string_base_map_t;
#endif /* N Windows */

			string_base_map_t baseStructures;
			for (std::vector<Base>::iterator it(explicitBases.begin()); it != explicitBases.end(); ++it) {
				if (helixStructures.find(it->helixName) == helixStructures.end()) {
					HPRINT("Unable to find Helix structure \"%s\"", it->helixName.c_str());
					return MStatus::kFailure;
				}
				Model::Base base;
				Model::Helix & helix(helixStructures[it->helixName]);
				HMEVALUATE_RETURN(status = Model::Base::Create(helix, it->name.c_str(), it->position, base), status);
				base.setLabel(it->label);
				
				Model::Material material;
				HMEVALUATE_RETURN(status = Model::Material::Find(it->materialName.c_str(), material), status);
				HMEVALUATE_RETURN(status = base.setMaterial(material), status);
				baseStructures.insert(std::make_pair(it->name, base));
			}

			// Set explicit labels
			for (explicit_base_labels_t::iterator it(explicitBaseLabels.begin()); it != explicitBaseLabels.end(); ++it) {
				if (baseStructures.find(it->first) == baseStructures.end()) {
					HPRINT("Unable to find Base structure \"%s\"", it->first.c_str());
					return MStatus::kFailure;
				}

				Model::Base & base(baseStructures[it->first]);
				HMEVALUATE_RETURN(status = base.setLabel(it->second), status);
			}

			for (std::vector< std::pair<std::string, std::string> >::const_iterator it(paintStrands.begin()); it != paintStrands.end(); ++it) {
				string_helix_map_t::iterator helixIt(helixStructures.find(it->first));

				if (helixIt == helixStructures.end()) {
					HPRINT("Failed to find helix named \"%s\"", it->first.c_str());
					return MStatus::kFailure;
				}

				Model::Base base;
				const Connection::Type type(Connection::TypeFromString(it->second.c_str()));
				switch (type) {
				case Connection::kNamed:
				{
					string_base_map_t::iterator baseIt(baseStructures.find(it->second));

					if (baseIt == baseStructures.end()) {
						HPRINT("failed to find base \"%s\"", it->second.c_str());
						return MStatus::kFailure;
					}

					base = baseIt->second;
				}
					break;
				default:
					HMEVALUATE_RETURN(status = getBaseFromConnectionType(helixIt->second, type, base), status);
					break;
				}

				paintStrandBases.push_back(base);
			}

			// Create explicit connections
			Controller::Connect connect;
			for (std::vector<Connection>::iterator it(connections.begin()); it != connections.end(); ++it) {
				if (helixStructures.find(it->fromHelixName) == helixStructures.end()) {
					HPRINT("Failed to find helix named \"%s\"", it->fromHelixName.c_str());
					return MStatus::kFailure;
				}

				if (helixStructures.find(it->toHelixName) == helixStructures.end()) {
					HPRINT("Failed to find helix named \"%s\"", it->toHelixName.c_str());
					return MStatus::kFailure;
				}

				Model::Helix & fromHelix(helixStructures[it->fromHelixName]), & toHelix(helixStructures[it->toHelixName]);

				Model::Base fromBase, toBase;

				if (it->fromType == Connection::kNamed) {
					string_base_map_t::iterator baseIt(baseStructures.find(it->fromName));

					if (baseIt == baseStructures.end()) {
						HPRINT("failed to find base \"%s\"", it->fromName.c_str());
						return MStatus::kFailure;
					}

					fromBase = baseIt->second;
				} else
					HMEVALUATE_RETURN(status = getBaseFromConnectionType(fromHelix, it->fromType, fromBase), status);

				if (it->toType == Connection::kNamed) {
					string_base_map_t::iterator baseIt(baseStructures.find(it->toName));

					if (baseIt == baseStructures.end()) {
						HPRINT("failed to find base \"%s\"", it->toName.c_str());
						return MStatus::kFailure;
					}

					toBase = baseIt->second;
				}
				else
					HMEVALUATE_RETURN(status = getBaseFromConnectionType(toHelix, it->toType, toBase), status);

				HMEVALUATE_RETURN(status = connect.connect(fromBase, toBase), status);
			}

			for (std::vector<Model::Base>::iterator it(disconnectBackwardBases.begin()); it != disconnectBackwardBases.end(); ++it)
				HMEVALUATE_RETURN(status = it->disconnect_backward(), status);

			for (std::vector<Model::Base>::iterator it(paintStrandBases.begin()); it != paintStrandBases.end(); it++)
				HMEVALUATE(functor(*it), functor.status());

			return MStatus::kSuccess;
		}
	}
}
