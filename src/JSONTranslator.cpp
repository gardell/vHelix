/*
 * JSONTranslator.cpp
 *
 *  Created on: Jul 25, 2011
 *      Author: bjorn
 */

#include <JSONTranslator.h>

#include <Helix.h>
#include <HelixBase.h>
#include <DNA.h>
#include <PaintStrand.h> // Because new bases get a color assigned
#include <ToggleCylinderBaseView.h> // Because we refresh the cylinder/base view
#include <Locator.h>
#include <Utility.h>

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MProgressWindow.h>
#include <maya/MDagPathArray.h>
#include <maya/MVector.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MPlugArray.h>
#include <maya/MQuaternion.h>
#include <maya/MMatrix.h>

#include <json/json.h>

#include <fstream>
#include <vector>
#include <limits>
#include <algorithm>

#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif /* MAX */

namespace Helix {
	//
	// The JSON Importer part
	//

	// This is a representation of a helix, exactly as represented in the JSON file, but will be faster
	// Note that staples length and scaf length are assumed to be the same, because slots can be empty, it seems to always be the case
	//

	struct JSON_import_helix {
		int *loop, *skip, *strands[2], row, col, low, hi; // Note that stap and scaf are not multidimensional arrays, but each element has 4 ints.
														  // The low and hi will contain the indices of the lowest and highest bases that are not [-1,-1,-1,-1]. Note that they will not be physical indices, but take care of skip and loop too!!
		size_t length;

		inline void resize(size_t len) {
			std::cerr << "Resize called" << std::endl;
			erase();

			loop = new int[len];
			skip = new int[len];
			strands[0] = new int[len * 4];
			strands[1] = new int[len * 4];

			length = len;
		}

		inline JSON_import_helix() : loop(NULL), skip(NULL), row(0), col(0), low(std::numeric_limits<int>::max()), hi(0) {
			strands[0] = NULL;
			strands[1] = NULL;
		}

		inline void erase() {
			if (loop)
				delete loop;
			if (skip)
				delete skip;
			if (strands[0])
				delete strands[0];
			if (strands[1])
				delete strands[1];
		}

		inline bool isEmpty() const {
			return !strands[0];
		}

		inline JSON_import_helix(const JSON_import_helix & copy) : loop(copy.loop), skip(copy.skip), row(copy.row), col(copy.col), low(copy.low), hi(copy.hi) {
			strands[0] = copy.strands[0];
			strands[1] = copy.strands[1];
			/*if (copy.strands[0] || copy.strands[1] || copy.loop || copy.skip)
				std::cerr << "WARNING: JSON_import_helix::#ctor called with non-null memebers!" << std::endl;*/
			std::cerr << "JSON_import_helix copy constructor" << std::endl;
		}
	};

	// Helper method, validates the index given considering the skip value

	inline bool IsValidIndex(std::vector<JSON_import_helix> & binary_content, int helix, int base) {
		return binary_content[helix].skip[base] >= 0;
	}

	// Helper method for getting the from and to indices of a given base, result will contain 4 int values

	inline bool GetTargetIndices(std::vector<JSON_import_helix> & binary_content, size_t strand_index, int helix, int base, int **result) {
		*result = &binary_content[helix].strands[strand_index][base * 4];

		return true;
	}

	// Helper method, takes helix and base indices and returns either that base or another more appropriate (considers skip and loop)
	//		str_strand is either stap or scaf
	// 		Direction is either ForwardIndex or BackwardIndex and determines the type of index given

	enum {
		ForwardIndex = 1,
		BackwardIndex = 0
	};

	bool GetValidIndex(std::vector<JSON_import_helix> & binary_content, size_t strand_index, int helix, int base, unsigned int direction, int & result_helix, int & result_base, int & result_loop_index) {
		int i_helix = helix, i_base = base;

		while (binary_content[helix].skip[i_base] < 0) {
			// This index is not a valid index, because it's supposed to be skipped, find a new index by iterating in the direction of `direction`

			int *targets = &binary_content[i_helix].strands[strand_index][i_base * 4];

			i_helix = targets[direction * 2 + 0];
			i_base = targets[direction * 2 + 1];
		}

		result_loop_index = direction == ForwardIndex ? 0 : binary_content[i_helix].loop[i_base];
		result_helix = i_helix;
		result_base = i_base;

		return true;
	}

