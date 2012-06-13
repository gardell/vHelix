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
	class PaintStrand : public MPxCommand {
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

		// Direct interface, used by other methods and implements the undo/redo ability

		//MStatus paintStrands(MDagPathArray & dagPathArray);

	private:
		// The NEW STL-oriented interface requires less data to be saved
		Controller::PaintMultipleStrandsWithNewColorFunctor m_functor;

		// Just paint the bases given and put their old color in m_previousPaint
		/*MStatus paintBases(MDagPathArray & dagPathArray);

		//std::vector<std::pair<MDagPath, MString> > m_previousPaint; // Used for undo/redo
		MDagPathArray m_previousPaintedDagNodes;
		MStringArray m_paintedColors[2];
		int m_currentPaintedColors;*/
	};
}

#endif /* PAINTSTRAND_H_ */
