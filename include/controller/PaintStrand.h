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

		class VHELIXAPI PaintStrandOperation : public Operation<Model::Base, Model::Material, Model::Material> {
		public:
			PaintStrandOperation(Model::Material & material) : m_material(material) {

			}

			PaintStrandOperation() {

			}

			void setMaterial(const Model::Material & material) {
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

		class VHELIXAPI PaintMultipleStrandsFunctor {
		public:
			inline MStatus loadMaterials() {
				MStatus status;

				m_materials_begin = Model::Material::AllMaterials_begin(status, m_numMaterials);

				if (!status) {
					status.perror("Material::GetAllMaterials");
					return status;
				}

				return MStatus::kSuccess;
			}

			inline void operator() (Model::Strand strand) {
				/*
				 * Randomize a new color, assume numMaterials > 0
				 */

				m_operation.setMaterial(*(m_materials_begin + (rand() % m_numMaterials)));

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

		protected: // FIXME
			PaintStrandOperation m_operation;

			/*Model::Material *m_materials;
			size_t m_numMaterials;*/
			Model::Material::Iterator m_materials_begin;
			Model::Material::Container::size_type m_numMaterials;

			MStatus m_status;
		};

		/*
		 * Extended utility class of the above where we look at the current color of the base and tries to pick another color
		 */

		class VHELIXAPI PaintMultipleStrandsWithNewColorFunctor : public PaintMultipleStrandsFunctor {
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
					chosen_material = *(m_materials_begin + (rand() % m_numMaterials));
					//chosen_material = *(Model::Material::AllMaterials_begin(m_status, m_numMaterials) + (rand() % m_numMaterials));
				} while (chosen_material == previous_material);

				m_operation.setMaterial(chosen_material);

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

		/*
		 * This one is used by the JSON importer, it is not interested in undo functionality as the strand has not existed before
		 */

		class VHELIXAPI PaintMultipleStrandsNoUndoFunctor {
		public:
			class SetMaterialFunctor {
			public:
				inline SetMaterialFunctor(Model::Material::ApplyMaterialToBases apply) : m_apply(apply) {

				}

				inline void operator() (Model::Base & base) {
					MStatus status;

					if (!(status = m_apply.add(base)))
						m_status = status;
				}

				inline MStatus status() const {
					return m_status;
				}

				inline const Model::Material::ApplyMaterialToBases & getApply() const {
					return m_apply;
				}

			private:
				Model::Material::ApplyMaterialToBases m_apply;
				MStatus m_status;
			};

			inline void operator() (Model::Strand strand, Model::Material & material) {
				/*
				 * Randomize a new color, assume numMaterials > 0
				 */

				Model::Strand::ForwardIterator it = strand.forward_begin();

				SetMaterialFunctor functor(material.setMaterialOnMultipleBases());
				for_each_ref_itref(it, strand.forward_end(), functor);

				if (!(m_status = functor.status())) {
					HMEVALUATE_DESCRIPTION("SetMaterialFunctor", m_status);
					return;
				}

				if (!it.loop()) {
					// Step back a step so that we don't color our base again. In the case of an end base with no backward connection, the loop will just be empty
					strand.setDefiningBase(strand.getDefiningBase().backward());

					for_each_ref(strand.reverse_begin(), strand.reverse_end(), functor);

					if (!(m_status = functor.status())) {
						HMEVALUATE_DESCRIPTION("SetMaterialFunctor", m_status);
						return;
					}
				}

				m_status = functor.getApply().apply();
			}

			inline const MStatus & status() const {
				return m_status;
			}

		private:
			MStatus m_status;
		};

		class VHELIXAPI PaintMultipleStrandsNoUndoNoOverrideFunctor : public PaintMultipleStrandsNoUndoFunctor {
		public:
			inline void operator() (Model::Strand strand, Model::Material & material) {
				MStatus status;
				Model::Material m;

				if (!(status = strand.getDefiningBase().getMaterial(m)))
					PaintMultipleStrandsNoUndoFunctor::operator()(strand, material);
			}
		private:
		};
	}
}

#endif /* _CONTROLLER_PAINTSTRAND_H_ */
