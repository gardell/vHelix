#ifndef _CONTROLLER_APPLYSEQUENCE_H_
#define _CONTROLLER_APPLYSEQUENCE_H_

#include <controller/Operation.h>
#include <model/Base.h>

#include <DNA.h>

namespace Helix {
	namespace Controller {
		class ApplySequence : public Operation<Model::Base, DNA::Names, DNA::Names> {
		public:
			inline void setSequence(const MString & sequence) {
				m_sequence = sequence;
				m_sequence_index = 0;
			}

		protected:
			MStatus doExecute(Model::Base & element);
			MStatus doUndo(Model::Base & element, DNA::Names & undoData);
			MStatus doRedo(Model::Base & element, DNA::Names & redoData);

		private:
			MString m_sequence;
			unsigned int m_sequence_index;
		};
	}
}

#endif /* N _CONTROLLER_APPLYSEQUENCE_H_ */