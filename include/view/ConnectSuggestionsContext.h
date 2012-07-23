#ifndef _CONNECT_SUGGESTIONS_CONTEXT_H_
#define _CONNECT_SUGGESTIONS_CONTEXT_H_

#include <Definition.h>
#include <view/ConnectSuggestionsToolCommand.h>
#include <model/Base.h>

#include <maya/MPxContext.h>


namespace Helix {
	namespace View {
		class ConnectSuggestionsContext : public MPxContext {
		public:
			ConnectSuggestionsContext();
			virtual ~ConnectSuggestionsContext();

			virtual void toolOnSetup(MEvent &event);
			virtual void toolOffCleanup();
			virtual MStatus doPress(MEvent &event);
			virtual MStatus doDrag(MEvent &event);
			virtual MStatus doRelease(MEvent &event);
			virtual MStatus doEnterRegion(MEvent &event);
			virtual void getClassName(MString & name) const;

			virtual void completeAction();
			virtual void abortAction();

			/*
			 * These are called by the locator when there's an action to complete,
			 * the context uses the toolCommand to manage do/undo/redo functionality
			 */

			void connect(Model::Base & source, Model::Base & destination);

		private:
			MObject m_locatorNode;
			ConnectSuggestionsToolCommand *m_cmd;
		};
	}
}

#endif /* N _CONNECT_SUGGESTIONS_CONTEXT_H_ */