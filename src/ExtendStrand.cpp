/*
 * ExtendHelix.cpp
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#include <ExtendStrand.h>
#include <HelixBase.h>
#include <Utility.h>
#include <DNA.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPathArray.h>
#include <maya/MDGModifier.h>
#include <maya/MProgressWindow.h>

#include <vector>
#include <iterator>
#include <cmath>

#include <model/Helix.h>

namespace Helix {
	size_t ExtendStrand::default_length = DNA::CREATE_DEFAULT_NUM_BASES;

	ExtendStrand::ExtendStrand() {

	}

	ExtendStrand::~ExtendStrand() {

	}

	MStatus ExtendStrand::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		std::list<Model::Base> targets;
		int bases = 0;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-b")) {
			if (!(status = argDatabase.getFlagArgument("-b", 0, bases))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}

		if (bases == 0) {
			MGlobal::displayError("No valid number of bases to generate given");
			return MStatus::kFailure;
		}

		if (argDatabase.isFlagSet("-t")) {
			MString targetName;

			if (!(status = argDatabase.getFlagArgument("-t", 0, targetName))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;

			std::cerr << "add targetName: " << targetName.asChar() << std::endl;

			if (!(status = selectionList.add(targetName))) {
				status.perror("ExtendStrand::doIt: MSelectionList::add");
				return status;
			}

			MObject target_object;

			if (!(status = selectionList.getDependNode(0, target_object))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}

			targets.push_back(target_object);
		}
		else {
			// Find argument by select
			//

			MObjectArray selected;

			if (!(status = Model::Base::AllSelected(selected))) {
				status.perror("Helix::AllSelected");
				return status;
			}

			std::copy(&selected[0], &selected[0] + selected.length(), std::back_insert_iterator< std::list<Model::Base> >(targets));
		}

		default_length = bases;

		m_operation.setLength(bases);

		std::for_each(targets.begin(), targets.end(), m_operation.execute());

		return m_operation.status();
	}

	MStatus ExtendStrand::undoIt () {
		return m_operation.undo();
	}

	MStatus ExtendStrand::redoIt () {
		return m_operation.redo();
	}

	bool ExtendStrand::isUndoable () const {
		return true;
	}

	bool ExtendStrand::hasSyntax () const {
		return true;
	}

	MSyntax ExtendStrand::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-b", "-base", MSyntax::kLong);
		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *ExtendStrand::creator() {
		return new ExtendStrand();
	}

	void ExtendStrand::ExtendStrandWithProgressBar::onProgressBegin(int range) {
		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("Extend strand");
		MProgressWindow::setProgressStatus("Generating bases...");
		MProgressWindow::setProgressRange(0, range);
		MProgressWindow::startProgress();
	}

	void ExtendStrand::ExtendStrandWithProgressBar::onProgressStep() {
		MProgressWindow::advanceProgress(1);
	}

	void ExtendStrand::ExtendStrandWithProgressBar::onProgressDone() {
		MProgressWindow::endProgress();
	}
}