	// Helper method

	/*bool strand_num_less_comp(const Json::Value & strand_a, const Json::Value & strand_b) {
		return strand_a ["num"].asInt() < strand_b ["num"].asInt();
	}*/

	/*
	 * Override the paint functor to increase the progress bar
	 */

	class PaintMultipleStrandsWithProgressBar : public Controller::PaintMultipleStrandsFunctor {
	public:

		void operator() (Model::Strand & strand) {
			Controller::PaintMultipleStrandsFunctor::operator()(strand);
			MProgressWindow::advanceProgress(1);
		}
	};

	MStatus JSONTranslator::reader (const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode) {
		MStatus status;
		static const char *str_strands[] = { "scaf", "stap" };

		// Open and parse the JSON file
		//

		// Because jumping a lot in the JSON file is really slow for the parser, we must make a memory copy of it with native types :/

		std::vector<JSON_import_helix> binary_content;
		double average_row = 0.0, average_col = 0.0; // Used for centering the imported model

		int total_progress = 0; // Note that the number here doesn't have ANY meaning, no memory allocations etc can be done using this. It is just used for the ProgressBar

		{
			std::fstream fileh(file.fullName().asChar(), std::ios_base::in);

			if (!fileh) {
				MGlobal::displayError(MString("Failed to open file \"") + file.fullName() + "\" for reading");
				return MStatus::kFailure;
			}

			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(fileh, root, false)) {
				MGlobal::displayError(MString("Failed to parse file \"") + file.fullName() + "\" for reading");
				return MStatus::kFailure;
			}

			// Parse the JSON content and create nodes, according to the format defined by the caDNAno editor
			//

			if (!root.isObject()) {
				MGlobal::displayError(MString("Failed to parse file \"") + file.fullName() + "\" for reading");
				return MStatus::kFailure;
			}

			MGlobal::displayInfo(MString("Loading the JSON file \"") + file.fullName() + "\" name: \"" + root ["name"].asCString() + "\"");

			Json::Value vstrands = root ["vstrands"];

			if (!vstrands.isArray()) {
				MGlobal::displayError(MString("Failed to parse file \"") + file.fullName() + "\" for reading");
				return MStatus::kFailure;
			}

			/*Json::ValueIterator vstrands_largest_num_element = std::max_element(vstrands.begin(), vstrands.end(), strand_num_less_comp);

			if (vstrands_largest_num_element == vstrands.end()) {
				std::cerr << "std::max_element failed" << std::endl;
				return MStatus::kFailure;
			}

			unsigned int binary_content_size = vstrands[vstrands_largest_num_element.index()] ["num"].asInt() + 1;*/

			int binary_content_size = 0;

			for(Json::ArrayIndex i = 0; i < vstrands.size(); ++i)
				binary_content_size = MAX(binary_content_size, vstrands[i]["num"].asInt());

			// Increase to include the zero index
			++binary_content_size;

			std::cerr << "Reserving space for binary version of JSON file, vstrands::size: " << binary_content_size << std::endl;

			binary_content.resize(binary_content_size);

			std::cerr << "Resized" << std::endl;

			for(Json::ArrayIndex i = 0; i < vstrands.size(); ++i) {
				Json::Value bases[] = { vstrands[i] [str_strands[0]], vstrands[i] [str_strands[1]] };
				Json::Value loop = vstrands[i] ["loop"],
							skip = vstrands[i] ["skip"];

				int num = vstrands[i] ["num"].asInt();

				std::cerr << "accessing binary_content at index: " << num << " where there are " << binary_content.size() << " total elements" << std::endl;

				binary_content[num].row = vstrands[i] ["row"].asInt();
				binary_content[num].col = vstrands[i] ["col"].asInt();
				binary_content[num].resize(bases[1].size());

				average_row += (double) binary_content[num].row;
				average_col += (double) binary_content[num].col;

				int index = 0;

				for (size_t j = 0; j < binary_content[num].length; ++j) {
					for(size_t k = 0; k < 2; ++k) {
						int sum = 0;
						for(size_t l = 0; l < 4; ++l) {
							int val = bases[k][(Json::ArrayIndex) j][(Json::ArrayIndex) l].asInt();
							sum += val;

							binary_content[num].strands[k][j * 4 + l] = val;
						}

						// Try to save the indices if this is not an empty one

						if (sum != -4) {
							binary_content[num].low = MIN(index, binary_content[num].low);
							binary_content[num].hi = index;
						}
					}

					binary_content[num].loop[j] = loop[(Json::ArrayIndex) j].asInt();
					binary_content[num].skip[j] = skip[(Json::ArrayIndex) j].asInt();

					index += binary_content[num].skip[j] + binary_content[num].loop[j] + 1;
				}

				total_progress += int(binary_content[num].length); // For the progressbar later on
			}

			if (vstrands.size() > 0) {
				average_col /= (double) vstrands.size();
				average_row /= (double) vstrands.size();
			}
		}

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Can't reserve progress window, no progress information will be presented");
		MProgressWindow::setProgressRange(0, total_progress * 2);
		MProgressWindow::setTitle("Importing json (caDNAno) file...");
		MProgressWindow::setProgressStatus(MString("Parsing file: \"") + file.fullName() + "\"");
		MProgressWindow::setInterruptable(false);
		MProgressWindow::startProgress();

