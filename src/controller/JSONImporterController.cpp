#include <controller/JSONImporter.h>
#include <controller/PaintStrand.h>

#include <maya/MProgressWindow.h>

#include <fstream>
#include <algorithm>
#include <list>
#include <iterator>

namespace Helix {
	namespace Controller {
		/*
		 * Override the paint functor to increase the progress bar
		 */

		class PaintMultipleStrandsWithProgressBar : public Controller::PaintMultipleStrandsNoUndoNoOverrideFunctor {
		public:
			void operator() (Model::Strand strand, Model::Material & material) {
				Controller::PaintMultipleStrandsNoUndoFunctor::operator()(strand, material);
				MProgressWindow::advanceProgress(1);
			}
		};

		static const char *str_strands[] = { "scaf", "stap" };

		MStatus JSONImporter::parseFile(const char *filename) {
			MStatus status;

			std::fstream file(filename, std::ios_base::in);

			if (!file) {
				MGlobal::displayError(MString("Failed to open file \"") + filename + "\" for reading");
				return MStatus::kFailure;
			}

			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(file, root, false)) {
				MGlobal::displayError(MString("Failed to parse file \"") + filename);
				return MStatus::kFailure;
			}

			if (!root.isObject()) {
				MGlobal::displayError(MString("Failed to parse file \"") + filename);
				return MStatus::kFailure;
			}

			m_file.filename = filename;
			Json::Value vstrands = root ["vstrands"], name = root ["name"];

			if (name.isString())
				m_file.name = name.asCString();

			if (!vstrands.isArray()) {
				MGlobal::displayError(MString("Failed to parse file \"") + filename + "\" for reading");
				return MStatus::kFailure;
			}

			/*
			 * Calculate the average col and row for centering the helices. As well as a total_num for the progress bar
			 */

			int average_col = 0, average_row = 0, total_num_operations = 0, longest_strand = 0;

			for(Json::Value::iterator it = vstrands.begin(); it != vstrands.end(); ++it) {
				average_col += (*it) ["col"].asInt();
				average_row += (*it) ["row"].asInt();

				int size = (*it) ["loop"].size();
				total_num_operations += size;

				longest_strand = MAX(longest_strand, size);
			}

			average_col /= vstrands.size();
			average_row /= vstrands.size();

			/*
			 * These are bases building up strands that should be colored
			 */

			std::list< std::pair<Model::Base, Model::Material> > paintBases;

			/*
			 * Randomize the material that will be used for the scaffold
			 */

			Model::Material scaf_material;

			{
				size_t numMaterials;

				Model::Material::Iterator it = Model::Material::AllMaterials_begin(status, numMaterials);

				if (!status) {
					status.perror("Material::AllMaterials_begin");
					return status;
				}

				if (numMaterials == 0) {
					std::cerr << "Can't find any materials" << std::endl;
					return MStatus::kFailure;
				}

				scaf_material = *(it + (rand() % numMaterials));
			}

			MProgressWindow::endProgress();

			if (!MProgressWindow::reserve())
				MGlobal::displayWarning("Can't reserve progress window, no progress information will be presented");
			MProgressWindow::setProgressRange(0, total_num_operations);
			MProgressWindow::setTitle("Importing json (caDNAno) file...");
			MProgressWindow::setProgressStatus(MString("Parsing file: \"") + filename + "\"");
			MProgressWindow::setInterruptable(false);
			MProgressWindow::startProgress();

			/*
			 * Iterate over strands and generate the binary structures
			 * We also create the bases right here, without connecting them to each other yet
			 */

