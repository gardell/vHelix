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

	MStatus ApplySequence::applySequence(const MString & sequence, const MDagPath & target) {
		MStatus status;

		m_modifiedLabels.clear();
		m_appliedSequence = sequence;
		m_appliedTarget = target;

		// Ok we have a target and a string to apply
		//

		MObject target_object = target.node(&status);

		if (!status) {
			status.perror("MDagPath::node");
			return status;
		}

		MDagPath itDagPath = target;
		MObject itObject;

		size_t i = 0;
		bool isFirstIt = true; // Not clean, but the easiest way

		do {
			itObject = itDagPath.node(&status);

			if (!status) {
				status.perror("MDagPath::node");
				return status;
			}

			if (!isFirstIt) {
				if (itObject == target_object)
					break;
			}
			else
				isFirstIt = false;

			// Get the value for the base (A,T,G or C)

			DNA::Names base_value = DNA::ToName(sequence.asChar()[i]);

			MPlug labelPlug(itObject, HelixBase::aLabel);

			//bool isSource = labelPlug.isSource(&status);
			bool isDestination = labelPlug.isDestination(&status);

			if (!status) {
				status.perror("MPlug::isDestination");
				return status;
			}

			//std::cerr << "Applying " << sequence.asChar() [i] << " to " << itDagPath.fullPathName().asChar() << std::endl;

			if (!isDestination) {
				// If this base's label is a source connection, apply the value to the label

				// Copy the old value into our modifiedLabels-array for the undo method
				//

				int old_base_value;
				if (!(status = labelPlug.getValue(old_base_value))) {
					status.perror("MPlug::getValue");
					return status;
				}

				m_modifiedLabels.push_back(std::make_pair(itObject, old_base_value));


				// Now apply the value

				if (!(status = labelPlug.setInt((int) base_value))) {
					status.perror("MPlug::setInt");
					return status;
				}
			}
			else {
				// If it's a destination connection, we must apply the opposite value to the opposite base!

				MDagPath opposite_dagPath;

				if (!HelixBase_NextBase(itObject, HelixBase::aLabel, opposite_dagPath, &status)) {
					std::cerr << "Failed to find the opposite base!" << std::endl;
					return MStatus::kFailure;
				}

				if (!status) {
					status.perror("HelixBase_NextBase label");
					return status;
				}

				MObject opposite_object = opposite_dagPath.node(&status);

				if (!status) {
					status.perror("MDagPath::node");
					return status;
				}

				MPlug opposite_labelPlug(opposite_object, HelixBase::aLabel);

				if (!status) {
					status.perror("MDagPath::node");
					return status;
				}

				// Copy the old value into our modifiedLabels-array for the undo method
				//

				int old_base_value;
				if (!(status = opposite_labelPlug.getValue(old_base_value))) {
					status.perror("MPlug::getValue");
					return status;
				}

				m_modifiedLabels.push_back(std::make_pair(opposite_object, old_base_value));


				// Now apply the value

				if (!(status = opposite_labelPlug.setInt((int) DNA::OppositeBase(base_value)))) {
					status.perror("MPlug::setInt opposite");
					return status;
				}
			}

			if (sequence.length() == ++i)
				break;
		}
		while (HelixBase_NextBase(itObject, HelixBase::aForward, itDagPath, &status));

		if (!status) {
			status.perror("HelixBase_NextBase");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus ApplySequence::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		MDagPath target;
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
				status.perror("MSelectionList::add");
				return status;
			}

			MObject target_object;

			if (!(status = selectionList.getDagPath(0, target, target_object))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}
		}
		else {
			// Find argument by select
			//

			MObjectArray selectedBases;

			if (!(status = SelectedBases(selectedBases))) {
				status.perror("SelectedBases");
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

				if (!(status = dagNode.getPath(target))) {
					status.perror("MFnDagNode::getPath");
					return status;
				}
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

		return applySequence(sequence, target);
	}

	MStatus ApplySequence::undoIt () {
		MStatus status;
		// We save all the operations into this array, so the only thing we have to do is reapply the labels again..

		for(std::list<std::pair<MObject, int> >::iterator it = m_modifiedLabels.begin(); it != m_modifiedLabels.end(); ++it) {
			MPlug plug(it->first, HelixBase::aLabel);

			if (!(status = plug.setInt(it->second))) {
				status.perror("MPlug::setInt");
				return status;
			}
		}

		return MStatus::kSuccess;
	}

	MStatus ApplySequence::redoIt () {
		return applySequence(m_appliedSequence, m_appliedTarget);
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
