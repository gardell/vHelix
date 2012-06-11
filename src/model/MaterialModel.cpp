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

#include <DNA.h>

/*
 * Notice that this code is also defined in the old DNA.cpp. So in case it changes before this file becomes the main version..
 */

#define MEL_PARSE_MATERIALS_COMMAND			\
	"$materials = `ls -mat \"DNA*\"`;\n"	\
	"string $colors[];\n"	\
	"for ($material in $materials) {\n"	\
    "$dnaShader = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $material)`;\n"	\
    "connectAttr ($material + \".outColor\") ($dnaShader + \".surfaceShader\");\n"	\
    "$colors[size($colors)] = $dnaShader;\n"	\
	"}\n"	\
	"ls $colors;\n"

#define MEL_MATERIALS_EXIST_COMMAND			\
	"ls -mat \"DNA*\";"

namespace Helix {
	namespace Model {
		std::vector<Material> Material::s_materials;
		std::vector<std::pair<MString, bool> > Material::s_materialFiles;

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

		MStatus Material::GetAllMaterials(Material **materials, size_t & numMaterials) {
			MStatus status;
			MCommandResult commandResult;
			MStringArray materialNames;

			/*
			 * Make sure the materials exist
			 */

			{
				if (!(status = MGlobal::executeCommand(MEL_MATERIALS_EXIST_COMMAND, commandResult))) {
					status.perror("MGlobal::executeCommand 1");
					return status;
				}

				MStringArray result;

				if (!(status = commandResult.getResult(result))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				if (result.length() == 0) {
					s_materials.clear();
					ResetMaterialFilesLoaded();
				}
			}

			while(s_materials.size() == 0) {
				CacheMaterials();

				/*
				 * Find the materials, if we can't find any, let the loop spin
				 */

				if (!(status = MGlobal::executeCommand(MEL_PARSE_MATERIALS_COMMAND, commandResult))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				if (!(status = commandResult.getResult(materialNames))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				s_materials.clear();
				s_materials.reserve(materialNames.length());

				for(unsigned int i = 0; i < materialNames.length(); ++i)
					s_materials.push_back(Material(materialNames[i]));
			}

			*materials = &s_materials[0];
			numMaterials = s_materials.size();

			return MStatus::kSuccess;
		}

		/*
		 * Helper method
		 */

		MStatus UserImportFile(MString title) {
			MStatus status;
			MCommandResult commandResult;
			if (!(status = MGlobal::executeCommand(MString("fileDialog2 -fileFilter \"Maya Files (*.ma *.mb)\" -dialogStyle 2 -caption \"" + title + "\" -fileMode 1;"), commandResult))) {
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
					status.perror(MString("Failed to open the file \"") + stringArray[i] + "\"");
			}

			return MStatus::kSuccess;
		}

		MStatus Material::CacheMaterials() {
			MStatus status;

			bool empty = true;

			for(std::vector<std::pair<MString, bool> >::iterator it = s_materialFiles.begin(); it != s_materialFiles.end(); ++it) {
				if (!it->second) {
					empty = false;

					if (!(status = MFileIO::importFile(it->first))) {
						status.perror(MString("Failed to open the file \"") + it->first + "\". Asking the user for the file to open");

						if (!(status = UserImportFile(MString("Can't locate the file \"") + it->first + "\""))) {
							status.perror("Failed to open user requested file");
							return status;
						}
					}

					it->second = true;
				}
			}

			if (empty) {
				if (!(status = UserImportFile("No material files could be found. Please select a file containing suitable materials")))
					return MStatus::kFailure;
			}

			return MStatus::kSuccess;
		}

		void Material::ResetMaterialFilesLoaded() {
			for(std::vector<std::pair<MString, bool> >::iterator it = s_materialFiles.begin(); it != s_materialFiles.end(); ++it) {
				it->second = false;
			}
		}

		Material::RegisterMaterialFile defaultMaterialFile(DNASHADERS_SOURCE_FILE);
	}
}