			for(Json::Value::iterator it = vstrands.begin(); it != vstrands.end(); ++it) {
				Json::Value & scaf = (*it) [str_strands[0]],
							& stap = (*it) [str_strands[1]],
							& loop = (*it) ["loop"],
							& skip = (*it) ["skip"],
							& num_value = (*it) ["num"],
							& col = (*it) ["col"],
							& row = (*it) ["row"],
							& stap_colors = (*it) ["stap_colors"];

				if (!scaf.isArray() || !stap.isArray() || !skip.isArray() || !loop.isArray() || !num_value.isNumeric() || !col.isNumeric() || !row.isNumeric() || scaf.size() != stap.size() || loop.size() != scaf.size() || skip.size() != loop.size()) {
					MGlobal::displayError(MString("Syntax error in file \"") + filename + "\"");
					return MStatus::kFailure;
				}

				Helix helix;

				helix.col = col.asInt();
				helix.row = row.asInt();
				helix.scaf.reserve(scaf.size());
				helix.stap.reserve(stap.size());
				helix.loop.reserve(loop.size());
				helix.skip.reserve(skip.size());

				/*
				 * We take care about the loop and skip arrays first, because they're needed below
				 */

				int total_strand_length = 0; // Unchanged for every skip and increased for every loop

				for(Json::Value::ArrayIndex i = 0; i < scaf.size(); ++i) {
					int loop_int = loop [i].asInt(), skip_int = skip [i].asInt();
					helix.loop.push_back(loop_int);
					helix.skip.push_back(skip_int);

					total_strand_length += 1 + loop_int - skip_int;
				}

				int num = num_value.asInt();

				/*
				 * Scaf_direction: If the direction is inversed, the helix will be rotated 180 degrees along the X-axis.
				 * this requires us to compensate on the Z coordinate of the bases
				 */

				int scaf_direction = num % 2; // 0 = left to right, 1 = right to left

				/* 
				 * For creating the honeycomb lattice
				 */

				double shuffle = ((helix.row % 2) * 2 - 1) * (((helix.col + 1) % 2) * 2 - 1);

				MVector honeycomb_translation(
					DNA::HONEYCOMB_X_STRIDE * (double(helix.col) - average_col),
					DNA::HONEYCOMB_Y_STRIDE * (double(helix.row) - average_row) + DNA::HONEYCOMB_Y_OFFSET * shuffle
				);

				double honeycomb_rotation_offset = M_PI + 2.0 * DEG2RAD(DNA::PITCH);

				/*
				 * Transformation of the helix. Note that we flip every odd helix by 180 degrees.
				 * to get the directional arrows in the correct order. Notice however, that caDNAno does not do this.
				 * so we will have to flip back the bases.
				 */

				MTransformationMatrix helix_transform;

				if (!(status = helix_transform.setTranslation(honeycomb_translation, MSpace::kTransform))) {
					status.perror("MFnTransform::setTranslation");
					return status;
				}

				double rotation[] = { M_PI * scaf_direction, 0.0, honeycomb_rotation_offset };
					
				if (!(status = helix_transform.setRotation(rotation, MTransformationMatrix::kXYZ, MSpace::kTransform))) {
					status.perror("MTransformationMatrix::setRotation");
					return status;
				}

				/*
				 * Create the helix
				 */

				MMatrix helix_transform_matrix = helix_transform.asMatrix();

				if (!(status = Model::Helix::Create("helix1", helix_transform_matrix, helix.helix))) {
					status.perror("Helix::Create");
					return status;
				}

				/*
				 * Data for cylinder generation
				 */

				int lowest_valid_base_index = INT_MAX, highest_valid_base_index = 0;

				/*
				 * Iterate over bases, collect their connection data and generate the bases without connecting them
				 */

				int translation_index = 0;

				for(Json::Value::ArrayIndex i = 0; i < scaf.size(); ++i) {
					Base scaf_base, stap_base;
					Json::Value & scaf_indices = scaf[i], & stap_indices = stap[i];
					int & loop_int = helix.loop[i], & skip_int = helix.skip[i];

					for(int j = 0; j < 4; ++j) {
						scaf_base.connections[j] = scaf_indices[j].asInt();
						stap_base.connections[j] = stap_indices[j].asInt();
					}

					if (!skip_int) {
						scaf_base.bases.reserve(loop_int + 1);
						stap_base.bases.reserve(loop_int + 1);

						for(int j = 0; j < loop_int + 1; ++j) {
							//int index = scaf_direction == 1 ? (- translation_index - 1) : translation_index;
							//int index = translation_index * (-scaf_direction + ((scaf_direction + 1) % 2)) - scaf_direction;
							int index = (scaf_direction * 2 - 1) * -translation_index - scaf_direction;

							MVector forward_vector, backward_vector;

							DNA::CalculateBasePairPositions((double) index, forward_vector, backward_vector, 0.0, scaf_direction == 0 ? longest_strand : -longest_strand);

							if (scaf_base.isValid()) {
								// Create the base for the loop index j at index

								Model::Base base;
								MString name = MString(str_strands[0]) + (int) i;
								
								if (j > 0)
									name += "_loop" + j;

								if (!(status = Model::Base::Create(helix.helix, name, forward_vector, base))) {
									status.perror("Base::Create 1");
									return status;
								}

								scaf_base.bases.push_back(base);
							}

							if (stap_base.isValid()) {
								// Create the base for the loop index j at index

								Model::Base base;
								MString name = MString(str_strands[1]) + "_" + (int) i;
								
								if (j > 0)
									name += "_loop_" + j;

								if (!(status = Model::Base::Create(helix.helix, name, backward_vector, base))) {
									status.perror("Base::Create 2");
									return status;
								}

								stap_base.bases.push_back(base);
							}

							if (scaf_base.isValid() && stap_base.isValid()) {
								/*
								 * Connect the labels
								 */

								if (!(status = scaf_base.bases[scaf_base.bases.size() - 1].connect_opposite(stap_base.bases[stap_base.bases.size() - 1], true))) {
									status.perror("Base::connect_opposite");
									return status;
								}
							}
							else {
								//std::cerr << "Single stranded at helix: " << num << ", base index: " << i << " loop: " << j << std::endl;
							}

							++translation_index;
						}

						/*
						 * If there was a loop (loop_int > 0), iterate over the loop and connect them to each other
						 */

						if (scaf_direction == 0) {
							/*
							 * Connect scaffold forward and staples backward
							 */

							for(int j = 0; j < loop_int; ++j) {
								if (scaf_base.isValid()) {
									if (!(status = scaf_base.bases[j].connect_forward(scaf_base.bases[j + 1], true))) {
										status.perror("Model::Base::connect_forward 1 1");
										return status;
									}
								}

								if (stap_base.isValid()) {
									if (!(status = stap_base.bases[j + 1].connect_forward(stap_base.bases[j], true))) {
										status.perror("Model::Base::connect_forward 1 2");
										return status;
									}
								}
							}
						}
						else {
							/*
							 * Connect scaffold backward and staples forward
							 */

							for(int j = 0; j < loop_int; ++j) {
								if (scaf_base.isValid()) {
									if (!(status = scaf_base.bases[j + 1].connect_forward(scaf_base.bases[j], true))) {
										status.perror("Model::Base::connect_forward 2 1");
										return status;
									}
								}

								if (stap_base.isValid()) {
									if (!(status = stap_base.bases[j].connect_forward(stap_base.bases[j + 1], true))) {
										status.perror("Model::Base::connect_forward 2 2");
										return status;
									}
								}
							}
						}

						/*
						 * Collect data for cylinder generation
						 */

						if (scaf_base.isValid() || stap_base.isValid()) {
							lowest_valid_base_index = MIN(lowest_valid_base_index, translation_index);
							highest_valid_base_index = MAX(highest_valid_base_index, translation_index);
						}
					}

					helix.scaf.push_back(scaf_base);
					helix.stap.push_back(stap_base);

					MProgressWindow::advanceProgress(1);
				}

				/*
				 * Create the helix cylinder using the collected range
				 */

				if (!(status = helix.helix.setCylinderRange(
						(double(-total_strand_length + (lowest_valid_base_index + highest_valid_base_index)) / 2.0 * DNA::STEP - DNA::Z_SHIFT) * (1 - scaf_direction * 2), 
						(highest_valid_base_index - lowest_valid_base_index) * DNA::STEP
					))) {
					status.perror("Helix::setCylinderRange");
					return status;
				}

				/*
				 * Now iterate over stap_colors and extract the base and its material
				 */

				for(Json::Value::iterator it = stap_colors.begin(); it != stap_colors.end(); ++it) {
					int index = (*it) [0].asInt(), color = (*it) [1].asInt();

					float c[] = { float(color >> 16) / 0x100, float((color >> 8) & 0xFF) / 0x100, float(color & 0xFF) / 0x100 };

					Model::Material material;

					if (!(status = Model::Material::Create("DNA_caDNAno1", c, material))) {
						status.perror("Material::Create");
						return status;
					}

					paintBases.push_back(std::make_pair(helix.stap[index].bases[0], material));
				}

				m_file.helices[num] = helix;
			}

