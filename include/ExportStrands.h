/*
 * ExportStrands.h
 *
 *  Created on: 12 jul 2011
 *      Author: johan
 */

#ifndef EXPORTSTRANDS_H_
#define EXPORTSTRANDS_H_

/*
 * Command for exporting the strand sequences of the target, selected or all helices
 * The result will be passed out as an MStringArray
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>

#include <controller/ExportStrands.h>

#define MEL_EXPORTSTRANDS_COMMAND "exportStrands"

namespace Helix {
	class ExportStrands : public MPxCommand {
	public:
		ExportStrands();
		virtual ~ExportStrands();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		class ExportStrandsWithProgressBar : public Controller::ExportStrands {
		public:
			void onProgressStep();
		} m_operation;
	};
}

#endif /* EXPORTSTRANDS_H_ */
