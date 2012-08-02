/*
 * Material.cpp
 *
 *  Created on: 16 feb 2012
 *      Author: johan
 */

#include <model/Material.h>
#include <model/Base.h>
#include <model/Strand.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <DNA.h>

#include <algorithm>
#include <iterator>

/*
 * New code for managing materials, using this code, duplication of material nodes should never occur
 * We're also no longer dependent on the DNAshaders.ma file, which makes it so much easier for users to install the plugin
 *
 */

#define MEL_SETUP_MATERIALS_COMMAND																							\
	"$materials = `ls -mat \"DNA*\"`;\n"																					\
	"string $material_sets[];\n"																							\
	"clear($material_sets);\n"																								\
																															\
	"if (size($materials) == 0) {\n"																						\
	"	matrix $colors[10][3] = <<\n"																						\
	"		0, 1, 0.45666122 ;\n"																							\
	"		1, 0.18333334, 0 ;\n"																							\
	"		1, 1, 1 ;\n"																									\
	"		0.75, 0.75, 0.75 ;\n"																							\
	"		0, 0, 1 ;\n"																									\
	"		1, 1, 0 ;\n"																									\
	"		1, 0, 1 ;\n"																									\
	"		0.20863661, 0.20863661, 0.20863661 ;\n"																			\
	"		0, 1, 1 ;\n"																									\
	"		1, 0.12300003, 0.29839987\n"																					\
	"	>>;\n"																												\
																															\
	"	for($i = 0; $i < 10; ++$i) {\n"																						\
	"		$node = `createNode lambert -ss -n (\"DNA\" + $i)`;\n"															\
	"		setAttr ($node + \".dc\") 1;\n"																					\
	"		setAttr ($node + \".c\") -type \"float3\" $colors[$i][0] $colors[$i][1] $colors[$i][2];\n"						\
	"		connectAttr ($node + \".msg\") \":defaultShaderList1.s\" -na;\n"												\
																															\
	"		$materials[size($materials)] = $node;\n"																		\
	"	}\n"																												\
	"}\n"																													\
																															\
	"for ($material in $materials) {\n"																						\
	"	$sets = `listSets -as -t 1`;\n"																						\
																															\
	"	$exists = false;\n"																									\
																															\
	"	for ($set in $sets) {\n"																							\
	"		if ($set == \"SurfaceShader_\" + $material) {\n"																\
	"			$exists = true;\n"																							\
	"			break;\n"																									\
	"		}\n"																											\
	"	}\n"																												\
																															\
	"	string $current_set;\n"																								\
																															\
	"	if (!$exists) {\n"																									\
																															\
	"		$current_set = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $material)`;\n"	\
	"		connectAttr ($material + \".outColor\") ($current_set + \".surfaceShader\");\n"									\
	"	}\n"																												\
	"	else\n"																												\
	"		$current_set = \"SurfaceShader_\" + $material;\n"																\
																															\
	"	$material_sets[size($material_sets)] = $current_set;\n"																\
	"}\n"																													\
																															\
	"ls $material_sets;\n"

/*
 * The caDNAno importer creates new materials with the colors extracted from the JSON file. This script creates the material
 * Note that before this command, you *must* define the array $color that sets the color of the material and the $name that sets the name of the material
 */

#define MEL_CREATE_MATERIAL_COMMAND																							\
	"$materialNode = `createNode lambert -ss -n $materialName`;\n"															\
	"setAttr ($materialNode + \".dc\") 1;\n"																				\
	"setAttr ($materialNode + \".c\") -type \"float3\" $materialColor[0] $materialColor[1] $materialColor[2];\n"			\
	"connectAttr ($materialNode + \".msg\") \":defaultShaderList1.s\" -na;\n"												\
	"$materialSet = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $materialNode)`;\n"		\
	"connectAttr ($materialNode + \".outColor\") ($materialSet + \".surfaceShader\");\n"									\
	"ls $materialSet;\n"

namespace Helix {
	namespace Model {

		std::vector<Material> s_materials;
		//std::vector<std::pair<MString, bool> > Material::s_materialFiles;

		class PaintStrandWithMaterialFunctor {
		public:
			inline PaintStrandWithMaterialFunctor(Material & m, MString *_tokenize) : material(&m), tokenize(_tokenize) {

			}