			MProgressWindow::endProgress();

			if (!MProgressWindow::reserve())
				MGlobal::displayWarning("Can't reserve progress window, no progress information will be presented");
			MProgressWindow::setProgressRange(0, total_num_operations);
			MProgressWindow::setTitle("Importing json (caDNAno) file...");
			MProgressWindow::setProgressStatus(MString("Connecting bases"));
			MProgressWindow::setInterruptable(false);
			MProgressWindow::startProgress();

			/*
			 * Now, using the binary structure (and the generated bases), connect them to each other
			 */

			for(std::map<int, Helix>::iterator it = m_file.helices.begin(); it != m_file.helices.end(); ++it) {
				//int index = 0;

				for(size_t i = 0; i < it->second.scaf.size(); ++i) {
					if (it->second.scaf[i].isValid()) {
						/*
						 * There's a base here that's not [ -1, -1, -1, -1 ] thus look at its connections and connect it!
						 * Remember that the object at scaf[i].bases is an array of bases, only the first and last should be connected
						 * the ones inbetween have already been linked above
						 */

						if (it->second.scaf[i].hasPreviousConnection()) {
							/*
							 * Has a backward connection
							 */

							Base & backward = m_file.helices[it->second.scaf[i].connections[0]].scaf[it->second.scaf[i].connections[1]];

							if (!(status = backward.bases[backward.bases.size() - 1].connect_forward(it->second.scaf[i].bases[0], true))) {
								status.perror("Model::Base::connect_forward scaf 1");
								return status;
							}
						}

						if (it->second.scaf[i].hasNextConnection()) {
							/*
							 * Has a forward connection
							 */

							Base & forward = m_file.helices[it->second.scaf[i].connections[2]].scaf[it->second.scaf[i].connections[3]];

							if (!(status = it->second.scaf[i].bases[it->second.scaf[i].bases.size() - 1].connect_forward(forward.bases[0], true))) {
								status.perror("Model::Base::connect_forward scaf 2");
								return status;
							}
						}

						/*
						 * If there's a circular strand, it wont be colored, as we're only coloring strands where we find a 5' end
						 * thus, add all scaffold bases that have a previous from another strand than itself.
						 * We will have to color the same strand several times, but as the functor we're using does not apply color to bases
						 * that already have a color assigned, the performance penalty should be minimal
						 */

						/*if (it->second.scaf[i].connections[0] != it->first)
							paintBases.push_back(std::make_pair(it->second.scaf[i].bases[0], scaf_material));*/
					}

					if (it->second.stap[i].isValid()) {
						/*
						 * Also a valid base
						 */

						if (it->second.stap[i].hasPreviousConnection()) {
							/*
							 * Has a backward connection
							 */

							Base & backward = m_file.helices[it->second.stap[i].connections[0]].stap[it->second.stap[i].connections[1]];

							if (!(status = backward.bases[backward.bases.size() - 1].connect_forward(it->second.stap[i].bases[0], true))) {
								status.perror("Model::Base::connect_forward stap 1");
								return status;
							}
						}

						if (it->second.stap[i].hasNextConnection()) {
							/*
							 * Has a forward connection
							 */

							Base & forward = m_file.helices[it->second.stap[i].connections[2]].stap[it->second.stap[i].connections[3]];

							if (!(status = it->second.stap[i].bases[it->second.stap[i].bases.size() - 1].connect_forward(forward.bases[0], true))) {
								status.perror("Model::Base::connect_forward stap 2");
								return status;
							}
						}
					}

					MProgressWindow::advanceProgress(1);
				}
			}

