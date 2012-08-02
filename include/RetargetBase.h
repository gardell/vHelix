/*
 * RetargetBase.h
 *
 *  Created on: 27 jan 2012
 *      Author: johan
 */

#ifndef RETARGETBASE_H_
#define RETARGETBASE_H_

/*
 * This is a MEL command that removes all aimconstraints on a base then makes it target a given base, either directly or in a perpendicular fashion
 * This command was originally baked into the HelixBase MPxTransform but made into it's own command because of the bugs in Maya when deleting nodes
 * This command is therefore NOT undoable and should not be undoable either! This makes it possible to call this command with OnIdle to avoid
 * executing the command during a deletion being in progress
 * Command usage: retargetBase [-perpendicular 1] -base <base name> -target <targe base name>
 */

#include <Definition.h>
#include <iostream>

#include <maya/MPxCommand.h>

#define MEL_RETARGETBASE_COMMAND "retargetBase"

namespace Helix {
	class VHELIXAPI RetargetBase : public MPxCommand {
	public:
		RetargetBase();
		virtual ~RetargetBase();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();
	};
}

#endif /* RETARGETBASE_H_ */
