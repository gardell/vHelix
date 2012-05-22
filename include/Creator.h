/*
 * Creator.h
 *
 *  Created on: 9 jul 2011
 *      Author: johan
 */

#ifndef CREATOR_H_
#define CREATOR_H_

/*
 * Command for creating new helices
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>

#define MEL_CREATEHELIX_COMMAND "createHelix"

namespace Helix {
	class Creator : public MPxCommand {
	public:
		Creator();
		virtual ~Creator();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

		static int default_bases; // If no argument is given, uses this. It is initialized to DNA::CREATE_DEFAULT_NUM_BASES on startup

	private:
		MStatus createHelix(int bases);
		MObject m_helix;
		//MObjectArray m_undoObjects; // Objects to be deleted by the undo proc
	};
}


#endif /* CREATOR_H_ */
