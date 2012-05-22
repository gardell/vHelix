/*
 * PaintStrand.h
 *
 *  Created on: 8 mar 2012
 *      Author: johan
 */

#ifndef _CONTROLLER_PAINTSTRAND_H_
#define _CONTROLLER_PAINTSTRAND_H_

#include <controller/Operation.h>
#include <model/Material.h>
#include <model/Base.h>

namespace Helix {
	namespace Controller {
		/*
		 * This is the controller for painting strands, these are implemented as functors used with the STL
		 */

		class PaintStrandOperation : public Operation<Model::Base, Model::Material, Model::Material> {
		public:
			PaintStrandOperation(Model::Material & material) : m_material(material) {

			}

			MStatus doExecute(Model::Base & element);
			MStatus doUndo(Model::Base & element, Model::Material & undoData);
			MStatus doRedo(Model::Base & element, Model::Material & redoData);
		private:
			Model::Material m_material;
		};
	}
}

#endif /* _CONTROLLER_PAINTSTRAND_H_ */
