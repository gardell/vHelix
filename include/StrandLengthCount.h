#ifndef _STRAND_LENGTH_COUNT_H_
#define _STRAND_LENGTH_COUNT_H_

/*
* Command for counting the length of a strand.
*/

#include <Definition.h>

#include <controller/StrandLengthCount.h>

#include <iostream>
#include <list>

#include <maya/MPxCommand.h>
#include <maya/MDagPath.h>

#define MEL_STRANDLENGTHCOUNT_COMMAND "strandLength"

namespace Helix {
	class VHELIXAPI StrandLengthCount : public MPxCommand {
	public:
		virtual ~StrandLengthCount();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt();
		virtual MStatus redoIt();
		virtual bool isUndoable() const;
		virtual bool hasSyntax() const;

		static MSyntax newSyntax();
		static void *creator();

	private:
		Controller::StrandLengthCount m_operation;
	};
}

#endif /* _STRAND_LENGTH_COUNT_H_ */