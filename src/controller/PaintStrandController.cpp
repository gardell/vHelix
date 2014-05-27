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
				//return status;

				/*
				 * This is in case there's no material assigned. It won't really be undoable though
				 * The JSON importer does not paint scaffolds, thus it is affected by this
				 */
				previous_material = m_material;
			}

			saveUndoRedo(element, previous_material, m_material);

			/*
			 * Set the new material
			 */

			if (!(status = element.setMaterial(m_material))) {
				//status.perror("Base::setMaterial");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus PaintStrandOperation::doUndo(Model::Base & element, Model::Material & undoData) {
			MStatus status;

			Model::Material oldMaterial;
			element.getMaterial(oldMaterial);

			if (!(status = element.setMaterial(undoData))) {
				//status.perror("Base::setMaterial");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus PaintStrandOperation::doRedo(Model::Base & element, Model::Material & redoData) {
			MStatus status;
			
			if (!(status = element.setMaterial(redoData))) {
				//status.perror("Base::setMaterial");
				return status;
			}

			return MStatus::kSuccess;
		}
	}
}