#ifndef _CREATECURVES_H_
#define _CREATECURVES_H_

#include <Definition.h>

#include <controller/CreateCurves.h>

#include <maya/MPxCommand.h>

#define MEL_CREATE_CURVES_COMMAND "createStrandCurves"

namespace Helix {
	class CreateCurves : public MPxCommand {
	public:
		CreateCurves();
		virtual ~CreateCurves();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static void *creator();
		static MSyntax newSyntax();

	private:
		Controller::CreateCurves m_operation;
	};
}

#endif /* N _CREATECURVES_H_ */
