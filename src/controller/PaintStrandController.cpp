#include <controller/PaintStrand.h>

#include <model/Helix.h>
#include <model/Base.h>

#include <DNA.h>

#include <maya/MDGModifier.h>

namespace Helix {
	namespace Controller {
		MStatus PaintStrandOperation::doExecute(Model::Base & element) {
			MStatus status;

			/*
			 * Get and save the old material in case the user wants to undo
			 */

			Model::Material previous_material;

			if (!(status = element.getMaterial(previous_material))) {
				status.perror("Base::getMaterial");
				return status;
			}

			std::cerr << "Painting with material: " << m_material.getMaterial().asChar() << ", previous: " << previous_material.getMaterial().asChar() << std::endl;

			saveUndoRedo(element, previous_material, m_material);

			/*
			 * Set the new material
			 */

			if (!(status = element.setMaterial(m_material))) {
				status.perror("Base::setMaterial");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus PaintStrandOperation::doUndo(Model::Base & element, Model::Material & undoData) {
			MStatus status;
			
			std::cerr << "Setting previous material: " << undoData.getMaterial().asChar() << std::endl;

			if (!(status = element.setMaterial(undoData))) {
				status.perror("Base::setMaterial");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus PaintStrandOperation::doRedo(Model::Base & element, Model::Material & redoData) {
			MStatus status;
			
			if (!(status = element.setMaterial(redoData))) {
				status.perror("Base::setMaterial");
				return status;
			}

			return MStatus::kSuccess;
		}
	}
}