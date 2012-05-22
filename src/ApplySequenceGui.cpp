/*
 * ApplySequenceGui.cpp
 *
 *  Created on: Jul 26, 2011
 *      Author: bjorn
 */

#include <ApplySequenceGui.h>
#include <DNA.h>

#include <maya/MGlobal.h>

namespace Helix {
	ApplySequenceGui::ApplySequenceGui() {

	}

	ApplySequenceGui::~ApplySequenceGui() {

	}

	MStatus ApplySequenceGui::doIt(const MArgList & args) {
		return MGlobal::executeCommandOnIdle(
				"string $text;\n"
				"string $result = `promptDialog -title \"Apply sequence...\" -message \"Enter the DNA sequence to apply:\" -button \"OK\" -button \"Cancel\" -defaultButton \"OK\" -cancelButton \"Cancel\" -dismissString \"Cancel\"`;\n"
				"if ($result == \"OK\") {\n"
				"applySequence -sequence `promptDialog -query -text`;\n"
				"}\n");
	}

	MStatus ApplySequenceGui::undoIt () {
		return MStatus::kSuccess;
	}

	MStatus ApplySequenceGui::redoIt () {
		return MStatus::kSuccess;
	}

	bool ApplySequenceGui::isUndoable () const {
		return false;
	}

	bool ApplySequenceGui::hasSyntax () const {
		return false;
	}

	void *ApplySequenceGui::creator() {
		return new ApplySequenceGui();
	}
}
