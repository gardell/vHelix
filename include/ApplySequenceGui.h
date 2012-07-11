/*
 * ApplySequenceGui.h
 *
 *  Created on: Jul 26, 2011
 *      Author: bjorn
 */

#ifndef APPLYSEQUENCEGUI_H_
#define APPLYSEQUENCEGUI_H_

/*
 * Shows a confirmDialog in MEL that then calls the applySequence method
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>

#define MEL_APPLYSEQUENCE_GUI_COMMAND "applySequence_gui"

namespace Helix {
	class ApplySequenceGui : public MPxCommand {
	public:
		ApplySequenceGui();
		virtual ~ApplySequenceGui();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static void *creator();
		static MSyntax newSyntax();
	};
}

#endif /* APPLYSEQUENCEGUI_H_ */
