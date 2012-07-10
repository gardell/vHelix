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

namespace Helix {
	Creator::Creator() {

	}

	Creator::~Creator() {

	}

	int Creator::default_bases = DNA::CREATE_DEFAULT_NUM_BASES;

	MStatus Creator::createHelix(int bases, double rotation, const MVector & origo, Model::Helix & helix) {
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
		 * Rotate and translate the helix
		 */

		{
			MFnTransform helix_transform(helix.getDagPath(status));

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			if (!(status = helix_transform.setTranslation(origo, MSpace::kTransform))) {
				status.perror("MFnTransform::setTranslation");
				return status;
			}

			static double rotation_xyz[] = { 0.0, 0.0, DEG2RAD(rotation) };

			if (!(status = helix_transform.setRotation(rotation_xyz, MTransformationMatrix::kXYZ, MSpace::kTransform))) {
				status.perror("MFnTransform::setRotation");
				return status;
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

			for(int j = 0; j < 2; ++j) {
				MString strand(DNA::GetStrandName(j));

				if (!(status = Model::Base::Create(helix, strand + "_" + (i + 1), basePositions[j], base_objects[j]))) {
					status.perror("Base::Create");
					return status;
				}

				if (!(status = base_objects[j].setMaterial(m_materials[j]))) {
					status.perror("Base::setMaterial");
					return status;
				}

				MProgressWindow::advanceProgress(1);
			}

			/*
			 * Now connect the two base pairs
			 */

			if (!(status = base_objects[0].connect_opposite(base_objects[1], true))) {
				status.perror("Base::connect_opposite");
				return status;
			}

			/*
			 * Now connect the previous bases to the newly created ones
			 */

			if (last_base_objects[0]) {
				if (!(status = last_base_objects[0].connect_forward (base_objects[0], true))) {
					status.perror("Base::connect_forward 0");
					return status;
				}

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

		if (!(status = Model::Helix::RefreshCylinderOrBases()))
			status.perror("Helix::RefreshCylinderOrBases");

		MProgressWindow::endProgress();

		/*
		 * Select the newly created Helix
		 */

		if (!(status = Model::Object::Select(&helix, &helix + 1)))
			status.perror("Object::Select");

		m_helices.push_back(helix);

		return MStatus::kSuccess;
	}

	MStatus Creator::createHelix(const MVector & origo, const MVector & end, double rotation, Model::Helix & helix) {
		MVector distance = end - origo, direction = distance.normal(), end_origo = origo + distance / 2.0;

		return createHelix(end_origo, direction, (int) floor(distance.length() / DNA::STEP + 0.5), rotation); // There's no round() in the C++ STD!
	}

	MStatus Creator::createHelix(const MVector & origo, const MVector & direction, int bases, double rotation, Model::Helix & helix) {
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

		if (!(status = createHelix(bases, rotation, origo, helix))) {
			status.perror("createHelix(bases, helix)");
			return status;
		}

		MFnTransform helix_transform(helix.getDagPath(status));

		if (!status) {
			status.perror("Helix::getDagPath");
			return status;
		}

		/*
		 * Now rotate along the Z-axis with the given rotation
		 */

		double rotation_vector[] = { 0.0, 0.0, DEG2RAD(rotation) };

		if (!(status = helix_transform.setRotation(rotation_vector, MTransformationMatrix::kXYZ, MSpace::kTransform))) {
			status.perror("MFnTransform::rotateBy 2");
			return status;
		}

		if (!(status = helix_transform.rotateBy(quaternion1, MSpace::kTransform))) {
			status.perror("MFnTransform::setRotation");
			return status;
		}

		if (!(status = helix_transform.rotateBy(quaternion2, MSpace::kTransform))) {
			status.perror("MFnTransform::rotateBy 1");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Creator::createHelix(const MPointArray & points, double rotation) {
		MStatus status;

		double current_rotation = rotation;
		Model::Helix helix;

		for(unsigned int i = 0; i < points.length() - 1; ++i) {
			MVector origo(points[i]), end(points[i + 1]);
			if (!(status = createHelix(origo, end, current_rotation, helix))) {
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

			return createHelix(bases, rotation, origo);
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
}
