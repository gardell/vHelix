/*
 * CreatorGui.cpp
 *
 *  Created on: Jul 26, 2011
 *      Author: bjorn
 */

#include <CreatorGui.h>
#include <Creator.h>
#include <DNA.h>

#include <maya/MGlobal.h>

namespace Helix {
	CreatorGui::CreatorGui() {

	}

	CreatorGui::~CreatorGui() {

	}

	MStatus CreatorGui::doIt(const MArgList & args) {
		return MGlobal::executeCommandOnIdle(MString(
				"string $text;\n"
				"string $result = `promptDialog -title \"Create helix...\" -message \"Enter the number of bases to generate:\" -button \"OK\" -button \"Cancel\" -defaultButton \"OK\" -cancelButton \"Cancel\" -dismissString \"Cancel\" -style \"integer\" -text ") + Creator::default_bases + "`;\n"
				"if ($result == \"OK\") {\n"
				"int $bases = `promptDialog -query -text`;\n"
				"createHelix -base $bases;\n"
				"}\n");
	}

	MStatus CreatorGui::undoIt () {
		return MStatus::kSuccess;
	}

	MStatus CreatorGui::redoIt () {
		return MStatus::kSuccess;
	}

	bool CreatorGui::isUndoable () const {
		return false;
	}

	bool CreatorGui::hasSyntax () const {
		return false;
	}

	void *CreatorGui::creator() {
		return new CreatorGui();
	}
}
