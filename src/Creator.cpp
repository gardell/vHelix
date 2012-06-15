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

namespace Helix {
	Creator::Creator() : m_helix(MObject::kNullObj) {

	}

	Creator::~Creator() {

	}

	int Creator::default_bases = DNA::CREATE_DEFAULT_NUM_BASES;

	MStatus Creator::createHelix(int bases) {
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

		if (!(status = Model::Helix::Create("vHelix", MMatrix::identity, m_helix))) {
			status.perror("Helix::Create");
			return status;
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
				MString strand(DNA::strands[j]);

				if (!(status = Model::Base::Create(m_helix, strand + "_" + (i + 1), basePositions[j], base_objects[j]))) {
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

			if (!(status = base_objects[0].connect_opposite(base_objects[1]))) {
				status.perror("Base::connect_opposite");
				return status;
			}

			/*
			 * Now connect the previous bases to the newly created ones
			 */

			if (last_base_objects[0]) {
				if (!(status = last_base_objects[0].connect_forward (base_objects[0]))) {
					status.perror("Base::connect_forward 0");
					return status;
				}

				if (!(status = base_objects[1].connect_forward (last_base_objects[1]))) {
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

		if (!(status = m_helix.setCylinderRange(0.0, DNA::STEP * (bases - 1))))
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

		if (!(status = Model::Object::Select(&m_helix, &m_helix + 1)))
			status.perror("Object::Select");

		return MStatus::kSuccess;
	}

	MStatus Creator::doIt(const MArgList & args) {
		int bases = default_bases;

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-b", &status)) {
			if (!(status = argDatabase.getFlagArgument("-b", 0, bases))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}

		if (bases <= 0) {
			MGlobal::displayError("No valid number of bases given");
			return MStatus::kFailure;
		}

		/*
		 * Pick two materials by random
		 */
			
		//Model::Material *materials;
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

		/*
		 * Create the helix with the set materials and the given number of bases
		 */

		return createHelix(bases);
	}

	MStatus Creator::undoIt () {
		MStatus status;
		
		if (!(status = m_helix.deleteNode())) {
			status.perror("Helix::deleteNode");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Creator::redoIt () {
		return createHelix(default_bases);
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

		return syntax;
	}

	void *Creator::creator() {
		return new Creator();
	}
}
