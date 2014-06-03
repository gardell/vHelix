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
#include <maya/MProgressWindow.h>

#define BUFFER_SIZE 1024

namespace Helix {
	namespace Controller {
		struct non_nicked_strand_t {
			Model::Strand strand;
			// Bases together with their calculated offset along the strand.
			std::vector< std::pair<Model::Base, int> > bases;

			inline non_nicked_strand_t(Model::Base & base) : strand(base), bases(1, std::make_pair(base, 0)) {}
			inline void add_base(Model::Base & base) {
				bases.push_back(std::make_pair(base, 0));
			}
		};

		struct base_offset_comparator_t : public std::binary_function<std::pair<Model::Base, int>, std::pair<Model::Base, int>, bool> {
			const int offset;

			inline base_offset_comparator_t(int offset) : offset(offset) {}

			inline bool operator() (const std::pair<Model::Base, int> & p1, const std::pair<Model::Base, int> & p2) const {
				return std::abs(p1.second - offset) < std::abs(p2.second - offset);
			}
		};

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

		MStatus TextBasedImporter::read(const char *filename, int nicking_min_length, int nicking_max_length) {
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

			std::vector<Model::Base> nonNickedBases;

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
				else if (line == "autostaple" || line == "autonick")
					autostaple = true;
			}

			// Now create the helices, bases and make the connections.
			Creator creator;
#if defined(WIN32) || defined(WIN64)
			typedef std::unordered_map<std::string, Model::Helix> string_helix_map_t;
#else
			typedef std::tr1::unordered_map<std::string, Model::Helix> string_helix_map_t;
#endif /* N Windows */

			if (!MProgressWindow::reserve())
				MGlobal::displayWarning("Failed to reserve the progress window");

			MProgressWindow::setTitle("Import routed polygon");
			MProgressWindow::setProgressStatus("Creating new helices...");
			MProgressWindow::setProgressRange(0, int(helices.size()));
			MProgressWindow::startProgress();

			string_helix_map_t helixStructures;
			for (std::vector<Helix>::iterator it(helices.begin()); it != helices.end(); ++it) {
				Model::Helix helix;
				MTransformationMatrix transform;
				transform.setTranslation(it->position, MSpace::kTransform);
				transform.rotateTo(it->orientation);
				HMEVALUATE_RETURN(status = creator.create(it->bases, transform, helix, it->name.c_str(), false), status);
				helixStructures.insert(std::make_pair(it->name, helix));

				if (autostaple && it->bases > 1) {
					Model::Base base;
					HMEVALUATE_RETURN(status = helix.getBackwardFivePrime(base), status);
					
					// Disconnect base at backward 5' + floor(<num bases> / 2)

					for (unsigned int i = 0; i < (it->bases - 1) / 2 + 1; ++i)
						base = base.forward();

					if (it->bases > (unsigned int) nicking_min_length) {
						disconnectBackwardBases.push_back(base);
						paintStrandBases.push_back(base);
					}
					else {
						// Add this edge to non-nicked strands but only add the strand once.

						//HPRINT("Not nicking %s because only %u bases", base.getDagPath(status).fullPathName().asChar(), it->bases);
						nonNickedBases.push_back(base);
					}
				}

				MProgressWindow::advanceProgress(1);
			}

			MProgressWindow::endProgress();

			Controller::PaintMultipleStrandsWithNewColorFunctor functor;
			HMEVALUATE_RETURN(status = functor.loadMaterials(), status);

			// Create explicit bases
#if defined(WIN32) || defined(WIN64)
			typedef std::unordered_map<std::string, Model::Base> string_base_map_t;
#else
			typedef std::tr1::unordered_map<std::string, Model::Base> string_base_map_t;
#endif /* N Windows */

			string_base_map_t baseStructures;

