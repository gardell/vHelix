#include <controller/Disconnect.h>

namespace Helix {
	namespace Controller {
		MStatus Disconnect::doExecute(Model::Base & element) {
			MStatus status;

			/*
			 * Get the current forward attachment, if there is none, status will be kNotFound, thus nothing needs to be done!
			 */

			Model::Base forward = element.forward(status);
			
			if (status) {
				/*
				 * A connection exists, now disconnect it
				 */

				if (!(status = element.disconnect_forward())) {
					status.perror("Base::disconnect_forward");
					return status;
				}

				/*
				 * Now recolor its strand to another color than its current one
				 */

				{
					Model::Material *materials;
					size_t numMaterials;

					if (!(status = Model::Material::GetAllMaterials(&materials, numMaterials))) {
						status.perror("Material::GetAllMaterials");
					}
				}

				saveUndoRedo(element, forward);
			}
			else if (status != MStatus::kNotFound) {
				status.perror("Base::forward");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Disconnect::doUndo(Model::Base & element, Model::Base & undoData) {
			MStatus status;
			
			/*
			 * All elements added to the undo/redo queue are guaranteed to have had valid connections, all we need to do is restore them
			 */

			if (!(status = element.connect_forward(undoData))) {
				status.perror("Base::disconnect_forward");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Disconnect::doRedo(Model::Base & element, Empty & redoData) {
			MStatus status;

			/*
			 * We do the exact same thing as in doExecute except we never call the saveUndoRedo mechanism
			 */

			Model::Base forward = element.forward(status);
			
			if (status) {
				/*
				 * A connection exists, now disconnect it
				 */

				if (!(status = element.disconnect_forward())) {
					status.perror("Base::disconnect_forward");
					return status;
				}
			}
			else if (status != MStatus::kNotFound) {
				status.perror("Base::forward");
				return status;
			}

			return MStatus::kSuccess;
		}
	}
}
