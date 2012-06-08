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
 * This is a class for managing the color materials attached to bases
 * The material is just identified by a string, but this class contains some additional features for querying and applying them to bases
 */

namespace Helix {
	namespace Model {
		class Base;

		class Material {
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

			static MStatus GetAllMaterials(Material **materials, size_t & numMaterials);

			class RegisterMaterialFile {
			public:
				inline RegisterMaterialFile(const char *filepath) {
					s_materialFiles.push_back(std::make_pair(MString(filepath), false));
				}
			};

		protected:
			MString m_material;

		private:
			/*
			 * The above GetAllMaterials call this method
			 */

			static MStatus CacheMaterials();
			static std::vector<Material> s_materials;
			static std::vector<std::pair<MString, bool> > s_materialFiles;
		};
	}
}

#endif /* MATERIAL_H_ */
