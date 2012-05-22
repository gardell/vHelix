/*
 * Connect.h
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#ifndef CONNECT_H_
#define CONNECT_H_

/*
 * Command for connecting helix bases, disconnects old bindings if required
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>
#include <maya/MObject.h>

#define MEL_CONNECTBASES_COMMAND "connectBases"

namespace Helix {
	class Connect : public MPxCommand {
	public:
		Connect();
		virtual ~Connect();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		MStatus connect(const MObject & target, const MObject & source);

		MObject m_connected_target, m_connected_source, m_old_target, m_old_source; // Used for undo/redo
	};
}

#endif /* CONNECT_H_ */
