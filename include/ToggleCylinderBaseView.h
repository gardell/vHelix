/*
 * HelixView.h
 *
 *  Created on: 14 jul 2011
 *      Author: johan
 */

#ifndef TOGGLECYLINDERBASEVIEW_H_
#define TOGGLECYLINDERBASEVIEW_H_

/*
 * Really simple command, returns the currently set view and allows changing it.
 * The result will be passed out as an MString
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>

#define MEL_TOGGLECYLINDERBASEVIEW_COMMAND "toggleCylinderBaseView"

namespace Helix {
	class ToggleCylinderBaseView : public MPxCommand {
	public:
		ToggleCylinderBaseView();
		virtual ~ToggleCylinderBaseView();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

		// This is the current status, can be accessed from the outside
		//

		static int CurrentView;

	private:
		MStatus toggle(bool toggle, bool refresh);
		bool m_toggled; // For undo/redo
	};
}


#endif /* HELIXVIEW_H_ */
