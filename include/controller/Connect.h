#ifndef _CONTROLLER_CREATOR_H_
#define _CONTROLLER_CREATOR_H_

#include <controller/Operation.h>

#include <model/Base.h>

#include <list>
#include <vector>

namespace Helix {
	namespace Controller {
		class Connect {
		public:
			MStatus connect(Model::Base source, Model::Base target);
			MStatus undo();
			MStatus redo();
		private:
			/*
			 * The array will have two elements, source and target, where the vector will contain one or two elements where the second will point to the element's old target
			 * Note that the first element in the array is the source and the second is the destination, thus the second elements in the vector have the opposite type of connection (destination and source)
			 * Also note that the second element in the vector is optional
			 */

			std::vector<Model::Base> m_previous_connections[2];
		};
	}
}

#endif /* N _CONTROLLER_CREATOR_H_ */