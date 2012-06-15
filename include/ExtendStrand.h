/*
 * ExtendHelix.h
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#ifndef EXTENDSTRAND_H_
#define EXTENDSTRAND_H_

/*
 * Command for extending end helix bases with more bases. Select either one of the end bases of a helix or both and call this method
 * Supports multiple ends selected
 */

#include <Definition.h>

#include <iostream>

#include <maya/MPxCommand.h>

#include <controller/ExtendStrand.h>

#include <DNA.h>

#define MEL_EXTENDSTRAND_COMMAND "extendHelix"

namespace Helix {
	class ExtendStrand : public MPxCommand {
	public:
		ExtendStrand();
		virtual ~ExtendStrand();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

		static size_t default_length;

	private:
		class ExtendStrandWithProgressBar : public Controller::ExtendStrand {
		protected:
			void onProgressBegin(int range);
			void onProgressStep();
			void onProgressDone();
		} m_operation;
	};
}


#endif /* EXTENDSTRAND_H_ */
