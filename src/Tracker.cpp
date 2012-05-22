/*
 * HelixTracker.cpp
 *
 *  Created on: 20 jun 2011
 *      Author: johan
 */

#include <Locator.h>
#include <Tracker.h>

#include <Helix.h>
#include <HelixBase.h>
#include <Utility.h>

#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlugArray.h>

namespace Helix {
	MDagPathArray HelixTracker::s_selectedBases, HelixTracker::s_selectedNeighbourBases, HelixTracker::s_selectedFivePrimeBases, HelixTracker::s_selectedThreePrimeBases, HelixTracker::s_selectedAdjacentBases;

	HelixTracker::HelixTracker() {

	}

	HelixTracker::~HelixTracker() {

	}

	void *HelixTracker::creator() {
		return new HelixTracker();
	}

	MStatus HelixTracker::recursiveSearchForNeighbourBases(MDagPath dagPath, MObject forward_attribute, MObject backward_attribute, MDagPathArray endBases, bool force) {
		MStatus status;

		unsigned int selectedNeighbourBases_length = s_selectedNeighbourBases.length();

		if (!force) {
			for (unsigned int i = 0; i < selectedNeighbourBases_length; ++i) {
				if (s_selectedNeighbourBases[i] == dagPath) {
					// Already exists, we're not going to work anymore with this node
					return MStatus::kSuccess;
				}
			}
		}

		s_selectedNeighbourBases.append(dagPath);

		MObject node = dagPath.node(&status);

		if (!status) {
			status.perror("MDagPath::node");
			return status;
		}

		MPlug labelPlug(dagPath.node(), HelixBase::aLabel);

		if (labelPlug.isConnected(&status)) {
			MPlugArray plugArray;

			if (labelPlug.connectedTo(plugArray, true, true, &status)) {
				unsigned int plugArray_length = plugArray.length();

				for (unsigned int i = 0; i < plugArray_length; ++i) {

					if (plugArray[i] == HelixBase::aLabel) {
						MFnDagNode target_dagNode(plugArray[i].node(&status));
						MDagPath target_dagPath;

						if (!status) {
							status.perror("MPlug::node");
							return status;
						}

						if (!(status = target_dagNode.getPath(target_dagPath))) {
							status.perror("MFnDagNode::dagPath");
							return status;
						}

						s_selectedAdjacentBases.append(target_dagPath);
					}
				}
			}

			if (!status) {
				status.perror("MPlug::connectedTo");
				return status;
			}
		}

		if (!status) {
			status.perror("MPlug::isConnected");
			return status;
		}

		MPlug iteratorPlug(dagPath.node(), forward_attribute);

		bool hasConnections = false;

		if (iteratorPlug.isConnected(&status)) {
			MPlugArray plugArray;

			if (iteratorPlug.connectedTo(plugArray, true, true, &status)) {
				unsigned int plugArray_length = plugArray.length();

				for(unsigned int i = 0; i < plugArray_length; ++i) {
					MObject targetObject = plugArray[i].node(&status);

					if (!status) {
						status.perror("MPlugArray[i]::node");
						return status;
					}

					MFnDagNode target_dagNode(targetObject);

					if (plugArray[i] == backward_attribute && target_dagNode.typeName(&status) == HELIX_HELIXBASE_NAME) {
						MDagPath target_dagPath;

						if (!(status = target_dagNode.getPath(target_dagPath))) {
							status.perror("MFnDagNode::getPath");
							return status;
						}

						// This is a valid HelixBase target, lets iterate over it

						hasConnections = true;
						recursiveSearchForNeighbourBases(target_dagPath, forward_attribute, backward_attribute, endBases);
					}

					if (!status) {
						status.perror("MFnDagNode::typeName");
						return status;
					}
				}
			}
		}

		if (!hasConnections) {
			endBases.append(dagPath);
		}

		if (!status) {
			status.perror("MPlug::isConnected");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus HelixTracker::doIt(const MArgList & args) {
		MStatus status;

		// Remove all the currently selected HelixBase's
		//

		s_selectedBases.clear();
		s_selectedNeighbourBases.clear();
		s_selectedFivePrimeBases.clear();
		s_selectedThreePrimeBases.clear();
		s_selectedAdjacentBases.clear();

		// Find all the selected HelixBase's
		//

		MObjectArray selectedBase_objects;

		if (!(status = SelectedBases(selectedBase_objects))) {
			status.perror("SelectedBases");
			return status;
		}

		// Now iterate over the selectedBases and figure out what neighbours we have and if they're end bases
		//

		for(unsigned int i = 0; i < selectedBase_objects.length(); ++i) {
			MDagPath dagPath;

			MFnDagNode dagNode(selectedBase_objects[i]);

			if (!(status = dagNode.getPath(dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			recursiveSearchForNeighbourBases(dagPath, HelixBase::aForward, HelixBase::aBackward, s_selectedFivePrimeBases);
			recursiveSearchForNeighbourBases(dagPath, HelixBase::aBackward, HelixBase::aForward, s_selectedThreePrimeBases, true);

			s_selectedBases.append(dagPath);
		}

		// Make Maya rerender the scene

		if (!(status = MGlobal::executeCommand("refresh", false))) {
			status.perror("MGlobal::executeCommand('refresh')");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus HelixTracker::undoIt () {
		std::cerr << "I can't undo that! :)" << std::endl;

		return MStatus::kSuccess;
	}

	MStatus HelixTracker::redoIt () {
		std::cerr << "I don't want to redo that! :)" << std::endl;

		return MStatus::kSuccess;
	}

	bool HelixTracker::isUndoable () const {
		return false;
	}

	bool HelixTracker::hasSyntax () const {
		return false;
	}

	MStatus HelixTracker::registerOnSelectionChangedListener() {
		return MGlobal::executeCommand(MEL_REGISTER_ONSELECTIONCHANGED_COMMAND_NATIVE, false);
	}

	MStatus HelixTracker::deregisterOnSelectionChangedListener() {
		return MGlobal::executeCommand(MEL_DEREGISTER_ONSELECTIONCHANGED_COMMAND_NATIVE, false);
	}
}
