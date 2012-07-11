/*
 * ExtendGui.cpp
 *
 *  Created on: Jul 26, 2011
 *      Author: bjorn
 */

#include <ExtendGui.h>
#include <DNA.h>

#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include <ExtendStrand.h>

namespace Helix {
	ExtendGui::ExtendGui() {

	}

	ExtendGui::~ExtendGui() {

	}

	MStatus ExtendGui::doIt(const MArgList & args) {
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

		return MGlobal::executeCommandOnIdle(MString(
				"string $text;\n"
				"string $result = `promptDialog -title \"Extend bases...\" -message \"Enter the number of bases to generate:\" -button \"OK\" -button \"Cancel\" -defaultButton \"OK\" -cancelButton \"Cancel\" -dismissString \"Cancel\" -style \"integer\" -text ") + (double) ExtendStrand::default_length + "`;\n"
				"if ($result == \"OK\") {\n"
				"int $bases = `promptDialog -query -text`;\n"
				MEL_EXTENDSTRAND_COMMAND " -base $bases" + (target.length() > 0 ? (MString(" -target ") + target) : MString("")) + ";\n"
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
		return true;
	}

	MSyntax ExtendGui::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *ExtendGui::creator() {
		return new ExtendGui();
	}
}