		// This is a name translation table, because the first loop below will create all helices and bases while the next loop will create all bindings required
		// The second loop will translate the indices in the JSON file into MDagPath objects using this table
		// The layout is: helix, base_index, loop_index
		std::vector< std::vector<MObjectArray *> > base_names[2];

		// Stores the bases that we need to call paintStrands on

		std::list<Model::Strand> colorBases;

		// Preallocating increases performance
		for(int k = 0; k < 2; ++k)
			base_names[k].resize(binary_content.size(), std::vector<MObjectArray *> ());

		for(Json::ArrayIndex i = 0; i < binary_content.size(); ++i) {
			if (binary_content[i].isEmpty())
				continue;

			double shuffle = (-1.0 * ((binary_content[i].row + 1) % 2) + (binary_content[i].row % 2)) * (float((binary_content[i].col + 1) % 2) + -1.0 * (binary_content[i].col % 2));

			MVector honeycomb_translation(
					DNA::HONEYCOMB_X_STRIDE * (double(binary_content[i].col) - average_col),
					DNA::HONEYCOMB_Y_STRIDE * (double(binary_content[i].row) - average_row) + DNA::HONEYCOMB_Y_OFFSET * shuffle
			);

			double honeycomb_rotation_offset = 90.0 + 90.0 * shuffle;

			// Create the Helix object
			//

			MFnDagNode helix_dagNode;
			MObject helix = helix_dagNode.create(Helix::id, MObject::kNullObj, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			// Do a setAttr m_helix.displayHandle true equivalent
			//

			{
				MPlug displayHandle(helix, MPxTransform::displayHandle);

				if (!(status = displayHandle.setBool(true)))
					status.perror("MPlug::setBool");
			}

			double height = DNA::STEP * (binary_content[i].hi - binary_content[i].low);

			// Translate the helix
			//

			MFnTransform helix_transform(helix);

			// Rotate it 180.0 degrees IF it is an odd numbered

			if (i % 2 == 1) {
				MEulerRotation eulerRotation(M_PI, 0.0, 0.0);

				if (!(status = helix_transform.setRotation(eulerRotation))) {
					status.perror("MFnTransform::setRotation");
					return status;
				}
			}

			helix_transform.setTranslation(honeycomb_translation, MSpace::kTransform);

			// Create the cylinder
			//

			// This is still easiest to do using MEL for a lot of reasons

			if (!(status = MGlobal::executeCommand(
					MString("$cylinder = `cylinder -radius ") + DNA::RADIUS + " -heightRatio " + (height / DNA::RADIUS) + (" -name \"cylinderRepresentation\" -axis 0.0 0.0 1.0`;\n"
					"move -relative 0.0 0.0 ") + (DNA::STEP * (binary_content[i].hi + binary_content[i].low) / 2.0 - height * (i % 2)) + (" $cylinder[0];\n"
					"parent -relative $cylinder[0] ") + helix_dagNode.fullPathName() + ";\n"
					))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			// Now generate the helix locator node that will display visual information
			//

			MFnDagNode locator_dagNode;
			MObject locator_object = locator_dagNode.create(HelixLocator::id, helix, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			int index = 0; // Increases not only with the index of staples/scaffolds, but also for loops, it _decreases_ for skips
			std::vector<MObjectArray *> current_base_names[2];

			for (size_t j = 0; j < 2; ++j)
				current_base_names[j].reserve(binary_content[i].length);

			for(size_t j = 0; j < binary_content[i].length; ++j) {
				int loopIndex = binary_content[i].loop [j];
				bool skipIndex = binary_content[i].skip [j] < 0;

				MObjectArray *loop_base_names[] = { new MObjectArray(), new MObjectArray() };

				for(size_t k = 0; k < 2; ++k)
					loop_base_names[k]->setSizeIncrement(loopIndex + 1);

				if (!skipIndex) {
					// Generate the scaffold and staple bases
					//

					for (int k = 0; k < loopIndex + 1; ++k) {
						// Create the helix bases, staples and scaffolds
						//

						MObjectArray base_pair;

						for(int l = 0; l < 2; ++l) {
							if (binary_content[i].strands[l][j * 4] == -1 && binary_content[i].strands[l][j * 4 + 1] == -1 && binary_content[i].strands[l][j * 4 + 2] == -1 && binary_content[i].strands[l][j * 4 + 3] == -1)
								continue;

							//int directional_index = index + (i % 2 == 0 ? (l == 1 ? loopIndex - k : k) : k);

							int directional_index = (i % 2 == 0) ? (index + (l == 1 ? loopIndex - k : k)) : -(index + k);

							MObject helixBase;

							//
							// Helix_CreateBase is where the creation of the node is made
							//

							if (!(status = Helix_CreateBase(helix, MString(DNA::strands[l]) + "_base_" + directional_index, MVector(
									DNA::RADIUS * cos(DEG2RAD(honeycomb_rotation_offset + 155.0 * l + DNA::PITCH * directional_index)),
									DNA::RADIUS * sin(DEG2RAD(honeycomb_rotation_offset + 155.0 * l + DNA::PITCH * directional_index)),
									DNA::STEP * directional_index), helixBase))) {
								status.perror("Helix_CreateBase");
								return status;
							}

							base_pair.append(helixBase);
							loop_base_names[l]->append(helixBase);

							// Is this base a left end base? In that case, save it for coloring!

							if ((binary_content[i].strands[l][j * 4] == -1 && binary_content[i].strands[l][j * 4 + 1] == -1) /*||
								(binary_content[i].strands[l][j * 4 + 2] == -1 && binary_content[i].strands[l][j * 4 + 3] == -1) ||
								 (int) i != binary_content[i].strands[l][j * 4] ||
								 (int) i != binary_content[i].strands[l][j * 4 + 2]*/) {
								MDagPath helixBase_dagPath;

								if (!(status = MFnDagNode(helixBase).getPath(helixBase_dagPath))) {
									status.perror("MFnDagNode::getPath");
									return status;
								}

								Model::Base helixBase_base(helixBase_dagPath);
								colorBases.push_back(Model::Strand(helixBase_base));
							}
						}

						// Connect this pair to each other using the label attribute. The direction is important!
						if (base_pair.length() == 2) {
							MDGModifier dgModifier;

							if (!(status = dgModifier.connect(base_pair[0], HelixBase::aLabel, base_pair[1], HelixBase::aLabel))) {
								status.perror("MDGModifier::connect");
								return status;
							}

							if (!(status = dgModifier.doIt())) {
								status.perror("MDGModifier::connect");
								return status;
							}
						}
					}

					index += loopIndex + 1;
				}

				MProgressWindow::advanceProgress(1);

				for(int k = 0; k < 2; ++k)
					current_base_names[k].push_back(loop_base_names[k]);
			}

			for(int k = 0; k < 2; ++k)
				base_names[k] [i] = current_base_names[k];
		}

		// Now reset progresswindow, and do the bindings between the bases, use the same progress range as before
		// For some reason, just setting progress to 0 doesn't work..
		//

		MProgressWindow::setProgressStatus("Setting up bindings");
		std::cerr << "Setting up bindings" << std::endl;

		//MDGModifier dgModifier;

		for(size_t i = 0; i < binary_content.size(); ++i) {
			MDGModifier dgModifier; // Placing it here, and not outside gives a better user responseness

			for(size_t j = 0; j < binary_content[i].length; ++j) {
				int loopIndex = binary_content[i].loop [j];
				bool skipIndex = binary_content[i].skip [j] < 0;

				if (!skipIndex) {
					for (int k = 0; k < 2; ++k) {
						// Find the indices we're coming from and going to

						int *targets = &binary_content[i].strands[k][j * 4];

						if (targets[2] != -1 && targets[3] != -1) {
							int valid_targets[3];

							if (!GetValidIndex(binary_content, k, targets[2], targets[3], ForwardIndex, valid_targets[0], valid_targets[1], valid_targets[2])) {
								std::cerr << "GetValidIndex forward failed" << std::endl;
								return MStatus::kFailure;
							}

							MObject forward_object = (*base_names[k][valid_targets[0]] [valid_targets[1]]) [valid_targets[2]],
									 forward_target_object = (*base_names[k][i][j])[loopIndex];

							// Connecting from our last base in the loop to the `to` base

							if (!(status = dgModifier.connect(forward_object, HelixBase::aBackward, forward_target_object, HelixBase::aForward))) {
								status.perror("MDGModifier::connect");
								return status;
							}
						}

						// Note that loopIndex is n-1 of the total bases at this index
						for(int l = 0; l < loopIndex; ++l) {
							MObject backward_object = (*base_names[k][i][j])[l],
									 this_object = (*base_names[k][i][j])[l + 1];

							if (!(status = dgModifier.connect(this_object, HelixBase::aBackward, backward_object, HelixBase::aForward))) {
								status.perror("MDGModifier::connect");
								return status;
							}
						}

						// Now create connections for the helix_forward and helix_backward, for loops it works the same as above, but for the first and last, connect to the prev and next bases on the current strand

						/*if (j < binary_content[i].length - 1 && binary_content[i].skip[j] >= 0) {
							int *next_targets = &binary_content[i].strands[k][(j + 1) * 4];
							int sum = 0;
							for(size_t l = 0; l < 4; ++l)
								sum += next_targets[l];

							if (sum != -4) {
								// The base next to this one is a valid one and we should create a connection

								MObject this_base = (*base_names[k][i][j])[loopIndex], next_base = (*base_names[k][i][j + 1])[0];

								if (!(status = dgModifier.connect(this_base, HelixBase::aHelix_Backward, next_base, HelixBase::aHelix_Forward))) {
									status.perror("MDGModifier::connect");
									return status;
								}
							}
						}*/
					}
				}

				MProgressWindow::advanceProgress(1);
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}
		}

		// Because MObjectArray has its operator= set to protected/private, we must deallocate them of using the std::vectors erase methods
		for(size_t i = 0; i < 2; ++i) {
			for(std::vector< std::vector<MObjectArray *> >::iterator it = base_names[i].begin(); it != base_names[i].end(); ++it) {
				for(std::vector<MObjectArray *>::iterator iit = it->begin(); iit != it->end(); ++iit)
					delete *iit;
			}
		}


		/*std::cerr << "Doing the MDGModifier, might be slow" << std::endl;
		MProgressWindow::setProgressStatus("Postprocessing, this might take a while...");

		if (!(status = dgModifier.doIt())) {
			status.perror("MDGModifier::doIt");
			return status;
		}*/

		MProgressWindow::endProgress();
		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Can't reserve progress window, no progress information will be presented");

		MProgressWindow::setProgressRange(0, (int) colorBases.size());
		MProgressWindow::setTitle("Importing json (caDNAno) file...");
		MProgressWindow::setProgressStatus(MString("Coloring strands"));
		MProgressWindow::setInterruptable(false);
		MProgressWindow::startProgress();

		/*
		 * Paint all strands
		 */

		std::cerr << "Coloring strands" << std::endl;

		std::for_each(colorBases.begin(), colorBases.end(), PaintMultipleStrandsWithProgressBar());

		/*for(unsigned int i = 0; i < colorBases.length(); ++i) {
			PaintStrand paintStrand;
			MDagPathArray base;
			base.setLength(1);
			base[0] = colorBases[i];

			if (!(status = paintStrand.paintStrands(base))) {
				status.perror("PaintStrand::paintStrands");
				return status;
			}

		
			paintStrandOperation.setMaterial(materials[rand() % numMaterials]);


			MProgressWindow::advanceProgress(1);
		}*/

		if (!(status = MGlobal::executeCommand(MEL_TOGGLECYLINDERBASEVIEW_COMMAND " -refresh true"))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		MProgressWindow::endProgress();

		return MStatus::kSuccess;
	}

	//
	// The JSON Exporter part
	//

	// Data structure for saving helices for export

	class JSON_export_helix;

	class JSON_export_base {
	public:

		MObject m_base;
		int m_index;

		JSON_export_helix *m_next_helix;
		int m_next_base;

		JSON_export_helix *m_prev_helix;
		int m_prev_base;

		inline JSON_export_base(const MObject & base, int index = -1, JSON_export_helix *next_helix = NULL, int next_base = -1, JSON_export_helix *prev_helix = NULL, int prev_base = -1) : m_base(base), m_index(index), m_next_helix(next_helix), m_next_base(next_base), m_prev_helix(prev_helix), m_prev_base(prev_base) {

		}
	};

	class JSON_export_helix {
	public:

		MDagPath m_helix;
		std::vector<JSON_export_base> m_strands[2]; // First scaf then stap
		bool m_odd; // If the direction of the base is negative when projected on the Z-axis, m_odd is true

		inline JSON_export_helix(const MDagPath & helix, bool odd = false) : m_helix(helix), m_odd(odd) {

		}
	};

	// Utility method, looks up the connected bases from the given `base` and translates them using the table `helices` into pointers and indices

	MStatus TranslateConnectionForExport(const MObject & base, const std::vector<JSON_export_helix> & helices, const JSON_export_helix **target_helices, int target_bases[2]) {
		MStatus status;

		MPlug plugs[] = { MPlug(base, HelixBase::aForward), MPlug(base, HelixBase::aBackward) };

		for(size_t i = 0; i < 2; ++i) {
			MPlugArray plugArray;
			bool connected = plugs[i].connectedTo(plugArray, true, true, &status);

			if (!status) {
				status.perror("MPlugArray[i]::connectedTo");
				return status;
			}

			bool foundConnection = false;

			if (connected) {
				// Find out if any of the connected is another base

				for(unsigned int j = 0; j < plugArray.length(); ++j) {
					MFnDagNode target_dagNode(plugArray[j].node(&status));

					if (!status) {
						status.perror("MPlugArray[j]::node");
						return status;
					}

					if (target_dagNode.typeId(&status) == HelixBase::id) {
						// We found a connected base, use this one

						MObject target_base = target_dagNode.object(&status);

						if (!status) {
							status.perror("MFnDagNode::object");
							return status;
						}

						// Find the base in the "table" of helices/bases

						for(std::vector<JSON_export_helix>::const_iterator it = helices.begin(); it != helices.end(); ++it) {
							for(unsigned int k = 0; k < 2; ++k) {
								unsigned int l = 0;
								for(std::vector<JSON_export_base>::const_iterator bit = it->m_strands[k].begin(); bit != it->m_strands[k].end(); ++bit, ++l) {
									if (bit->m_base == target_base) {
										target_helices[i] = &(*it);
										target_bases[i] = l;//bit->m_index;

										foundConnection = true;
										break;
									}
								}
							}

							if (foundConnection)
								break;
						}
					}

					if (foundConnection)
						break;
				}
			}

			// Assume no connection exist

			if (!foundConnection) {
				target_helices[i] = NULL;
				target_bases[i] = -1;
			}
		}

		return MStatus::kSuccess;
	}

	// This method sets the m_odd flag, and projects all bases onto the XY-plane to figure out their ordering for m_index. It chooses scaf or stap depending on the direction of the helix

	MStatus GenerateHelixBaseDataForExport(JSON_export_helix & helix) {
		MStatus status;
		MQuaternion rotation_quaternion;

		if (!(status = MFnTransform(helix.m_helix).getRotation(rotation_quaternion, MSpace::kWorld))) {
			status.perror("MFnTransform::getRotation");
			return status;
		}

		helix.m_odd = (rotation_quaternion.asMatrix() * MVector::zAxis).z > 0.0;

		// If odd is false, use the positive directed bases as the scaffold (along the z-axis)
		// If odd is true, use the negative directed bases as the scaffold

		std::cerr << "The helix " << helix.m_helix.fullPathName().asChar() << ", is an " << (helix.m_odd ? "odd" : "even") << " one" << std::endl;

		MFnDagNode helix_dagNode(helix.m_helix);

		MObjectArray unsorted_strands[2]; // We put them here as either 0 = scaf, 1 = stap, then later on we'll sort them using their z-axis value

		size_t childCount = helix_dagNode.childCount(&status);

		if (!status) {
			status.perror("MFnDagNode::childCount");
			return status;
		}

		for(unsigned int i = 0; i < childCount; ++i) {
			MObject child_object = helix_dagNode.child(i, &status);

			if (!status) {
				status.perror("MFnDagNode::child");
				return status;
			}

			MFnDagNode child_dagNode(child_object);

			if (child_dagNode.typeId(&status) == HelixBase::id) {
				bool isForward = HelixBase_isForward(child_object, &status);

				if (!status) {
					status.perror("HelixBase_isForward");
					return status;
				}

				unsorted_strands[isForward ? 0 : 1].append(child_object);
			}

			if (!status) {
				status.perror("MFnDagNode::typeId");
				return status;
			}
		}

		for(size_t i = 0; i < 2; ++i) {
			MObjectArray sorted;

			if (!(status = HelixBases_sort(unsorted_strands[i], sorted))) {
				status.perror("HelixBases_sort");
				return status;
			}

			// Now translate them into the structures, some cleanup here could increase performance

			for(unsigned int j = 0; j < sorted.length(); ++j)
				helix.m_strands[i].push_back(JSON_export_base(sorted[j]));
		}

		return MStatus::kSuccess;
	}

	// Using the m_odd attribute of the helix, and measuring the number of inter-helix connections between each pair of helix, try to figure out some kind of a logic structure

	class JSON_export_helix_position {
	public:
		int m_num, m_row, m_col;

		inline JSON_export_helix_position(int num = 0, int row = 0, int col = 0) : m_num(num), m_row(row), m_col(col) {

		}
	};

	MStatus OptimizeConnectedHelicesOrdering(const std::vector<JSON_export_helix> & helices, std::vector<std::pair<JSON_export_helix *, JSON_export_helix_position> > & result) {

		return MStatus::kSuccess;
	}

	MStatus JSONTranslator::writer (const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) {
		MStatus status;

		Json::StyledWriter writer;
		Json::Value root(Json::objectValue);

		// Ok, now fill the JSON document with the scene data
		//

		std::vector<JSON_export_helix> helices;

		MItDag dagIt(MItDag::kBreadthFirst, MFn::kTransform, &status);

		for(; !dagIt.isDone(); dagIt.next()) {
			MDagPath path;

			if (!(status = dagIt.getPath(path))) {
				status.perror("MItDag::getPath");
				return status;
			}

			MFnDagNode dagNode(path);

			if (dagNode.typeId(&status) == Helix::id) {
				// Found a helix, export it

				JSON_export_helix helix(path);

				if (!(status = GenerateHelixBaseDataForExport(helix))) {
					status.perror("GenerateHelixBaseDataForExport");
					return status;
				}

				// Now the helix should contain all the bases, sorted on their z axis value, ready for export

				helices.push_back(helix);
			}

			if (!status) {
				status.perror("MFnDagNode::typeId");
				return status;
			}
		}

		// Now, iterate over the helices we're exporting to extract their targets
		//

		for(std::vector<JSON_export_helix>::iterator it = helices.begin(); it != helices.end(); ++it) {
			for(size_t i = 0; i < 2; ++i) {
				for(std::vector<JSON_export_base>::iterator bit = it->m_strands[i].begin(); bit != it->m_strands[i].end(); ++bit) {
					const JSON_export_helix *target_helices[2];
					int target_bases[2];

					if (!(status = TranslateConnectionForExport(bit->m_base, helices, target_helices, target_bases))) {
						status.perror("TranslateConnectionForExport");
						return status;
					}

					// FIXME
					bit->m_next_helix = const_cast<JSON_export_helix *>(target_helices[0]);
					bit->m_prev_helix = const_cast<JSON_export_helix *>(target_helices[1]);
					bit->m_next_base = target_bases[0];
					bit->m_prev_base = target_bases[1];
				}
			}
		}

		// Ok, our structure is ready for export, just write it to the JSON interface
		//

		// DEBUG: Dump some info:

		/*std::cerr << "Dumping data:" << std::endl;
		for(std::vector<JSON_export_helix>::iterator it = helices.begin(); it != helices.end(); ++it) {
			std::cerr << "Helix: " << it->m_helix.fullPathName().asChar() << std::endl;

			for(size_t i = 0; i < 2; ++i) {
				std::cerr << "strand no " << i << std::endl;

				for(std::vector<JSON_export_base>::iterator bit = it->m_strands[i].begin(); bit != it->m_strands[i].end(); ++bit) {
					std::cerr << MFnDagNode(bit->m_base).fullPathName() << ": [ ";
					if (bit->m_prev_helix)
						std::cerr << bit->m_prev_helix->m_helix.fullPathName().asChar();
					else
						std::cerr << "(null)";

					std::cerr << ", " << bit->m_prev_base << ", ";
					if (bit->m_next_helix)
						std::cerr << bit->m_next_helix->m_helix.fullPathName().asChar();
					else
						std::cerr << "(null)";

					std::cerr << ", " << bit->m_next_base << "]\t";
				}

				std::cerr << std::endl;
			}
		}


		std::cerr << "End dump" << std::endl;*/

		std::string document = writer.write(root);
		std::fstream fileh(file.fullName().asChar(), std::ios_base::out);

		if (!fileh) {
			MGlobal::displayError(MString("Failed to open file \"") + file.fullName() + "\" for writing");
			return MStatus::kFailure;
		}

		// Fill the root JSON element with all elements required

		// FIXME: Generate the correct date and all that...
		root ["name"] = Json::Value(Json::StaticString("fixme"));

		Json::Value & vstrands = root["vstrands"] = Json::Value(Json::arrayValue);

		for(std::vector<JSON_export_helix>::iterator it = helices.begin(); it != helices.end(); ++it) {
			Json::Value helix(Json::objectValue);

			Json::Value strands[] = { helix["stap"] = Json::Value(Json::objectValue), helix["scaf"] = Json::Value(Json::objectValue) };

			// Write staples and scaffolds arrays

			for(size_t i = 0; i < 2; ++i) {
				int prev_helix, next_helix; // These need to be figured out, we only have the pointer to the helices
			}

			// Write num, row, col, skip and loop (skip and loop are just arrays of zeroes)
		}

		fileh << document;

		fileh.close();

		return MStatus::kSuccess;
	}

	MPxFileTranslator::MFileKind JSONTranslator::identifyFile(const MFileObject& file, const char* buffer, short size) const {
		MString filePath = file.resolvedFullName().toLowerCase();
		return strstr(filePath.asChar(), ".json") != NULL ? MPxFileTranslator::kIsMyFileType : MPxFileTranslator::kNotMyFileType;
	}

	// The generic implementation part
	//

	bool JSONTranslator::haveWriteMethod () const {
		return true;
	}

	bool JSONTranslator::haveReadMethod () const {
		return true;
	}

	bool JSONTranslator::canBeOpened () const {
		return true;
	}

	MString JSONTranslator::defaultExtension () const {
		return MString("json");
	}

	void *JSONTranslator::creator() {
		return new JSONTranslator();
	}
}
