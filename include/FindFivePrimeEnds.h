/*
 * FindFivePrimeEnds.h
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#ifndef FINDFIVEPRIMEENDS_H_
#define FINDFIVEPRIMEENDS_H_

/*
 * Command for finding the fiveprime ends
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>

#define MEL_FINDFIVEPRIMEENDS_COMMAND "findFivePrimeEnds"

namespace Helix {
	class VHELIXAPI FindFivePrimeEnds : public MPxCommand {
	public:
		FindFivePrimeEnds();
		virtual ~FindFivePrimeEnds();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		MSelectionList m_new_selectionList, m_old_selectionList; // For undo, redo
	};
}

#endif /* FINDFIVEPRIMEENDS_H_ */