			if (!explicitBases.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Creating explicit bases...");
				MProgressWindow::setProgressRange(0, int(explicitBases.size()));
				MProgressWindow::startProgress();

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
					HMEVALUATE(status = base.setMaterial(material), status);
					baseStructures.insert(std::make_pair(it->name, base));

					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			if (!explicitBaseLabels.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Setting explicit labels...");
				MProgressWindow::setProgressRange(0, int(explicitBaseLabels.size()));
				MProgressWindow::startProgress();

				// Set explicit labels
				for (explicit_base_labels_t::iterator it(explicitBaseLabels.begin()); it != explicitBaseLabels.end(); ++it) {
					if (baseStructures.find(it->first) == baseStructures.end()) {
						HPRINT("Unable to find Base structure \"%s\"", it->first.c_str());
						return MStatus::kFailure;
					}

					Model::Base & base(baseStructures[it->first]);
					HMEVALUATE_RETURN(status = base.setLabel(it->second), status);

					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			if (!paintStrands.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Setting up painting for explicit strands...");
				MProgressWindow::setProgressRange(0, int(paintStrands.size()));
				MProgressWindow::startProgress();

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
					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}


			if (!connections.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Making explicit base connections...");
				MProgressWindow::setProgressRange(0, int(connections.size()));
				MProgressWindow::startProgress();

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

					Model::Helix & fromHelix(helixStructures[it->fromHelixName]), &toHelix(helixStructures[it->toHelixName]);

					Model::Base fromBase, toBase;

					if (it->fromType == Connection::kNamed) {
						string_base_map_t::iterator baseIt(baseStructures.find(it->fromName));

						if (baseIt == baseStructures.end()) {
							HPRINT("failed to find base \"%s\"", it->fromName.c_str());
							return MStatus::kFailure;
						}

						fromBase = baseIt->second;
					}
					else
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

					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			if (!disconnectBackwardBases.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Nicking staples, this might take a while...");
				MProgressWindow::setProgressRange(0, int(disconnectBackwardBases.size()));
				MProgressWindow::startProgress();

				for (std::vector<Model::Base>::iterator it(disconnectBackwardBases.begin()); it != disconnectBackwardBases.end(); ++it) {
					HMEVALUATE_RETURN(status = it->disconnect_backward(), status);
					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			if (!paintStrandBases.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Painting strands, this might take a while...");
				MProgressWindow::setProgressRange(0, int(paintStrandBases.size()));
				MProgressWindow::startProgress();

				for (std::vector<Model::Base>::iterator it(paintStrandBases.begin()); it != paintStrandBases.end(); it++) {
					functor(*it);
					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			/*
			 * Group bases on the same strands.
			 */
			std::vector<non_nicked_strand_t> nonNickedStrands;

			if (!nonNickedBases.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Grouping non-nicked strands...");
				MProgressWindow::setProgressRange(0, int(nonNickedBases.size()));
				MProgressWindow::startProgress();

				for (std::vector<Model::Base>::iterator base_it(nonNickedBases.begin()); base_it != nonNickedBases.end(); ++base_it) {
					Model::Strand strand(*base_it);
					bool found = false;
					for (std::vector<non_nicked_strand_t>::iterator it(nonNickedStrands.begin()); it != nonNickedStrands.end(); ++it) {
						bool contains_base;
						HMEVALUATE_RETURN(contains_base = it->strand.contains_base(*base_it, status), status);
						if (contains_base) {
							//HPRINT("Paired with strand of %s", it->strand.getDefiningBase().getDagPath(status).fullPathName().asChar());
							it->add_base(*base_it);
							found = true;
						}
					}

					if (!found)
						nonNickedStrands.push_back(non_nicked_strand_t(*base_it));

					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			if (!nonNickedStrands.empty()) {
				if (!MProgressWindow::reserve())
					MGlobal::displayWarning("Failed to reserve the progress window");

				MProgressWindow::setTitle("Import routed polygon");
				MProgressWindow::setProgressStatus("Nicking previously non-nicked strands...");
				MProgressWindow::setProgressRange(0, int(nonNickedStrands.size()));
				MProgressWindow::startProgress();

				for (std::vector<non_nicked_strand_t>::iterator it(nonNickedStrands.begin()); it != nonNickedStrands.end(); ++it) {
					it->strand.rewind();
					unsigned int length(0);
					for (Model::Strand::ForwardIterator fit(it->strand.forward_begin()); fit != it->strand.forward_end(); ++fit, ++length) {
						for (std::vector< std::pair<Model::Base, int> >::iterator bit(it->bases.begin()); bit != it->bases.end(); ++bit) {
							if (bit->first == *fit)
								bit->second = length;
						}
					}

					const int num_nicks(int(std::ceil(double(length) / nicking_max_length)) - 1);
					HPRINT("Strand %s with length %u, will be nicked %u times.", it->strand.getDefiningBase().getDagPath(status).fullPathName().asChar(), length, num_nicks);
					for (int i = 0; i < num_nicks; ++i) {
						const unsigned int offset(i * nicking_max_length);

						std::vector< std::pair<Model::Base, int> >::iterator base_min_iterator(std::min_element(it->bases.begin(), it->bases.end(), base_offset_comparator_t(offset)));
						{
							Model::Base & base(base_min_iterator->first);
							for (std::vector< std::pair<Model::Base, int> >::iterator t_it(it->bases.begin()); t_it != it->bases.end(); ++t_it) {
								std::cerr << "base: " << t_it->first.getDagPath(status).fullPathName().asChar() << " offset: " << t_it->second << std::endl;
							}
							HPRINT("Disconnecting %s at offset: %u", base.getDagPath(status).fullPathName().asChar(), offset);
							base.disconnect_backward();
						}

						// Uncomment this to pick nicking sites with replacement.
						it->bases.erase(base_min_iterator);
					}

					MProgressWindow::advanceProgress(1);
				}

				MProgressWindow::endProgress();
			}

			return MStatus::kSuccess;
		}
	}
}
