#ifndef _CONTROLLER_FILLSTRANDGAPS_H_
#define _CONTROLLER_FILLSTRANDGAPS_H_

#include <Definition.h>

#include <model/Base.h>
#include <model/Helix.h>
#include <model/Object.h>

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MVector.h>

#include <RetargetBase.h>

#include <vector>

namespace Helix {
	namespace Controller {
		class FillStrandGaps {
		public:
			/*
			 * ContainerT: Iterable container of MObjects. Must implement begin, end and size.
			 */
			template<typename ContainerT>
			MStatus fill(const ContainerT & objects) {
				MStatus status;

				for (typename ContainerT::const_iterator it(objects.begin()); it != objects.end(); ++it) {
					HMEVALUATE_RETURN(status = fill_object(*it), status);
				}

				return redo();
			}

			
			inline MStatus undo() {
				MStatus status;

				for (std::vector<Model::Base>::iterator it(undoable.begin()); it != undoable.end(); ++it) {
					HMEVALUATE(it->deleteNode(), status);
				}

				for (std::vector < std::pair<Model::Base, Model::Base> >::iterator it(previously_connected.begin()); it != previously_connected.end(); ++it) {
					// There seem to be a bug with the reconnectioning of bases.
					HMEVALUATE(status = it->first.connect_forward(it->second), status);
					MDagPath first_fullpath, second_fullpath;
					HMEVALUATE_RETURN(first_fullpath = it->first.getDagPath(status), status);
					HMEVALUATE_RETURN(second_fullpath = it->second.getDagPath(status), status);
					HMEVALUATE_RETURN(status = MGlobal::executeCommandOnIdle(MString(MEL_RETARGETBASE_COMMAND " -base ") + first_fullpath.fullPathName() + " -target " + second_fullpath.fullPathName()), status);
				}


				undoable.clear();
				previously_connected.clear();
				return MStatus::kSuccess;
			}

			MStatus redo();

			MStatus fill_object(const MObject & object);
			MStatus fill_base(Model::Base & base);

		private:
			struct Redoable {
				Model::Base start, end;
				Model::Helix helix;

				inline Redoable(const Model::Base & start, const Model::Base & end, const Model::Helix & helix) : start(start), end(end), helix(helix) {}
			};

			std::vector<Model::Base> undoable;
			std::vector < std::pair<Model::Base, Model::Base> > previously_connected;
			std::vector<Redoable> redoable;
		};
	}
}

#endif /* _CONTROLLER_FILLSTRANDGAPS_H_ */
