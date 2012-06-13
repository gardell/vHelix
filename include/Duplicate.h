/*
 * Duplicate.h
 *
 *  Created on: Aug 3, 2011
 *      Author: bjorn
 */

#ifndef DUPLICATE_H_
#define DUPLICATE_H_

/*
 * Command for duplicating helices and their belongings
 */

#include <Definition.h>

#include <controller/Duplicate.h>

#include <iostream>
#include <vector>

#include <maya/MPxCommand.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>

#define MEL_DUPLICATEHELICES_COMMAND "duplicateHelices"

namespace Helix {
	class Duplicate : public MPxCommand {
	public:
		Duplicate();
		virtual ~Duplicate();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		class DuplicateWithProgressBar : public Controller::Duplicate {
			void onProgressStart(unsigned int process, unsigned int range);
			void onProgressStep();
			void onProgressDone();
		} m_operation;

		/*MStatus duplicate(const MObjectArray & targets);

		MObjectArray m_duplicated_helices, m_target_helices; // First is for undo, second for redo*/
	};
}

#endif /* DUPLICATE_H_ */