			MStatus operator()(Base & base) {
				MStatus status;
				MDagPath base_dagPath = base.getDagPath(status);

				if (!status) {
					status.perror("Base::getDagPath");
					return status;
				}

				(*tokenize) += base_dagPath.fullPathName(&status) + " ";

				return status;
			}

		private:
			Material *material;
			MString *tokenize;
		};

		
		Material::Iterator Material::AllMaterials_end() {
			return &s_materials[0] + s_materials.size();
		}

		Material::Iterator Material::AllMaterials_begin(MStatus & status, Material::Container::size_type & numMaterials) {
			Iterator begin = AllMaterials_begin(status);
			numMaterials = s_materials.size();
			return begin;
		}
		
		Material::Iterator Material::AllMaterials_begin(MStatus & status) {
			MCommandResult commandResult;
			MStringArray materialNames;

			s_materials.clear();

			/*
			 * New code: All we need to do is execute the above command. The result will be a string array of the materials available
			 */

			if (!(status = MGlobal::executeCommand(MEL_SETUP_MATERIALS_COMMAND, commandResult))) {
				status.perror("MGlobal::executeCommand");
				return AllMaterials_end();
			}

			if (!(status = commandResult.getResult(materialNames))) {
				status.perror("MCommandResult::getResult");
				return AllMaterials_end();
			}

			s_materials.reserve(materialNames.length());
			std::copy(&materialNames[0], &materialNames[0] + materialNames.length(), std::back_insert_iterator<Container>(s_materials));

			return &s_materials[0];
		}

		MStatus Material::getColor(float color[3]) const {
			MStatus status;
			MObject material_object;

			{
				MSelectionList list;
				if (!(status = list.add(m_material))) {
					status.perror("MSelectionList::add");
					return status;
				}

				if (!(status = list.getDependNode(0, material_object))) {
					status.perror("MSelectionList::getDependNode");
					return status;
				}
			}

			MFnDependencyNode material_dependencyNode(material_object);

			MPlug surfaceShader_plug = material_dependencyNode.findPlug("surfaceShader", &status);

			if (!status) {
				status.perror("MFnDependencyNode::findPlug");
				return status;
			}

			MPlugArray surfaceShader_attachments;

			if (!surfaceShader_plug.connectedTo(surfaceShader_attachments, true, false, &status)) {
				std::cerr << "The outColor is not attached as a destination, thus we cannot find the color attribute" << std::endl;
				return MStatus::kFailure;
			}

			for(unsigned int i = 0; i < surfaceShader_attachments.length(); ++i) {
				MFnDependencyNode attachment_dependencyNode(surfaceShader_attachments[i].node());

				MPlug color_plug = attachment_dependencyNode.findPlug("color", &status);

				if (!status) {
					status.perror("MFnDependencyNode::findPlug");
					continue;
				}

				color[0] = color_plug.child(0).asFloat();
				color[1] = color_plug.child(1).asFloat();
				color[2] = color_plug.child(2).asFloat();

				return MStatus::kSuccess;
			}

			std::cerr << "Couldn't find a color attribute" << std::endl;

			return MStatus::kFailure;
		}

		MStatus Material::Create(const MString & name, float color[3], Material & material) {
			MCommandResult result;
			MStringArray materialName;
			MStatus status;

			if (!(status = MGlobal::executeCommand(MString("float $materialColor[] = { (float) ") + color[0] + ", (float) " + color[1] + ", (float) " + color[2] + " };\nstring $materialName = \"" + name + "\";\n" + MEL_CREATE_MATERIAL_COMMAND, result))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			if (!(status = result.getResult(materialName))) {
				status.perror("MCommandResult::getResult");
				return status;
			}

			std::cerr << "Created new material named: " << materialName[0].asChar() << std::endl;

			material.m_material = materialName[0];

			return MStatus::kSuccess;
		}

		MStatus Material::ApplyMaterialToBases::add(Base & base) {
			MStatus status;

			m_concat += " " + base.getDagPath(status).fullPathName();

			return status;
		}

		MStatus Material::ApplyMaterialToBases::apply() const {
			MStatus status;

			if (!(status = MGlobal::executeCommand(MString("sets -noWarnings -forceElement ") + m_material.getMaterial() + m_concat))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			return MStatus::kSuccess;
		}
	}
}
