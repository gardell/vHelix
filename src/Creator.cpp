/*
 * Creator.cpp
 *
 *  Created on: 9 jul 2011
 *      Author: johan
 */

#include <DNA.h>
#include <Creator.h>
#include <Helix.h>
#include <HelixBase.h>
#include <PaintStrand.h> // We paint the newly created strands
#include <ToggleCylinderBaseView.h> // And refresh the display of cylinders and bases
#include <Locator.h>
#include <Utility.h>

#include <cmath>
#include <functional>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MDGModifier.h>
#include <maya/MProgressWindow.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MObjectArray.h>
#include <maya/MPxTransform.h>
#include <maya/MDagModifier.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MPlane.h>

namespace Helix {
	Creator::Creator() {

	}

	Creator::~Creator() {

	}

	bool Creator::CreateBaseControl::generateSharedCoordinates() const {
		return false;
	}

	bool Creator::CreateBaseControl::operator() (const MVector & translation, int index, bool forward) const {
		return true;
	}

	int Creator::default_bases = DNA::CREATE_DEFAULT_NUM_BASES;

	//MStatus Creator::createHelix(int bases, double rotation, const MVector & origo, Model::Helix & helix, const CreateBaseControl & control) {
	MStatus Creator::createHelix(int bases, Model::Helix & helix, const MMatrix & transform, const CreateBaseControl & control) {
		default_bases = bases;

		MStatus status;

		// Setup the progress window
		//

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("Create new helix");
		MProgressWindow::setProgressStatus("Generating new helix bases...");
		MProgressWindow::setProgressRange(0, bases * 2);
		MProgressWindow::startProgress();

		/*
		 * Create the helix
		 */

		if (!(status = Model::Helix::Create("helix1", MMatrix::identity, helix))) {
			status.perror("Helix::Create");
			return status;
		}

		/*
		 * If control requires world coordinates, we require the world matrix for the newly created helix
		 * Obtained below
		 */

		MMatrix helix_matrix;

		/*
		 * Rotate and translate the helix
		 */

		{
			MFnTransform helix_transform(helix.getDagPath(status));

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			/*if (!(status = helix_transform.setTranslation(origo, MSpace::kTransform))) {
				status.perror("MFnTransform::setTranslation");
				return status;
			}

			static double rotation_xyz[] = { 0.0, 0.0, DEG2RAD(rotation) };

			if (!(status = helix_transform.setRotation(rotation_xyz, MTransformationMatrix::kXYZ, MSpace::kTransform))) {
				status.perror("MFnTransform::setRotation");
				return status;
			}*/

			if (!(status = helix_transform.set(MTransformationMatrix(transform)))) {
				status.perror("MFnTransform::set");
				return status;
			}

			if (control.generateSharedCoordinates()) {
				std::cerr << "Setting up matrix for shared coordinates" << std::endl;

				helix_matrix = helix_transform.transformationMatrix(&status);

				if (!status) {
					status.perror("MFnTransform::transformationMatrix");
					return status;
				}


			}
		}

		/*
		 * Add the bases
		 */

		MVector basePositions[2];
		Model::Base base_objects[2], last_base_objects[2];

		for(int i = 0; i < bases; ++i) {
			/*
			 * Get the positions for the bases from DNA.h
			 */

			if (!(status = DNA::CalculateBasePairPositions(double(i), basePositions[0], basePositions[1], 0.0, bases))) {
				status.perror("DNA::CalculateBasePairPositions");
				return status;
			}

			/*
			 * Now create the two base pairs
			 */

			bool created[2];

			for(int j = 0; j < 2; ++j) {
				MString strand(DNA::GetStrandName(j));

				created[j] = control(control.generateSharedCoordinates() ? (helix_matrix * basePositions[j]) : basePositions[j], i, j == 0);

				if (created[j]) {
					if (!(status = Model::Base::Create(helix, strand + "_" + (i + 1), basePositions[j], base_objects[j]))) {
						status.perror("Base::Create");
						return status;
					}

					if (!(status = base_objects[j].setMaterial(m_materials[j]))) {
						status.perror("Base::setMaterial");
						return status;
					}
				}
				else
					base_objects[j] = MObject::kNullObj;

				MProgressWindow::advanceProgress(1);
			}

			/*
			 * Now connect them
			 */

			if (created[0] && created[1]) {
				if (!(status = base_objects[0].connect_opposite(base_objects[1], true))) {
					status.perror("Base::connect_opposite");
					return status;
				}
			}

			/*
			 * Now connect the previous bases to the newly created ones
			 */

			if (last_base_objects[0] && created[0]) {
				if (!(status = last_base_objects[0].connect_forward (base_objects[0], true))) {
					status.perror("Base::connect_forward 0");
					return status;
				}
			}

			if (last_base_objects[1] && created[1]) {
				if (!(status = base_objects[1].connect_forward (last_base_objects[1], true))) {
					status.perror("Base::connect_forward 1");
					return status;
				}
			}

			for(int j = 0; j < 2; ++j)
				last_base_objects[j] = base_objects[j];
		}

		/*
		 * Setup cylinder
		 */

		if (!(status = helix.setCylinderRange(0.0, DNA::STEP * (bases - 1))))
			status.perror("Helix::setCylinderRange");

		/*
		 * Refresh cylinder/base view
		 */

		/*if (!(status = Model::Helix::RefreshCylinderOrBases()))
			status.perror("Helix::RefreshCylinderOrBases");*/

		MProgressWindow::endProgress();

		/*
		 * Select the newly created Helix
		 */

		if (!(status = Model::Object::Select(&helix, &helix + 1)))
			status.perror("Object::Select");

		m_helices.push_back(helix);

		return MStatus::kSuccess;
	}

