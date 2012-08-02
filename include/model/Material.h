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
