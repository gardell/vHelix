/*
 * Duplicate.cpp
 *
 *  Created on: Aug 3, 2011
 *      Author: bjorn
 */

#include <Duplicate.h>
#include <ToggleCylinderBaseView.h>

#include <Helix.h>

#include <maya/MSelectionList.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MItDag.h>
#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>
#include <maya/MProgressWindow.h>
#include <maya/MFnDagNode.h>

#include <model/Helix.h>

namespace Helix {
	Duplicate::Duplicate() {

	}

	Duplicate::~Duplicate() {

	}

	void Duplicate::DuplicateWithProgressBar::onProgressStart(unsigned int process, unsigned int range) {
		static const char *msg[] = { "Generating new helices and bases...", "Connecting bases..." };

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("Duplicate helices");
		MProgressWindow::setProgressStatus(msg[process]);
		MProgressWindow::setProgressRange(0, range);
		MProgressWindow::startProgress();
	}

	void Duplicate::DuplicateWithProgressBar::onProgressStep() {
		MProgressWindow::advanceProgress(1);
	}

	void Duplicate::DuplicateWithProgressBar::onProgressDone() {
		MProgressWindow::endProgress();
	}

	MStatus Duplicate::doIt(const MArgList & args) {
		// Find out our targets. First either by -target or selection
		//

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		//m_target_helices.clear();
		MObjectArray target_helices;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t", &status)) {
			MString target_str;

			if (!(status = argDatabase.getFlagArgument("-t", 0, target_str))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;
			if (!(status = selectionList.add(target_str))) {
				status.perror("Duplicate::doIt: MSelectionList::add");
				return status;
			}

			MObject target;

			if (!(status = selectionList.getDependNode(0, target))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			if (!(status = target_helices.append(target))) {
				status.perror("MObjectArray::append");
				return status;
			}
		}

		if (target_helices.length() == 0) {
			// Find out by select instead

			if (!(status = Model::Helix::AllSelected(target_helices))) {
				status.perror("Helix::AllSelected");
				return status;
			}

			if (target_helices.length() == 0) {
				// If there's still no selected helices, duplicate the whole scene

				MItDag dagIt(MItDag::kBreadthFirst, MFn::kTransform, &status);

				for(; !dagIt.isDone(); dagIt.next()) {
					MObject node = dagIt.currentItem(&status);

					if (!status) {
						status.perror("MItDag::currentItem");
						return status;
					}

					MFnDagNode node_dagNode(node);

					if (node_dagNode.typeId(&status) == Helix::id)
						target_helices.append(node);
				}
			}
		}

		// Now find out, if there's any other connected helices connecting to one of ours. In that case, we duplicate them too
		//

		MObjectArray all_target_helices;

		if (!(status = Model::Helix::GetRelatives(target_helices, all_target_helices))) {
			status.perror("Helix::GetRelatives");
			return status;
		}

		if (!(status = m_operation.duplicate(all_target_helices))) {
			status.perror("Controller::Duplicate::duplicate");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Duplicate::undoIt () {
		return m_operation.undo();
	}

	MStatus Duplicate::redoIt () {
		return m_operation.redo();
	}

	bool Duplicate::isUndoable () const {
		return true;
	}

	bool Duplicate::hasSyntax () const {
		return true;
	}

	MSyntax Duplicate::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *Duplicate::creator() {
		return new Duplicate();
	}
}
