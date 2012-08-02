/*
 * PaintStrand.h
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#ifndef PAINTSTRAND_H_
#define PAINTSTRAND_H_

/*
 * Command for painting the selected strands with a random material
 */

#include <Definition.h>

#include <controller/PaintStrand.h>

#include <iostream>

#include <maya/MPxCommand.h>
#include <maya/MDagPathArray.h>
#include <maya/MStringArray.h>

#define MEL_PAINTSTRAND_COMMAND "paintStrand"

namespace Helix {
	class VHELIXAPI PaintStrand : public MPxCommand {
	public:
		PaintStrand();
		virtual ~PaintStrand();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

	private:
		Controller::PaintMultipleStrandsWithNewColorFunctor m_functor;
	};
}

#endif /* PAINTSTRAND_H_ */
