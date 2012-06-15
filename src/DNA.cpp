/*
 * DNA.cpp
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#include <DNA.h>

#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>
#include <maya/MDagPathArray.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MItDag.h>

#include <HelixBase.h>
#include <Utility.h>

#include <algorithm>

#define MEL_PARSE_MATERIALS_COMMAND			\
	"$materials = `ls -mat \"DNA*\"`;\n"	\
	"string $colors[];\n"	\
	"for ($material in $materials) {\n"	\
    "$dnaShader = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $material)`;\n"	\
    "connectAttr ($material + \".outColor\") ($dnaShader + \".surfaceShader\");\n"	\
    "$colors[size($colors)] = $dnaShader;\n"	\
	"}\n"	\
	"ls $colors;\n"

namespace DNA {

	MStatus CalculateBasePairPositions(double index, MVector & forward, MVector & backward, double offset, double totalNumBases) {
		/*

x=xstart+(1-sphereRadius)*math.sin(i*(self.PITCH/360.0)*2*math.pi)
z=zstart+(1-sphereRadius)*math.cos(i*(self.PITCH/360.0)*2*math.pi)
y=ystart-self.STEP*numBases/2.0+i*self.STEP+0.165

Där i är basparets index

Och följande för reverse strand:

x=xstart+(1-sphereRadius)*math.sin(((i*self.PITCH+155)/360.0)*2*math.pi)
z=zstart+(1-sphereRadius)*math.cos(((i*self.PITCH+155)/360.0)*2*math.pi)
y=ystart-self.STEP*numBases/2.0+i*self.STEP+0.165

		 */

		double rad = DEG2RAD(offset) + index * DEG2RAD(-PITCH);

		forward.x = ONE_MINUS_SPHERE_RADIUS * sin(rad);
		forward.y = ONE_MINUS_SPHERE_RADIUS * cos(rad);
		forward.z = index * STEP + Z_SHIFT - totalNumBases * STEP / 2;

		rad += DEG2RAD(155.0);

		backward.x = ONE_MINUS_SPHERE_RADIUS * sin(rad);
		backward.y = ONE_MINUS_SPHERE_RADIUS * cos(rad);
		backward.z = index * STEP + Z_SHIFT - totalNumBases * STEP / 2;

		return MStatus::kSuccess;
	}
	
	static MDagPath moleculeModel_dagPath, arrowModel_dagPath;

	// Another helper method

	MStatus GetDagPathFromString(const char *name, MDagPath & dagPath) {
		MStatus status;
		MSelectionList selectionList;

		if (!(status = selectionList.add(name, false))) {
			status.perror("GetDagPathFromString: MSelectionList::add");
			return status;
		}

		MObject backboneArrow_object;

		if (!(status = selectionList.getDagPath(0, dagPath))) {
			status.perror("MSelectionList::getDagPath");
			return status;
		}

		return MStatus::kSuccess;
	}

	// Helper method, load the file and set the moleculeModel and arrowModel

	MStatus LoadMoleculeAndArrowModels() {
		MStatus status;

		// Load the file containing the models
		//

		if (!(status = MFileIO::importFile(HELIXBASE_MODEL_SOURCE_FILES))) {
			status.perror("MFileIO::importFile");

			// Ask the user to select the file containing the source files instead
			//

			bool opened_file = false;
			while (!opened_file) {
				MCommandResult commandResult;
				if (!(status = MGlobal::executeCommand(MString("fileDialog2 -fileFilter \"Maya Files (*.ma *.mb)\" -dialogStyle 2 -caption \"Select the file containing the Helix molecule (Typically called " HELIXBASE_MODEL_SOURCE_FILES ")\" -fileMode 1;"), commandResult))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				MStringArray stringArray;
				if (!(status = commandResult.getResult(stringArray))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				for(unsigned int i = 0; i < stringArray.length(); ++i) {
					if (!(status = MFileIO::importFile(stringArray[i])))
						status.perror(MString("MFileIO::importFile for ") + stringArray[i]);
				}

				// Make sure we actually loaded something useful
				//

				if (!(status = GetDagPathFromString("BackboneArrow", arrowModel_dagPath))) {
					status.perror("GetDagPathFromString");
					continue;
				}

				bool isValid = arrowModel_dagPath.isValid(&status);

				if (!status) {
					status.perror("MDagPath::isValid");
					return status;
				}

				if (!isValid)
					continue;

				opened_file = true;
			}
		}

		// Extract the models
		//

		if (!(status = GetDagPathFromString("BackboneArrow", arrowModel_dagPath))) {
			status.perror("GetDagPathFromString");
			return status;
		}

		{
			MCommandResult commandResult;

			if (!(status = MGlobal::executeCommand(MString("sphere -radius ") + DNA::SPHERE_RADIUS + " -name vHelixMolecule", commandResult))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			MStringArray result;
			if (!(status = commandResult.getResult(result))) {
				status.perror("MCommandResult::getResult");
				return status;
			}

			MSelectionList selectionList;

			if (!(status = selectionList.add(result[0], false))) {
				status.perror("LoadMoleculeAndArrowModels: MSelectionList::add");
				return status;
			}

			MObject molecule_object;

			if (!(status = selectionList.getDagPath(0, moleculeModel_dagPath, molecule_object))) {
				status.perror("MSelectionList::getDagPath main");
				return status;
			}
		}

		/*
		 * Now make the models invisible
		 */

		{
			MFnDagNode molecule_dagNode(moleculeModel_dagPath);

			MObject molecule_visibility_attribute = molecule_dagNode.attribute("visibility", &status);

			if (!status) {
				status.perror("MFnDagNode::attribute 1");
				return status;
			}

			MPlug moleculeVisibility(moleculeModel_dagPath.node(), molecule_visibility_attribute);

			if (!(status = moleculeVisibility.setBool(false))) {
				status.perror("MPlug::setBool 1");
				return status;
			}

			MFnDagNode arrow_dagNode(arrowModel_dagPath);

			MObject arrow_visibility_attribute = arrow_dagNode.attribute("visibility", &status);

			if (!status) {
				status.perror("MFnDagNode::attribute 2");
				return status;
			}

			MPlug arrowVisibility(arrowModel_dagPath.node(), arrow_visibility_attribute);

			if (!(status = arrowVisibility.setBool(false))) {
				status.perror("MPlug::setBool 2");
				return status;
			}
		}

		return MStatus::kSuccess;
	}

	MStatus GetMoleculeModel(MDagPath & result) {
		MStatus status;
		bool valid = moleculeModel_dagPath.isValid(&status);

		if (!status) {
			status.perror("MDagPath::isValid");
			return status;
		}

		if (!valid) {
			if (!(status = LoadMoleculeAndArrowModels())) {
				status.perror("LoadMoleculeAndArrowModels");
				return status;
			}
		}

		result = moleculeModel_dagPath;

		return MStatus::kSuccess;
	}

	MStatus GetArrowModel(MDagPath & result) {
		MStatus status;
		bool valid = arrowModel_dagPath.isValid(&status);

		if (!status) {
			status.perror("MDagPath::isValid");
			return status;
		}

		if (!valid) {
			if (!(status = LoadMoleculeAndArrowModels())) {
				status.perror("LoadMoleculeAndArrowModels");
				return status;
			}
		}

		result = arrowModel_dagPath;

		return MStatus::kSuccess;
	}
}
