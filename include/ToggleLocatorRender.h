/*
 * ToggleLocatorRender.h
 *
 *  Created on: Aug 2, 2011
 *      Author: bjorn
 */

#ifndef TOGGLELOCATORRENDER_H_
#define TOGGLELOCATORRENDER_H_

/*
 * Really simple command, return an array of rendering features currently enabled for the locator nodes
 * The result will be passed out as an MStringArray
 * Can also set the render modes to be used, special keyword "all" is available
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>

#define MEL_TOGGLELOCATORRENDER_COMMAND "toggleLocatorRender"

// There's also a special alias "all"
#define RENDER_MODES	"halo", "pair_line", "sequence", "directional_arrow"

namespace Helix {
	class ToggleLocatorRender : public MPxCommand {
	public:
		ToggleLocatorRender();
		virtual ~ToggleLocatorRender();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

		// This is the current status, can be accessed from the outside
		//

		enum {
			kRenderHalo = 0x1,
			kRenderPairLines = 0x2,
			kRenderSequence = 0x4,
			kRenderDirectionalArrow = 0x8,
			kRenderAll = 0xF
		};

		static unsigned int CurrentRender;

	private:
		int m_lastRender; // For undo

		MStatus updateRender(unsigned int view);
	};
}

#endif /* TOGGLELOCATORRENDER_H_ */