			MProgressWindow::endProgress();

			if (!MProgressWindow::reserve())
				MGlobal::displayWarning("Can't reserve progress window, no progress information will be presented");
			MProgressWindow::setProgressRange(0, (int) paintBases.size());
			MProgressWindow::setTitle("Importing json (caDNAno) file...");
			MProgressWindow::setProgressStatus(MString("Painting strands"));
			MProgressWindow::setInterruptable(false);
			MProgressWindow::startProgress();

			/*
			 * Iterate over all scaffold bases tracking the strands we have, a lot of them are probably circular
			 */

			/*{
				 *
				 * Every strand has a number of bases identified by helix num and base index
				 *

				class BaseID {
				public:
					int helix, base;

					inline bool operator==(const BaseID & id) const {
						return helix == id.helix && base == id.base;
					}

					inline BaseID(int helix_, int base_) : helix(helix_), base(base_) {

					}
				};

				std::list< std::vector< BaseID > > strands;

				for(std::map<int, Helix>::iterator it = m_file.helices.begin(); it != m_file.helices.end(); ++it) {
					for(size_t i = 0; i < it->second.scaf.size(); ++i) {
						 *
						 * Find this base in the list of already saved bases
						 *

						BaseID base(it->first, (int) i);

						bool found = false;

						for(std::list< std::vector< BaseID > >::iterator it = strands.begin(); it != strands.end(); ++it) {
							std::vector<BaseID>::iterator base_it = std::find(it->begin(), it->end(), base);

							if (base_it != it->end()) {
								std::cerr << "Found" << std::endl;
								found = true;
								break;
							}
						}

						if (!found) {
							 *
							 * This base is part of a new strand that didn't exist before, create a new strand
							 *

							std::vector<BaseID> strand;
							strand.push_back(base);

							strands.push_back(strand);
						}
					}
				}

				 *
				 * Ok, we should have a list of all the unique scaffold strands
				 *

				std::cerr << "Got " << strands.size() << " scaffold strands to paint" << std::endl;

				for(std::list< std::vector< BaseID > >::iterator it = strands.begin(); it != strands.end(); ++it) {
					if (!it->empty())
						paintBases.push_back(std::make_pair(m_file.helices[it->begin()->helix].scaf[it->begin()->base].bases[0], scaf_material));
				}
			}*/

