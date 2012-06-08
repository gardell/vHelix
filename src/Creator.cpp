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

		/*MFnDagNode helix_dagNode;
		m_helix = helix_dagNode.create(Helix::id, MObject::kNullObj, &status);

		if (!status) {
			status.perror("MFnDagNode::create");
			return status;
		}

		// Do a setAttr m_helix.displayHandle true equivalent
		//

		{
			MPlug displayHandle(m_helix, MPxTransform::displayHandle);

			if (!(status = displayHandle.setBool(true)))
				status.perror("MPlug::setBool");
		}

		// Now generate a helix cylinder
		//

		// This is still easiest to do using MEL for a lot of reasons

		if (!(status = MGlobal::executeCommand(
				MString("$cylinder = `cylinder -radius ") + DNA::RADIUS + " -ch false -heightRatio " + (DNA::STEP * bases / DNA::RADIUS) + (" -name \"cylinderRepresentation\" -axis 0.0 0.0 1.0`;\n"
				"$topCap = `planarSrf -name \"cylinderRepresentation_topCap\" -ch false ($cylinder[0] + \".u[0]\")`;\n"
				"$bottomCap = `planarSrf -name \"cylinderRepresentation_bottomCap\" -ch false ($cylinder[0] + \".u[") + (DNA::STEP * bases) + ("]\")`;\n"
				"parent -relative $topCap[0] $bottomCap[0] $cylinder[0];\n"
				"$parented_cylinder = `parent -relative $cylinder[0] ") + helix_dagNode.fullPathName() + ("`;\n"
				"move -relative 0.0 0.0 ") + (DNA::STEP * bases / 2.0) + "$parented_cylinder[0]\n"
				))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		// Now generate the helix locator node that will display visual information
		//

		MFnDagNode locator_dagNode;
		MObject locator_object = locator_dagNode.create(HelixLocator::id, m_helix, &status);

		if (!status) {
			status.perror("MFnDagNode::create");
			return status;
		}

		// Store the objects here
		MObjectArray all_base_objects[2];

		MDGModifier dgModifier;

		for (int i = 0; i < bases; ++i) {
			MObject base_objects[2];

			// Get the positions for the bases from DNA.h

			MVector basePositions[2];
			if (!(status = DNA::CalculateBasePairPositions(double(i), basePositions[0], basePositions[1]))) {
				status.perror("DNA::CalculateBasePairPositions");
				return status;
			}

			for(int j = 0; j < 2; ++j) {
				// Create the base
				//

				if (!(status = Helix_CreateBase(m_helix,
												MString(DNA::strands[j]) + "_" + ((bases - i) * j + (i + 1) * ((j + 1) % 2)),
												basePositions[j],
												//MVector(DNA::RADIUS * cos(DEG2RAD(DNA::PITCH * i + 180.0 * j)), DNA::RADIUS * sin(DEG2RAD(DNA::PITCH * i + 180.0 * j)), DNA::STEP * i),
												base_objects[j]))) {
					status.perror("Helix_CreateBase");
					return status;
				}

				all_base_objects[j].append(base_objects[j]);

				MProgressWindow::advanceProgress(1);
			}

			if (!(status = dgModifier.connect(base_objects[0], HelixBase::aLabel, base_objects[1], HelixBase::aLabel))) {
				status.perror("MDGModifier::connect");
				return status;
			}
		}

		// Now, connect the bases
		//

		MProgressWindow::setProgress(0);
		MProgressWindow::setProgressStatus("Connecting helix bases");

		for(int i = 0; i < bases - 1; ++i) {
			if (!(status = dgModifier.connect(all_base_objects[0][i + 1], HelixBase::aBackward, all_base_objects[0][i], HelixBase::aForward))) {
				status.perror("MDGModifier:connect");
				return status;
			}

			MProgressWindow::advanceProgress(1);

			if (!(status = dgModifier.connect(all_base_objects[1][i], HelixBase::aBackward, all_base_objects[1][i + 1], HelixBase::aForward))) {
				status.perror("MDGModifier:connect");
				return status;
			}

			MProgressWindow::advanceProgress(1);
		}

		// Finally, realize all the MDGModifier commands

		if (!(status = dgModifier.doIt())) {
			status.perror("MDGModifier::doIt");
			return status;
		}

		MProgressWindow::endProgress();

		// Due to a an issue that didn't exist in the Python/PyMEL API for some reason, there's no material assigned to the bases. So we color them here using the PaintStrand command

		for(int i = 0; i < 2; ++i) {
			PaintStrand paintStrands;
			MDagPathArray endBases;

			endBases.setLength(1);
			if (!(status = MFnDagNode(all_base_objects[i][0]).getPath(endBases[0]))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			if (!(status = paintStrands.paintStrands(endBases))) {
				status.perror("PaintStrands::paintStrands");
			}
		}*/

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

		if (!(status = MGlobal::executeCommand(MEL_TOGGLECYLINDERBASEVIEW_COMMAND " -refresh true"))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		MProgressWindow::endProgress();

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
			
		Model::Material *materials;
		size_t numMaterials;

		if (!(status = Model::Material::GetAllMaterials(&materials, numMaterials))) {
			status.perror("Material::GetAllMaterials");
			return status;
		}

		for(int i = 0; i < 2; ++i)
			m_materials[0] = Model::Material();

		if (numMaterials == 1) {
			for(int i = 0; i < 2; ++i)
				m_materials[i] = materials[0];
		}
		else if(numMaterials >= 2) {

			/*
			 * Pick out two by random, not the same ones twice!
			 */

			unsigned int indices[] = { rand() % numMaterials, 0 };
			do { indices[1] = rand() % numMaterials; } while (indices[0] == indices[1]);

			for(int i = 0; i < 2; ++i)
				m_materials[i] = materials[indices[i]];
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
