#ifndef _VIEW_CONNECT_SUGGESTIONS_TOOL_COMMAND_H_
#define _VIEW_CONNECT_SUGGESTIONS_TOOL_COMMAND_H_

#include <Definition.h>
#include <model/Base.h>
#include <controller/Connect.h>
#include <controller/PaintStrand.h>

#include <maya/MPxToolCommand.h>

#define MEL_CONNECT_SUGGESTIONS_TOOL_COMMAND "connectSuggestionsCmd"

namespace Helix {
	namespace View {
		class ConnectSuggestionsToolCommand : public MPxToolCommand {
		public:
			ConnectSuggestionsToolCommand();
			virtual ~ConnectSuggestionsToolCommand();

			MStatus doIt(const MArgList& args);
			MStatus redoIt();
			MStatus undoIt();
			bool isUndoable() const;
			MStatus finalize();

			static MSyntax newSyntax();
			static void *creator();

			void connect(Model::Base & source, Model::Base & destination);

		private:
			Controller::Connect m_operation;
			Controller::PaintMultipleStrandsWithNewColorFunctor m_functor;
		};
	}
}

#endif /* N _VIEW_CONNECT_SUGGESTIONS_TOOL_COMMAND_H_ */