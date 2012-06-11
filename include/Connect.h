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

#include <controller/Connect.h>
#include <controller/PaintStrand.h>

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
		Controller::Connect m_operation;
		Controller::PaintMultipleStrandsWithNewColorFunctor m_functor;
	};
}

#endif /* CONNECT_H_ */
