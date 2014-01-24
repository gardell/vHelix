/*
 * ApplySequenceGui.cpp
 *
 *  Created on: Jul 26, 2011
 *      Author: bjorn
 */

#include <ApplySequenceGui.h>
#include <ApplySequence.h>
#include <DNA.h>

#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

namespace Helix {
	ApplySequenceGui::ApplySequenceGui() {

	}

	ApplySequenceGui::~ApplySequenceGui() {

	}

	MStatus ApplySequenceGui::doIt(const MArgList & args) {
		// OnIdle is okay here, cause we want the following command to be undoable!
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		MString target;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t")) {
			if (!(status = argDatabase.getFlagArgument("-t", 0, target))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}

		return MGlobal::executeCommandOnIdle(
				"{\n"
				"string $text;\n"
				"string $result = `promptDialog -title \"Apply sequence...\" -message \"Enter the DNA sequence to apply:\" -button \"OK\" -button \"Cancel\" -defaultButton \"OK\" -cancelButton \"Cancel\" -dismissString \"Cancel\"`;\n"
				"if ($result == \"OK\") {\n"
				MEL_APPLYSEQUENCE_COMMAND " -sequence `promptDialog -query -text`" + (target.length() > 0 ? (MString(" -target ") + target) : MString("")) + ";\n"
				"}\n}\n");
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
		return true;
	}

	MSyntax ApplySequenceGui::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *ApplySequenceGui::creator() {
		return new ApplySequenceGui();
	}
}
