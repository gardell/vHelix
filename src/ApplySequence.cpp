/*
 * ApplySequence.cpp
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#include <ApplySequence.h>
#include <DNA.h>
#include <Utility.h>
#include <HelixBase.h>

#include <model/Helix.h>
#include <model/Strand.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <maya/MGlobal.h>

namespace Helix {
	ApplySequence::ApplySequence() {

	}

	ApplySequence::~ApplySequence() {

	}

	MStatus ApplySequence::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		Model::Base target;
		MString sequence;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t")) {
			MString targetName;

			if (!(status = argDatabase.getFlagArgument("-t", 0, targetName))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;

			if (!(status = selectionList.add(targetName))) {
				status.perror("ApplySequence::doIt: MSelectionList::add");
				return status;
			}

			MObject target_object;

			MDagPath target_dagPath;
			
			if (!(status = selectionList.getDagPath(0, target_dagPath, target_object))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}

			target = target_dagPath;
		}
		else {
			// Find argument by select
			//

			MObjectArray selectedBases;

			if (!(status = Model::Base::AllSelected(selectedBases))) {
				status.perror("Helix::AllSelected");
				return status;
			}

			if (selectedBases.length() == 0) {
				MGlobal::displayError("No bases to apply the sequence to");
				return MStatus::kFailure;
			}
			else if (selectedBases.length() > 1) {
				MGlobal::displayError("Too many bases to apply sequences to, select one base only");
				return MStatus::kFailure;
			}
			else {
				MFnDagNode dagNode(selectedBases[0]);
				MDagPath target_dagPath;

				if (!(status = dagNode.getPath(target_dagPath))) {
					status.perror("MFnDagNode::getPath");
					return status;
				}

				target = target_dagPath;
			}
		}

		// Load the sequence

		if (argDatabase.isFlagSet("-s", &status)) {
			if (!(status = argDatabase.getFlagArgument("-s", 0, sequence))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}

		if (sequence.length() == 0) {
			MGlobal::displayError("No valid DNA sequence given");
			return MStatus::kFailure;
		}

		m_operation.setSequence(sequence);

		Model::Strand strand(target);
		std::for_each(strand.forward_begin(), strand.forward_end(), m_operation.execute());

		return m_operation.status();
	}

	MStatus ApplySequence::undoIt () {
		return m_operation.undo();
	}

	MStatus ApplySequence::redoIt () {
		return m_operation.redo();
	}

	bool ApplySequence::isUndoable () const {
		return true;
	}

	bool ApplySequence::hasSyntax () const {
		return true;
	}

	MSyntax ApplySequence::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);
		syntax.addFlag("-s", "-sequence", MSyntax::kString);

		return syntax;
	}

	void *ApplySequence::creator() {
		return new ApplySequence();
	}
}