	MStatus Creator::createHelix(const MVector & origo, const MVector & end, double rotation, Model::Helix & helix, const CreateBaseControl & control) {
		MVector distance = end - origo, direction = distance.normal(), end_origo = origo + distance / 2.0;

		return createHelix(end_origo, direction, (int) floor(distance.length() / DNA::STEP + 0.5), rotation, helix, control); // There's no round() in the C++ STD!
	}

	MStatus Creator::createHelix(const MVector & origo, const MVector & direction, int bases, double rotation, Model::Helix & helix, const CreateBaseControl & control) {
		MStatus status;

		if (bases <= 0) {
			MGlobal::displayWarning(MString("Ignoring request to create helix with ") + bases + " bases");
			return MStatus::kSuccess;
		}

		/*
		 * The quaternion will create a rotation between the first given axis to the second along their mutually perpendicular axis.
		 * this is not really what we want, thus, i'll do it in two steps, first rotate to a vector that is on the x-y plane, then rotate
		 * again to obtain the correct z value. This way, our helix will keep the y-axis as zero.
		 */

		MVector direction_xy(direction.x, direction.y);

		MQuaternion quaternion1(MVector::zAxis, direction_xy), quaternion2(direction_xy, direction);

		/*if (!(status = createHelix(bases, rotation, origo, helix, control))) {
			status.perror("createHelix(bases, helix)");
			return status;
		}

		MFnTransform helix_transform(helix.getDagPath(status));

		if (!status) {
			status.perror("Helix::getDagPath");
			return status;
		}*/

		/*
		 * Now rotate along the Z-axis with the given rotation
		 */

		double rotation_vector[] = { 0.0, 0.0, DEG2RAD(rotation) };
		MTransformationMatrix matrix;

		if (!(status = matrix.setRotation(rotation_vector, MTransformationMatrix::kXYZ))) {
			status.perror("MTransformationMatrix::setRotation");
			return status;
		}

		matrix.rotateBy(quaternion1, MSpace::kTransform);
		matrix.rotateBy(quaternion2, MSpace::kTransform);

		if (!(status = matrix.addTranslation(origo, MSpace::kTransform))) {
			status.perror("MTransformationMatrix::addTranslation");
			return status;
		}

		if (!(status = createHelix(bases, helix, matrix.asMatrix(), control))) {
			status.perror("createHelix(bases, helix)");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Creator::createHelix(const MPointArray & points, double rotation) {
		MStatus status;

		double current_rotation = rotation;
		Model::Helix helix;

		/*
		 * Generating a plane between the helices, we discard bases on the wrong side of it
		 * then rotate the upcoming helix in such a way that the last non-discarded bases can be connected
		 */

		/*class Control : public CreateBaseControl {
		public:
			 *
			 * We need the coordinates given as 'translation' to be in a common space between helices
			 *

			virtual bool generateSharedCoordinates() const {
				return true;
			}

			virtual bool operator() (const MVector & translation, int index, bool forward) const {
				std::cerr << "Intersection: " << (has_prev_plane ? "does have a previous plane " : "does NOT have a previous plane ") << (has_next_plane ? "does have a next plane " : "does NOT have a next plane ") << std::endl;
				
				if (has_prev_plane) {
					if (prev_plane.directedDistance(translation) > 0.0) {
						std::cerr << "Base at index: " << index << " of the " << (forward ? "forward" : "backward") << " strand at " << translation.x << ", " << translation.y << ", " << translation.z << " is discarded by the prev_plane" << std::endl;
						return false;
					}
				}

				if (has_next_plane) {
					if (next_plane.directedDistance(translation) < 0.0) {
						std::cerr << "Base at index: " << index << " of the " << (forward ? "forward" : "backward") << " strand at " << translation.x << ", " << translation.y << ", " << translation.z << " is discarded by the next_plane" << std::endl;
						return false;
					}
				}

				return true;
			}

			inline Control() : has_next_plane(false) {

			}

			bool has_prev_plane, has_next_plane;
			MPlane prev_plane, next_plane;
		} control;*/

		/*
		 * Stores the chosen end bases for later connection
		 */

		std::list<std::pair<Model::Base, Model::Base> > interHelixBases;

		for(unsigned int i = 0; i < points.length() - 1; ++i) {
			MVector origo(points[i]), end(points[i + 1]);

			/*
			 * Calculate the perpendicular plane between this and the upcoming helix
			 */

			/*std::cerr << "Section" << std::endl;

			control.prev_plane = control.next_plane;
			control.has_prev_plane = control.has_next_plane;
			control.has_next_plane = i < points.length() - 2;

			if (control.has_next_plane) {
				MVector next_end(points[i + 2]),
						* The current helix direction *
						current_dir = (end - origo).normal(),
						* The upcoming helix direction *
						next_dir = (next_end - end).normal(),
						* The normal of the plane cut between the helices *
						u = current_dir ^ next_dir,
						* Together, u and v defines the plane *
						v = (u ^ current_dir) + (u ^ next_dir),
						normal = u ^ v;

				 * The normal must be in the positive direction of the current_dir and next_dir
				 * we cannot however multiply it with the dot product directly as it could be really small or zero.
				 * the sgn_nozero returns -1 for negative and 1 for zero *or* positive *
				normal *=  sgn_nozero(normal * current_dir);

				control.next_plane.setPlane(normal.x, normal.y, normal.z, - normal * end);

				std::cerr << "Next plane: normal: " << normal.x << ", " << normal.y << ", " << normal.z << std::endl;
			}*/


			if (!(status = createHelix(origo, end, current_rotation, helix/*, control*/))) {
				status.perror(MString("createHelix at index: ") + i);
				return status;
			}

			/*
			 * Calculate a new rotation for the next helix
			 */

			// FIXME

			current_rotation += floor((end - origo).length() / DNA::STEP + 0.5) * DNA::PITCH;

			/*
			 * A new color for the upcoming helix
			 */

			if (!(status = randomizeMaterials())) {
				status.perror("randomizeMaterials");
				return status;
			}
		}

		return MStatus::kSuccess;
	}

	/*
	 * Helper method
	 */

	template<typename ElementT>
	bool GetArgument(const MArgDatabase & args, const char *arg, ElementT & element) {
		MStatus status;

		if (args.isFlagSet(arg, &status)) {
			if (!(status = args.getFlagArgument(arg, 0, element))) {
				status.perror("MArgDatabase::getFlagArgument");
				return false;
			}

			return true;
		}

		return false;
	}

	template<>
	bool GetArgument<MVector>(const MArgDatabase & args, const char *arg, MVector & element) {
		MStatus status;
		
		if (args.isFlagSet(arg, &status)) {
			for(int i = 0; i < 3; ++i) {
				if (!(status = args.getFlagArgument(arg, i, element[i]))) {
					status.perror("MArgDatabase::getFlagArgument");
					return false;
				}
			}

			return true;
		}

		return false;
	}

	MStatus Creator::doIt(const MArgList & args) {
		int bases = default_bases;
		MVector origo(0, 0, 0), end, direction;
		double rotation = 0.0;
		MObjectArray nurbsSurfaces, selectedObjects;

		// When deciding upon what createHelix command to call, these will be used

		bool hasEnd = false, hasDirection = false, noCurve = false;

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		/*if (argDatabase.isFlagSet("-b", &status)) {
			if (!(status = argDatabase.getFlagArgument("-b", 0, bases))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}*/

		noCurve = argDatabase.isFlagSet("-nc", &status);

		if (!status) {
			status.perror("MArgDatabase::isFlagSet");
			return status;
		}

		GetArgument(argDatabase, "-b", bases);

		/*
		 * This won't fail if the user didn't supply the -b argument, as bases will be equal to default_bases in this case
		 */

		if (bases <= 0) {
			MGlobal::displayError("No valid number of bases given");
			return MStatus::kFailure;
		}

		/*
		 * Look for the arguments for the other versions of createHelix
		 */

		/*if (argDatabase.isFlagSet("-b", &status)) {
			if (!(status = argDatabase.getFlagArgument("-b", 0, bases))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}*/

		GetArgument(argDatabase, "-o", origo);
		hasEnd = GetArgument(argDatabase, "-t", end);
		hasDirection = GetArgument(argDatabase, "-d", direction);
		GetArgument(argDatabase, "-r", rotation);

		if (!(status = ArgList_GetObjects(args, syntax(), "-c", nurbsSurfaces))) {
			if (status != MStatus::kNotFound) {
				status.perror("ArgList_GetObjects");
				return status;
			}
		}

		/*
		 * Pick two materials by random
		 */
			
		if (!(status = randomizeMaterials())) {
			status.perror("randomizeMaterials");
			return status;
		}

		/*
		 * Create the helix with the set materials and the given number of bases
		 */

		m_redoMode = CREATE_NONE;
		m_redoData.bases = bases;
		m_redoData.rotation = rotation;

		if (nurbsSurfaces.length() > 0) {
			/*
			 * Create helices along some CV curves
			 */

			m_redoData.points.reserve(nurbsSurfaces.length());

			for(unsigned int i = 0; i < nurbsSurfaces.length(); ++i) {
				MPointArray pointArray;

				if (!(status = CVCurveFromObject(nurbsSurfaces[i], pointArray))) {
					status.perror("CVCurveFromObject");
					return status;
				}

				m_redoData.points.push_back(pointArray);

				if (!(status = createHelix(pointArray, rotation))) {
					status.perror("createHelix(pointArray, rotation)");
					return status;
				}
			}

			m_redoMode = CREATE_ALONG_CURVE;
		}
		else if (hasEnd) {
			/*
			 * Create a helix between origo and the end points
			 */

			m_redoMode = CREATE_BETWEEN;
			m_redoData.end = end;
			m_redoData.origo = origo;

			return createHelix(origo, end, rotation);
		}
		else if (hasDirection) {
			/*
				* Create a helix from origo in the given direction
				*/

			m_redoMode = CREATE_ORIENTED;
			m_redoData.direction = direction;
			m_redoData.origo = origo;

			return createHelix(origo, direction, bases, rotation);
		}
		else {
			/*
			 * Look if we have any objects currently selected
			 */

			if (!noCurve) {
				MSelectionList selectionList;

				if (!(status = MGlobal::getActiveSelectionList(selectionList))) {
					status.perror("MGlobal::getActiveSelectionList");
					return status;
				}

				if (selectionList.length() > 0) {
					/*
					 * If any of these is a CV curve..
					 */

					m_redoData.points.reserve(selectionList.length());

					bool anyCurve = false;
					for(unsigned int i = 0; i < selectionList.length(); ++i) {
						MPointArray pointArray;
						MObject object;
					
						if (!(status = selectionList.getDependNode(i, object))) {
							status.perror("MSelectionList::getDependNode");
							return status;
						}

						if (!(status = CVCurveFromObject(object, pointArray))) {
							if (status != MStatus::kNotFound) {
								status.perror("CVCurveFromObject");
								return status;
							}
							else
								continue;
						}

						m_redoData.points.push_back(pointArray);

						if (!(status = createHelix(pointArray, rotation))) {
							status.perror("createHelix(pointArray, rotation)");
							return status;
						}

						anyCurve = true;
					}

					if (anyCurve) {
						m_redoMode = CREATE_ALONG_CURVE;
						return MStatus::kSuccess;
					}
				}
			}

			/*
			 * Just create a normal helix
			 */

			m_redoMode = CREATE_NORMAL;
			MTransformationMatrix matrix;
			double rotation_vector[] = { 0.0, 0.0, rotation };
			
			if (!(status = matrix.setRotation(rotation_vector, MTransformationMatrix::kXYZ))) {
				status.perror("MTransformationMatrix::setRotation");
				return status;
			}

			if (!(status = matrix.addTranslation(origo, MSpace::kTransform))) {
				status.perror("MTransformationMatrix::addTranslation");
				return status;
			}

			return createHelix(bases, matrix.asMatrix());
		}

		return MStatus::kSuccess;
	}

	MStatus Creator::randomizeMaterials() {
		MStatus status;

		Model::Material::Container::size_type numMaterials;
		Model::Material::Iterator materials_begin = Model::Material::AllMaterials_begin(status, numMaterials);

		if (!status) {
			status.perror("Material::AllMaterials_begin");
			return status;
		}

		for(int i = 0; i < 2; ++i)
			m_materials[0] = Model::Material();

		if (numMaterials == 1) {
			for(int i = 0; i < 2; ++i)
				m_materials[i] = *materials_begin;
		}
		else if(numMaterials >= 2) {

			/*
			 * Pick out two by random, not the same ones twice!
			 */

			unsigned int indices[] = { rand() % numMaterials, 0 };
			do { indices[1] = rand() % numMaterials; } while (indices[0] == indices[1]);

			for(int i = 0; i < 2; ++i)
				m_materials[i] = *(materials_begin + indices[i]);
		}

		return MStatus::kSuccess;
	}

	MStatus Creator::undoIt () {
		MStatus status;
		
		/*if (!(status = m_helix.deleteNode())) {
			status.perror("Helix::deleteNode");
			return status;
		}*/

		std::for_each(m_helices.begin(), m_helices.end(), std::mem_fun_ref(&Model::Helix::deleteNode));
		m_helices.clear();

		return MStatus::kSuccess;
	}

	MStatus Creator::redoIt () {
		switch (m_redoMode) {
		case CREATE_NORMAL:
			return createHelix(m_redoData.bases);
		case CREATE_ORIENTED:
			return createHelix(m_redoData.origo, m_redoData.direction, m_redoData.bases, m_redoData.rotation);
		case CREATE_BETWEEN:
			return createHelix(m_redoData.origo, m_redoData.end, m_redoData.rotation);
		case CREATE_ALONG_CURVE:
			for(std::vector<MPointArray>::iterator it = m_redoData.points.begin(); it != m_redoData.points.end(); ++it)
				createHelix(*it, m_redoData.rotation);
		}

		return MStatus::kSuccess;
	}

	bool Creator::isUndoable () const {
		return true;
	}

	bool Creator::hasSyntax () const {
		return true;
	}

	MSyntax Creator::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-b", "-base", MSyntax::kLong);
		syntax.addFlag("-o", "-origo", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
		syntax.addFlag("-t", "-target", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
		syntax.addFlag("-d", "-direction", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
		syntax.addFlag("-r", "-rotation", MSyntax::kDouble);
		syntax.addFlag("-c", "-curve", MSyntax::kString);
		syntax.addFlag("-nc", "-noCurve", MSyntax::kNoArg);
		syntax.makeFlagMultiUse("-c");

		return syntax;
	}

	void *Creator::creator() {
		return new Creator();
	}

	/*
	 * Direct access interfaces used by other libraries
	 */

	MStatus Creator::create(int bases, Model::Helix & helix) {
		MStatus status;
		
		m_redoMode = CREATE_NORMAL;
		m_redoData.bases = bases;
		m_redoData.origo = MVector(0.0, 0.0, 0.0);
		m_redoData.rotation = 0.0;

		if (!(status = randomizeMaterials())) {
			status.perror("randomizeMaterials");
			return status;
		}

		return createHelix(bases, helix);
	}

	MStatus Creator::create(const MVector & origo, int bases, double rotation, Model::Helix & helix) {
		MStatus status;

		m_redoMode = CREATE_NORMAL;
		m_redoData.bases = bases;
		m_redoData.origo = origo;
		m_redoData.rotation = rotation;

		if (!(status = randomizeMaterials())) {
			status.perror("randomizeMaterials");
			return status;
		}

		MTransformationMatrix matrix;
		double rotation_vector[] = { 0.0, 0.0, rotation };
			
		if (!(status = matrix.setRotation(rotation_vector, MTransformationMatrix::kXYZ))) {
			status.perror("MTransformationMatrix::setRotation");
			return status;
		}

		if (!(status = matrix.addTranslation(origo, MSpace::kTransform))) {
			status.perror("MTransformationMatrix::addTranslation");
			return status;
		}

		return createHelix(bases, helix, matrix.asMatrix());
	}

	MStatus Creator::create(const MVector & origo, const MVector & end, double rotation, Model::Helix & helix) {
		MStatus status;

		m_redoMode = CREATE_BETWEEN;
		m_redoData.end = end;
		m_redoData.origo = origo;
		m_redoData.rotation = rotation;

		if (!(status = randomizeMaterials())) {
			status.perror("randomizeMaterials");
			return status;
		}

		return createHelix(origo, end, rotation, helix);
	}
}
