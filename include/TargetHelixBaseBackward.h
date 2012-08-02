#ifndef _TARGET_HELIX_BASE_BACKWARD_H_
#define _TARGET_HELIX_BASE_BACKWARD_H_

#define MEL_TARGET_HELIXBASE_BACKWARD "targetHelixBaseBackward"

#include <Definition.h>
#include <maya/MPxCommand.h>

namespace Helix {
	/*
	 * Internal command used to target a base perpendicular to it's backward connection
	 * The reason for this is that Maya will crash if such an action is being made during a deletion of a node in the MPxNode implementation
	 * by calling a executeCommandOnIdle we can avoid this by postponing the execution until later.
	 * However, the OnIdle version allows the user to undo the action, which is very strange.
	 * By implementing it as a MPxCommand that is NOT undoable, Maya won't try to undo it.
	 * It's a long shot but it works
	 */

	class VHELIXAPI TargetHelixBaseBackward : public MPxCommand {
	public:
		TargetHelixBaseBackward();
		virtual ~TargetHelixBaseBackward();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();
	};
}

#endif /* N _TARGET_HELIX_BASE_BACKWARD_H_ */