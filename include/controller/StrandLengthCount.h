#ifndef _CONTROLLER_STRAND_LENGTH_COUNT_H_
#define _CONTROLLER_STRAND_LENGTH_COUNT_H_

#include <Definition.h>

#include <Model/Strand.h>

namespace Helix {
	namespace Controller {
		class VHELIXAPI StrandLengthCount {
		public:
			unsigned int length(Model::Strand & strand);
		};
	}
}

#endif /* _CONTROLLER_STRAND_LENGTH_COUNT_H_ */