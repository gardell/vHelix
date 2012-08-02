#ifndef _TOGGLE_SHOW_SUGGESTED_CONNECTIONS_H_
#define _TOGGLE_SHOW_SUGGESTED_CONNECTIONS_H_

#include <Definition.h>
#include <maya/MPxCommand.h>
#include <maya/MObject.h>
#include <maya/MMessage.h>

#define MEL_TOGGLESHOWSUGGESTEDCONNECTIONS_COMMAND "toggleShowSuggestedConnections"

namespace Helix {
	/*
	 * Simple command, creates a ConnectSuggestionsLocatorNode when on, deletes the one it just created on off
	 * Makes sure to track any destruction of it as this would break the MObject reference it has though
	 */

	class VHELIXAPI ToggleShowSuggestedConnections : public MPxCommand {
		friend void MNodeMessage_locatorNode_nodePreRemovalCallback(MObject& node,void *clientData);
	public:
		ToggleShowSuggestedConnections();
		virtual ~ToggleShowSuggestedConnections();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();
	private:
		static MObject s_locatorNode;
		static MCallbackId s_locatorNode_preRemovalCallbackId;
	};
}

#endif /* N _TOGGLE_SHOW_SUGGESTED_CONNECTIONS_H_ */