			/*
			 * Now paint the collected 5' ends
			 */

			PaintMultipleStrandsWithProgressBar functor;

			//for_each_ref(paintBases.begin(), paintBases.end(), functor);
			for(std::list< std::pair<Model::Base, Model::Material> >::iterator it = paintBases.begin(); it != paintBases.end(); ++it)
				functor(it->first, it->second);

			if (!(status = functor.status())) {
				status.perror("PaintMultipleStrandsFunctor");
				return status;
			}

			/*
			 * Refresh cylinder/base view
			 */

			if (!(status = Model::Helix::RefreshCylinderOrBases()))
				status.perror("Helix::RefreshCylinderOrBases");

			/*
			 * Select the newly created helices
			 * FIXME: Figure out why GCC can't use the shorter and faster template version
			 */

#ifndef MAC_PLUGIN
			class {
			public:
				inline Model::Helix operator() (const std::map<int, Helix>::iterator & input) const {
					return input->second.helix;
				}
			} select_functor;

			if (!(status = Model::Object::Select(m_file.helices.begin(), m_file.helices.end(), select_functor))) {
				status.perror("Object::Select");
				return status;
			}
#else
			std::list<Model::Helix> helices;
			for(std::map<int, Helix>::iterator it = m_file.helices.begin(); it != m_file.helices.end(); ++it)
				helices.push_back(it->second.helix);
			
			if (!(status = Model::Object::Select(helices.begin(), helices.end()))) {
				status.perror("Object::Select");
				return status;
			}
#endif /* MAC_PLUGIN */

			MProgressWindow::endProgress();

			return MStatus::kSuccess;
		}
	}
}