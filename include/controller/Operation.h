/*
 * Operation.h
 *
 *  Created on: 8 mar 2012
 *      Author: johan
 */

#ifndef _CONTROLLER_OPERATION_H_
#define _CONTROLLER_OPERATION_H_

#include <iostream>
#include <Definition.h>

#include <maya/MDagPath.h>
#include <maya/MObject.h>

#include <list>
#include <algorithm>

/*
 * An operation defines an action that is doable, undoable and redoable using functors (STL)
 */
namespace Helix {
	namespace Controller {
		template<typename ElementT, typename UndoDataT, typename RedoDataT>
		class Operation {
		public:
			class Do {
				friend class Operation;
			public:
				void operator() (ElementT & element) {
					m_operation->setStatus(m_operation->doExecute(element));
				}
			private:
				Do(Operation & operation) {
					m_operation = &operation;
				}

				Operation *m_operation;
			};

			class Undo {
				friend class Operation;
			public:
				void operator() (std::pair<ElementT, std::pair<UndoDataT, RedoDataT> > & data) {
					m_operation->setStatus(m_operation->doUndo(data.first, data.second.first));
				}
			private:
				Undo(Operation & operation) {
					m_operation = &operation;
				}

				Operation *m_operation;
			};

			class Redo {
				friend class Operation;
			public:
				void operator() (std::pair<ElementT, std::pair<UndoDataT, RedoDataT> > & data) {
					m_operation->setStatus(m_operation->doRedo(data.first, data.second.second));
				}
			private:
				Redo(Operation & operation) {
					m_operation = &operation;
				}

				Operation *m_operation;
			};

			/*
			 * These must be implemented by the algorithm
			 */
			virtual MStatus doExecute(ElementT & element) = 0;
			virtual MStatus doUndo(ElementT & element, UndoDataT & undoData) = 0;
			virtual MStatus doRedo(ElementT & element, RedoDataT & redoData) = 0;

			/*
			 * Use execute with the STL algorithms such as std::for_each
			 */
			Do execute() {
				m_status = MStatus::kSuccess;
				return Do(*this);
			}

			/*
			 * While undo and redo will iterate on all added undoable/redoable objects
			 */
			Undo undo() {
				m_status = MStatus::kSuccess;
				std::for_each(m_undoRedoData.begin(), m_undoRedoData.end(), Undo(*this));
			}

			Undo redo() {
				m_status = MStatus::kSuccess;
				std::for_each(m_undoRedoData.begin(), m_undoRedoData.end(), Redo(*this));
			}

			/*
			 * After an iteration has been completed, status can be queried
			 */
			MStatus status();

			/*
			 * The execute method can save undo/redo state by calling this method
			 */
			void saveUndoRedo(ElementT & element, UndoDataT & undoData, RedoDataT & redoData) {
				m_undoRedoData.push_back(std::pair<ElementT, std::pair<UndoDataT, RedoDataT> >(element, std::make_pair(undoData, redoData)));
			}

		private:
			void setStatus(MStatus & status) {
				if (status != MStatus::kSuccess)
					m_status = status;
			}

			MStatus m_status;

			std::list<std::pair<ElementT, std::pair<UndoDataT, RedoDataT> > > m_undoRedoData;
		};
	}
}

#endif /* _CONTROLLER_OPERATION_H_ */
