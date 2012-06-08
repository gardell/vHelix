/*
 * Disconnect.h
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#ifndef DISCONNECT_H_
#define DISCONNECT_H_

/*
 * Command for disconnecting helix bases, the selected base will have its forward attribute connection removed
 */

#include <Definition.h>

#include <iostream>
#include <vector>

#include <maya/MPxCommand.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>

#include <controller/Disconnect.h>

#define MEL_DISCONNECTBASE_COMMAND "disconnectBase"

namespace Helix {
	class Disconnect : public MPxCommand {
	public:
		Disconnect();
		virtual ~Disconnect();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		Controller::Disconnect m_operation;
		Controller::PaintMultipleStrandsWithNewColorFunctor m_functor;

		/*MStatus disconnect(const MObjectArray & targets);

		std::vector<std::pair<MObject, MObject> > m_disconnected; // For undo/redo
		MObjectArray m_disconnect_targets;*/
	};
}

#endif /* DISCONNECT_H_ */
