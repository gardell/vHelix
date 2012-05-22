/*
 * ExtendGui.cpp
 *
 *  Created on: Jul 26, 2011
 *      Author: bjorn
 */

#include <ExtendGui.h>
#include <DNA.h>

#include <maya/MGlobal.h>

namespace Helix {
	ExtendGui::ExtendGui() {

	}

	ExtendGui::~ExtendGui() {

	}

	MStatus ExtendGui::doIt(const MArgList & args) {
		// OnIdle is okay here, cause we want the following command to be undoable!

		return MGlobal::executeCommandOnIdle(MString(
				"string $text;\n"
				"string $result = `promptDialog -title \"Extend bases...\" -message \"Enter the number of bases to generate:\" -button \"OK\" -button \"Cancel\" -defaultButton \"OK\" -cancelButton \"Cancel\" -dismissString \"Cancel\" -style \"integer\" -text ") + DNA::CREATE_DEFAULT_NUM_BASES + "`;\n"
				"if ($result == \"OK\") {\n"
				"int $bases = `promptDialog -query -text`;\n"
				"extendHelix -base $bases;\n"
				"}\n");
	}

	MStatus ExtendGui::undoIt () {
		return MStatus::kSuccess;
	}

	MStatus ExtendGui::redoIt () {
		return MStatus::kSuccess;
	}

	bool ExtendGui::isUndoable () const {
		return false;
	}

	bool ExtendGui::hasSyntax () const {
		return false;
	}

	void *ExtendGui::creator() {
		return new ExtendGui();
	}
}
