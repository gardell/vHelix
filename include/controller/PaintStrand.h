/*
 * PaintStrand.h
 *
 *  Created on: 8 mar 2012
 *      Author: johan
 */

#ifndef _CONTROLLER_PAINTSTRAND_H_
#define _CONTROLLER_PAINTSTRAND_H_

#include <Utility.h>

#include <controller/Operation.h>
#include <model/Material.h>
#include <model/Base.h>
#include <model/Strand.h>

namespace Helix {
	namespace Controller {
		/*
		 * This is the controller for painting strands, these are implemented as functors used with the STL
		 */

		class PaintStrandOperation : public Operation<Model::Base, Model::Material, Model::Material> {
		public:
			PaintStrandOperation(Model::Material & material) : m_material(material) {

			}

			PaintStrandOperation() {

			}

			void setMaterial(Model::Material & material) {
				m_material = material;
			}

		protected:
			MStatus doExecute(Model::Base & element);
			MStatus doUndo(Model::Base & element, Model::Material & undoData);
			MStatus doRedo(Model::Base & element, Model::Material & redoData);
		private:
			Model::Material m_material;
		};

		/*
		 * Utility class for painting multiple strands with multiple randomized colors
		 */

		class PaintMultipleStrandsFunctor {
		public:
			inline PaintMultipleStrandsFunctor(Model::Material *materials, size_t & numMaterials) : m_materials(materials), m_numMaterials(numMaterials) {

			}

			inline PaintMultipleStrandsFunctor() {
				if (!(m_status = Model::Material::GetAllMaterials(&m_materials, m_numMaterials)))
					m_status.perror("Material::GetAllMaterials");
			}

			inline void operator() (Model::Strand strand) {
				/*
				 * Randomize a new color, assume numMaterials > 0
				 */

				m_operation.setMaterial(m_materials[rand() % m_numMaterials]);

				Model::Strand::ForwardIterator it = strand.forward_begin();
				for_each_itref(it, strand.forward_end(), m_operation.execute());

				if (!(m_status = m_operation.status())) {
					m_status.perror("PaintStrandOperation::status 1");
					return;
				}

				if (!it.loop()) {
					// Step back a step so that we don't color our base again. In the case of an end base with no backward connection, the loop will just be empty
					strand.setDefiningBase(strand.getDefiningBase().backward());

					std::for_each(strand.reverse_begin(), strand.reverse_end(), m_operation.execute());

					if (!(m_status = m_operation.status())) {
						m_status.perror("PaintStrandOperation::status 1");
						return;
					}
				}
			}

			inline const MStatus & status() const {
				return m_status;
			}

			inline MStatus undo() {
				return m_operation.undo();
			}

			inline MStatus redo() {
				return m_operation.redo();
			}

		protected:
			PaintStrandOperation m_operation;

			Model::Material *m_materials;
			size_t m_numMaterials;

			MStatus m_status;
		};

		/*
		 * Extended utility class of the above where we look at the current color of the base and tries to pick another color
		 */

		class PaintMultipleStrandsWithNewColorFunctor : public PaintMultipleStrandsFunctor {
		public:
			inline void operator() (Model::Strand strand) {
				/*
				 * Randomize a new color, assume numMaterials > 0
				 */

				Model::Material previous_material, chosen_material;

				{
					MStatus status;
					if (!(status = strand.getDefiningBase().getMaterial(previous_material)))
						status.perror("Base::getMaterial");
				}

				do {
					chosen_material = m_materials[rand() % m_numMaterials];
				} while (chosen_material == previous_material);

				m_operation.setMaterial(m_materials[rand() % m_numMaterials]);

				Model::Strand::ForwardIterator it = strand.forward_begin();
				for_each_itref(it, strand.forward_end(), m_operation.execute());


				if (!(m_status = m_operation.status())) {
					m_status.perror("PaintStrandOperation::status 1");
					return;
				}

				if (!it.loop()) {

					// Step back a step so that we don't color our base again. In the case of an end base with no backward connection, the loop will just be empty
					strand.setDefiningBase(strand.getDefiningBase().backward());
					
					std::for_each(strand.reverse_begin(), strand.reverse_end(), m_operation.execute());

					if (!(m_status = m_operation.status())) {
						m_status.perror("PaintStrandOperation::status 1");
						return;
					}
				}
			}
		};
	}
}

#endif /* _CONTROLLER_PAINTSTRAND_H_ */
