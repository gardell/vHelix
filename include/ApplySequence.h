/*
 * ApplySequence.h
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#ifndef APPLYSEQUENCE_H_
#define APPLYSEQUENCE_H_

/*
 * Command for applying a DNA string sequence to a selected strand and its opposites
 */

#include <Definition.h>

#include <controller/ApplySequence.h>

#include <iostream>
#include <list>

#include <maya/MPxCommand.h>
#include <maya/MDagPath.h>

#define MEL_APPLYSEQUENCE_COMMAND "applySequence"

namespace Helix {
	class ApplySequence : public MPxCommand {
	public:
		ApplySequence();
		virtual ~ApplySequence();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		Controller::ApplySequence m_operation;
	};
}

#endif /* APPLYSEQUENCE_H_ */
