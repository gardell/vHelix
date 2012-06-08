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
		static const double ONE_MINUS_SPHERE_RADIUS = (1.0 - SPHERE_RADIUS);

		forward.x = ONE_MINUS_SPHERE_RADIUS * sin(rad);
		forward.y = ONE_MINUS_SPHERE_RADIUS * cos(rad);
		forward.z = index * STEP + 0.165 - totalNumBases * STEP / 2;

		rad += DEG2RAD(155.0);

		backward.x = ONE_MINUS_SPHERE_RADIUS * sin(rad);
		backward.y = ONE_MINUS_SPHERE_RADIUS * cos(rad);
		backward.z = index * STEP + 0.165 - totalNumBases * STEP / 2;

		return MStatus::kSuccess;
	}

	Names OppositeBase(Names d) {
		switch(d) {
		case A:
			return T;
		case T:
			return A;
		case G:
			return C;
		case C:
			return G;
		default:
			return Invalid;
		}
	}

	const char ToChar(Names d) {
		static const char chars[] = { "ATGC?" };

		return chars[(int) d];
	}

	Names ToName(char c) {
		switch(c) {
		case 'A':
		case 'a':
			return A;
		case 'T':
		case 't':
			return T;
		case 'G':
		case 'g':
			return G;
		case 'C':
		case 'c':
			return C;
		default:
			return Invalid;
		}
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

		/*{
			MSelectionList selectionList;

			if (!(status = selectionList.add("BackboneArrow", false))) {
				status.perror("MSelectionList::add");
				return status;
			}

			MObject backboneArrow_object;

			if (!(status = selectionList.getDagPath(0, arrowModel_dagPath, backboneArrow_object))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}
		}*/

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

	/*void SetMoleculeModel(MDagPath & dagPath) {
		moleculeModel_dagPath = dagPath;
	}

	void SetArrowModel(MDagPath & dagPath) {
		arrowModel_dagPath = dagPath;
	}*/

	MStringArray g_materialNames;
	//MStringArray g_materials;

	bool ParseMaterials(MStatus & status) {
		MCommandResult commandResult;

		g_materialNames.clear();

		if (!(status = MGlobal::executeCommand(MEL_PARSE_MATERIALS_COMMAND, commandResult))) {
			status.perror("MGlobal::executeCommand 2");
			return status;
		}

		if (!(status = commandResult.getResult(g_materialNames))) {
			status.perror("MCommandResult::getResult");
			return status;
		}

		return g_materialNames.length() > 0;
	}

	MStatus GetMaterials(MStringArray & result) {
		//MCommandResult commandResult;
		MStatus status;

		// New Maya C++ API approach should be faster DOESNT WORK
		//

		/*MObjectArray materials;

		MItDag itSets(MItDag::kDepthFirst, MFn::kSet, &status);

		if (!status) {
			status.perror("MItDag::#ctor");
			return status;
		}

		for (; !itSets.isDone(); itSets.next()) {
			MDagPath set_dagPath;

			if (!(status = itSets.getPath(set_dagPath))) {
				status.perror("MItDag::getPath");
				return status;
			}

			MFnDagNode set_dagNode(set_dagPath);

			std::cerr << "Found set, fullpath: " << set_dagPath.fullPathName().asChar() << ", name: " << set_dagNode.name().asChar() << std::endl;


			if (strstr(set_dagNode.name().asChar(), "SurfaceShader_DNA") == set_dagNode.name().asChar()) {
				std::cerr << "The set is a DNA material!" << std::endl;

				materials.append(set_dagNode.object(&status));

				if (!status) {
					status.perror("MFnDagNode::object");
					return status;
				}
			}
		}

		if (materials.length() == 0) {
			// Load the material file
			//

			if (!(status = MFileIO::importFile(DNASHADERS_SOURCE_FILE))) {
				status.perror("MFileIO::importFile");
				return status;
			}

			// Parse the materials, unfortenately a lot easier with MEL

			MCommandResult commandResult;

			if (!(status = MGlobal::executeCommand(MEL_PARSE_MATERIALS_COMMAND, commandResult))) {
				status.perror("MGlobal::executeCommand 2");
				return status;
			}

			MStringArray materialNames;

			if (!(status = commandResult.getResult(materialNames))) {
				status.perror("MCommandResult::getResult");
				return status;
			}

			MSelectionList selectionList;

			for(unsigned int i = 0; i < materialNames.length(); ++i) {
				if (!(status = selectionList.add(materialNames[i]))) {
					status.perror("MSelectionList::add");
					return status;
				}
			}

			for(unsigned int i = 0; i < materialNames.length(); ++i) {
				MDagPath dagPath;
				MObject object;
				if (!(status = selectionList.getDagPath(i, dagPath, object))) {
					status.perror("MSelectionList::getDagPath");
					g_materials.append(object);
				}
			}

			// Lookup the material names into material objects for faster usage later

			"$materials = `ls -mat \"DNA*\"`;\n"	\
				"string $colors[];\n"	\
				"for ($material in $materials) {\n"	\
			    "$dnaShader = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $material)`;\n"	\
			    "connectAttr ($material + \".outColor\") ($dnaShader + \".surfaceShader\");\n"	\
			    "$colors[size($colors)] = $dnaShader;\n"	\
				"}\n"	\
				"ls $colors;\n"*/

			/*MItDag itMats(MItDag::kDepthFirst, MFn::kMaterial, &status);

			if (!status) {
				status.perror("MItDag::#ctor");
				return status;
			}

			for(; &itMats.isDone(); itMats.next()) {
				MDagPath mat_dagPath;

				if (!(status = itMats.getPath(mat_dagPath))) {
					status.perror("MItDag::getPath");
					return status;
				}

				MFnDagNode mat_dagNode(mat_dagPath);

				if (strstr(mat_dagNode.name().asChar(), "DNA") == mat_dagNode.name().asChar()) {
					std::cerr << "Found DNA material" << std::endl;
				}

				// Unfortenately, it's still easier to use MEL, there seems to be some missing properties with the C++ create command

				MCommandResult setsCommandResult;

				if (!(status = MGlobal::executeCommand(MString("sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_") + mat_dagNode.name() + "\")", setsCommandResult))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				MString setsName;

				if (!(status = setsCommandResult.getResult(setsName))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				std::cerr << "Created the set " << setsName << std::endl;

				MSelectionList selectionList;

				if (!(status = selectionList.add(setsName))) {
					status.perror("MSelectionList::add");
					return status;
				}

				MDagPath sets_dagPath;
				MObject sets_object;

				if (!(status = selectionList.getDagPath(0, sets_dagPath, sets_object))) {
					status.perror("MSelectionList::getDagPath");
					return status;
				}

				// Back to using the C++ API

				MDagModifier dagModifier;

				dagModifier.connect(mat_dagPath.node(), )
			}
			// Translate names into
		}
		else {
			g_materials.clear();

			for(unsigned int i = 0; i < materials.length(); ++i)
				g_materials.append(materials[i]);
		}*/

		// Search to see if they're already loaded
		//

		MCommandResult commandResult;

		if (!(status = MGlobal::executeCommand("ls -mat \"DNA*\"", commandResult))) {
			status.perror("MGlobal::executeCommand 1");
			return status;
		}

		MStringArray materialNames;

		if (!(status = commandResult.getResult(materialNames))) {
			status.perror("MCommandResult::getResult");
			return status;
		}

		if (materialNames.length() == 0) {
			// Load the material file
			//

			bool opened_file = false;

			if (!(status = MFileIO::importFile(DNASHADERS_SOURCE_FILE))) {
				status.perror("MFileIO::importFile");

				// Ask the user to select the file containing the source files instead
				//

				while (!opened_file) {
					MCommandResult commandResult;
					if (!(status = MGlobal::executeCommand(MString("fileDialog2 -fileFilter \"Maya Files (*.ma *.mb)\" -dialogStyle 2 -caption \"Select the file containing the Helix molecule materials (Typically called " DNASHADERS_SOURCE_FILE ")\" -fileMode 1;"), commandResult))) {
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

					// Parse the materials

					bool parseMaterialsStatus = ParseMaterials(status);

					if (!status) {
						status.perror("ParseMaterials");
						return status;
					}

					if (!parseMaterialsStatus)
						continue;

					opened_file = true;
				}
			}


			if (!opened_file) {// Parse the materials

				ParseMaterials(status);

				if (!status) {
					status.perror("ParseMaterials");
					return status;
				}
			}
		}

		result = g_materialNames;

		return MStatus::kSuccess;

		/*result.clear();
		for(unsigned int i = 0; i < g_materials.length(); ++i)
			result.append(g_materials[i]);

		return MStatus::kSuccess;*/
	}

	MStatus AssignMaterialToObjects(const MDagPathArray & dagPaths, const MString & material) {
		MStatus status;
		MString tokenize;//, material_fullPathName;

		unsigned int dagPaths_length = dagPaths.length();

		//material_fullPathName = material.fullPathName(&status);

		if (!status) {
			status.perror("MDagPath::fullPathName");
			return status;
		}

		// Generate a string containing all the objects full path names separated by a space
		//

		for(unsigned int i = 0; i < dagPaths_length; ++i) {
			tokenize += dagPaths[i].fullPathName(&status) + " ";

			if (!status) {
				status.perror("MDagPathArray[i]::fullPathName");
				return status;
			}
		}

		// Assign materials to selection, even though it is MEL it is just a single command
		//

		//std::cerr << "ExecuteCommand: \"" << (MString("sets -forceElement ") + material + " " + tokenize).asChar() << "\"" << std::endl;

		if (!(status = MGlobal::executeCommand(MString("sets -noWarnings -forceElement ") + material + " " + tokenize))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		// Faster native code doesn't work currently

		/*MFnSet material_set(material);

		for(size_t i = 0; i < dagPaths_length; ++i) {
			if (!(status = material_set.addMember(dagPaths[i].node()))) {
				status.perror("MFnSet::addMember");
				return status;
			}
		}*/

		return MStatus::kSuccess;
	}

	MStatus AssignMaterialsToObjects(const MDagPathArray & dagPaths, const MStringArray & materials) {
		MStatus status;

		MString tokenize;

		size_t dagPaths_length = dagPaths.length();

		if (!status) {
			status.perror("MDagPath::fullPathName");
			return status;
		}

		// Generate a string containing all the commands
		//

		for(unsigned int i = 0; i < dagPaths_length; ++i) {
			if (materials[i].length() == 0)
				continue;

			tokenize += MString("sets -forceElement ") + materials [i] + " " + dagPaths[i].fullPathName(&status) + ";\n";

			if (!status) {
				status.perror("MDagPathArray[i]::fullPathName");
				return status;
			}
		}

		// Assign materials to selection, even though it is MEL it is just a single command
		//

		std::cerr << "ExecuteCommand: \"" << tokenize.asChar() << "\"" << std::endl;

		if (!(status = MGlobal::executeCommand(tokenize))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus QueryMaterialOfObject(MObject base, MString & result) {
		MStatus status;

		// If this is not a base get the parent and assume it is one

		MFnDagNode base_dagNode(base);

		bool isHelixBase = base_dagNode.typeId(&status) == Helix::HelixBase::id;

		if (!status) {
			status.perror("MFnDagNode::typeId");
			return status;
		}

		MObject target = MObject::kNullObj;

		if (!isHelixBase) {
			if (base_dagNode.parentCount(&status) == 0) {
				std::cerr << "No parent!" << std::endl;
				return MStatus::kFailure;
			}

			if (!status) {
				status.perror("MFnDagNode::parentCount");
				return status;
			}

			MObject parent_object = base_dagNode.parent(0, &status);

			if (!status) {
				status.perror("MFnDagNode::parent");
				return status;
			}

			MFnDagNode parent_dagNode(parent_object);

			if (parent_dagNode.typeId(&status) == Helix::HelixBase::id) {
				target = parent_object;
			}
			else {
				std::cerr << "Parent is not a HelixBase either" << std::endl;
				return MStatus::kFailure;
			}

			if (!status) {
				status.perror("MFnDagNode::typeId 2");
				return status;
			}
		}
		else
			target = base;

		// There is no way to query in wich material set the base belongs to, we need to query for all our sets
		MFnDagNode target_dagNode(target);
		MDagPath target_dagPath;
		MStringArray materials;

		if (!(status = target_dagNode.getPath(target_dagPath))) {
			status.perror("MFnDagNode::getPath");
			return status;
		}

		if (!(status = GetMaterials(materials))) {
			status.perror("GetMaterials");
			return status;
		}

		MString fullPathName = target_dagNode.fullPathName(&status);

		if (!status) {
			status.perror("MFnDagNode::fullPathName");
			return status;
		}

		for(unsigned int i = 0; i < materials.length(); ++i) {
			// Is this material assigned?

			bool isMember;
			if (!(status = Helix::HelixBase_isMemberOfSet(target_dagPath, materials[i], isMember))) {
				status.perror("HelixBase_isMemberOfSet");
				return status;
			}

			if (isMember) {
				result = materials[i];
				return MStatus::kSuccess;
			}

			/*MCommandResult commandResult;
			int isMember = 0;



			if (!(status = MGlobal::executeCommand(MString("sets -isMember ") + materials[i] + " " + fullPathName, commandResult))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			if (!(status = commandResult.getResult(isMember))) {
				status.perror("MCommandResult::getResult");
				return status;
			}

			if (isMember) {
				std::cerr << "Weeii, the object \"" << fullPathName << "\" is part of the set \"" << materials[i] << "\"" << std::endl;
				result = materials [i];
				return MStatus::kSuccess;
			}
			else {
				std::cerr << "The object \"" << fullPathName << "\" is NOT part of the set \"" << materials[i] << "\"" << std::endl;
			}*/
		}

		result = "";
		return MStatus::kSuccess;
	}

	MStatus QueryMaterialsOfObjects(const MDagPathArray & objects, MStringArray & colors) {
		colors.setLength(objects.length());

		MStatus status;

		for(unsigned int i = 0; i < objects.length(); ++i) {
			if (!(status = QueryMaterialOfObject(objects[i].node(&status), colors[i]))) {
				status.perror("QueryMaterialOfObject");
				return status;
			}
		}

		return MStatus::kSuccess;
	}
}
