/*
 * ExtendHelix.h
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#ifndef EXTENDHELIX_H_
#define EXTENDHELIX_H_

/*
 * Command for extending end helix bases with more bases. Select either one of the end bases of a helix or both and call this method
 * Supports multiple ends selected
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>

#define MEL_EXTENDHELIX_COMMAND "extendHelix"

namespace Helix {
	class ExtendHelix : public MPxCommand {
	public:
		ExtendHelix();
		virtual ~ExtendHelix();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();
	};
}


#endif /* EXTENDHELIX_H_ */
