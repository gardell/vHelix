/*
 * Material.h
 *
 *  Created on: 16 feb 2012
 *      Author: johan
 */

#ifndef MODEL_MATERIAL_H_
#define MODEL_MATERIAL_H_

#include <model/Object.h>

#include <maya/MString.h>

#include <vector>

/*
* New code for managing materials, using this code, duplication of material nodes should never occur
* We're also no longer dependent on the DNAshaders.ma file, which makes it so much easier for users to install the plugin
*
*/

#define MEL_SETUP_MATERIALS_COMMAND																							\
	"string $material_sets[];\n"																							\
	"{\n"																													\
	"$materials = `ls -mat \"DNA*\"`;\n"																					\
	"clear($material_sets);\n"																								\
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
	"}\n"																													\
	"ls $material_sets;\n"

/*
* The caDNAno importer creates new materials with the colors extracted from the JSON file. This script creates the material
* Note that before this command, you *must* define the array $color that sets the color of the material and the $name that sets the name of the material
*/

#define MEL_CREATE_MATERIAL_COMMAND																							\
	"\n"																													\
	"$materialNode = `createNode lambert -ss -n $materialName`;\n"															\
	"setAttr ($materialNode + \".dc\") 1;\n"																				\
	"setAttr ($materialNode + \".c\") -type \"float3\" $materialColor[0] $materialColor[1] $materialColor[2];\n"			\
	"connectAttr ($materialNode + \".msg\") \":defaultShaderList1.s\" -na;\n"												\
	"$materialSet = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $materialNode)`;\n"		\
	"connectAttr ($materialNode + \".outColor\") ($materialSet + \".surfaceShader\");\n"									\
	"ls $materialSet;\n"																									\
	"\n"

/*
 * This is a class for managing the color materials attached to bases
 * The material is just identified by a string, but this class contains some additional features for querying and applying them to bases
 */

namespace Helix {
	namespace Model {
		class Base;

		class VHELIXAPI Material {
		public:
			inline Material(const MString & string) : m_material(string) {

			}

			inline Material(const char *string) : m_material(string) {

			}

			inline Material(const Material & material) : m_material(material.m_material) {

			}

			inline Material() {

			}

			inline Material & operator=(const Material & material) {
				m_material = material.m_material;
				return *this;
			}

			inline Material & operator=(const MString & string) {
				m_material = string;

				return *this;
			}

			inline Material & operator=(const char *string) {
				m_material = string;

				return *this;
			}

			inline bool operator==(const Material & material) const {
				return m_material == material.m_material;
			}

			inline bool operator!=(const Material & material) const {
				return !this->operator==(material);
			}

			inline const MString & getMaterial() const {
				return m_material;
			}


			/*
			 * The actually useful methods regarding materials
			 */

			/*
			 * Create a new material and make it available with the AllMaterials*
			 */

			static MStatus Create(const MString & name, float color[3], Material & material);

			static MStatus Find(const MString & name, Material & material);

			typedef std::vector<Material> Container;
			typedef const Material * Iterator;

			static Iterator AllMaterials_begin(MStatus & status, Container::size_type & numMaterials);
			static Iterator AllMaterials_begin(MStatus & status);
			
			static Iterator AllMaterials_end();

			MStatus getColor(float color[3]) const;

			/*
			 * Because setMaterial uses MEL, applying materials so many bases can be really slow. The JSON importer suffers from this
			 * thus, by buffering a list of bases and executing a single MEL command for all of them, performance can be increased
			 */

			class ApplyMaterialToBases {
				friend class Material;
			public:
				MStatus add(Base & base);
				MStatus apply() const;
			protected:
				inline ApplyMaterialToBases(const Material & material) : m_material(material) {

				}

				const Material & m_material;
				MString m_concat;
			};

			inline ApplyMaterialToBases setMaterialOnMultipleBases() const {
				return ApplyMaterialToBases(*this);
			}

		protected:
			MString m_material;
		};
	}
}

#endif /* MATERIAL_H_ */
