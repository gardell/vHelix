#include <controller/ApplySequence.h>

#include <DNA.h>

namespace Helix {
	namespace Controller {
		MStatus ApplySequence::doExecute(Model::Base & element) {
			if (m_sequence_index < m_sequence.length()) {
				MStatus status;

				char char_name = m_sequence.asChar()[m_sequence_index++];

				DNA::Names name(char_name), previous_name;

				if (!(status = element.getLabel(previous_name))) {
					status.perror("Base::getLabel");
					return status;
				}

				if (!(status = element.setLabel(name))) {
					status.perror("Base::setLabel");
					return status;
				}

				saveUndoRedo(element, name, previous_name);
			}

			return MStatus::kSuccess;
		}

		MStatus ApplySequence::doUndo(Model::Base & element, DNA::Names & undoData) {
			MStatus status;

			if (!(status = element.setLabel(undoData))) {
				status.perror("Base::setLabel");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus ApplySequence::doRedo(Model::Base & element, DNA::Names & redoData) {
			return doUndo(element, redoData);
		}
	}
}
