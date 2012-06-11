/*
 * HelixBase.cpp
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#include <HelixBase.h>
#include <DNA.h>
#include <Utility.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MPlugArray.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MFileIO.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>

namespace Helix {
	//
	// FIXME: To fix the crashes, we need to have a callback for both the pending deletion and the deleted events
	// During the deletion, a bool will be set, and no aimconstraint operations can be executed
	// All requests to aimconstraints must be saved to be executed later in the deleted event
	// Try to execute the aimconstraint in that event, but there's a chance that won't work either in case we're deleting the whole TREE
	// Check that! If that's the case we're upp for some trouble
	// Note that we DO want to execute the aimconstraint directly if the node's NOT being deleted (cause the callback will never be triggered ofc)
	// But also because subsequent connects (as a reconnect spawn disconnect and after a new connect)

	//
	// There's a bug in Maya (or lack of a feature?) that makes it impossible to execute certain commands in the connectionBroken method
	// cause it will crash Maya if the node is the way of being deleted. Here we try to hook up to some callbacks to see if there's some way
	// to get around this issue.
	//

	// Yes, a global variable here is not multithread aware etc, but (i think) there's no multithreading being done by Maya when nodes are deleted anyway
	// Since there's no way in Maya to obtain an MPx*-object from a MObject, this is the easiest and fastest way of tracking the connected base
	MObject BaseBeingRemoved;
	bool BaseDeletionInProgress = false;


	void MNode_NodePreRemovalCallbackFunc(MObject & node, void *clientData) {
		BaseBeingRemoved = node;
		BaseDeletionInProgress = true;

		std::cerr << __FUNCTION__ << std::endl;
	}

	MObject HelixBase::aForward, HelixBase::aBackward, HelixBase::aLabel;//, HelixBase::aHelix_Backward, HelixBase::aHelix_Forward;
	MTypeId HelixBase::id(HELIX_HELIXBASE_ID);

	HelixBase::HelixBase() {

	}

	HelixBase::~HelixBase() {

	}

	void HelixBase::postConstructor() {
		// See the callback definition at the end of the file for information about what it does.
		//

		MStatus status;
		MObject thisObject = thisMObject();

		MNodeMessage::addNodePreRemovalCallback(thisObject, &MNode_NodePreRemovalCallbackFunc, this, &status);
		
		if (!status)
			status.perror("MNodeMessage::addNodePreRemovalCallback");
	}

	MStatus HelixBase::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc) {
		if (plug == HelixBase::aForward && otherPlug == HelixBase::aBackward) {
			// This base has a new target, retarget its pointing arrow
			MObject thisObject = thisMObject();

			// Don't apply stuff if the node is set for deletion. The reason for this is a bug in Maya, see the callback that sets this parameter for more information

			/*if (BaseBeingRemoved == thisObject) {
				std::cerr << "ConnectionMade: isAboutToDelete or we're the opposite, canceling.. " << std::endl;

				return MPxTransform::connectionMade(plug, otherPlug, asSrc);
			}*/

			MStatus status;
			MFnDagNode this_dagNode(thisObject), target_dagNode(otherPlug.node(&status));

			if (!status) {
				status.perror("MPlug::node");
				return status;
			}

			MDagPath this_dagPath, target_dagPath;

			if (!(status = this_dagNode.getPath(this_dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			if (!(status = target_dagNode.getPath(target_dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			/*if (!(status = HelixBase_RemoveAllAimConstraints(thisObject))) {
				status.perror("removeAllAimConstraints");
				return status;
			}*/

			// Create aimConstraint
			//

			// DON'T USE executeCommandOnIdle since it will be executed too late in case of a disconnect followed by a new connect (will mess up everything)
			// Also, OnIdle's are undoable, which gets VERY weird

			/*if (!(status = MGlobal::executeCommand(MString("aimConstraint -aimVector 0 0 -1.0 ") + target_dagPath.fullPathName().asChar() + " " + this_dagPath.fullPathName().asChar() + ";"))) {
				status.perror("MGlobal::executeCommandOnIdle");
				return status;
			}*/

			if (!(status = MGlobal::executeCommand(MString("retargetBase -base ") + this_dagPath.fullPathName() + " -target " + target_dagPath.fullPathName() + ";", false))) {
				status.perror("MGlobal::executeCommandOnIdle");
				return status;
			}
		}

		return MPxTransform::connectionMade(plug, otherPlug, asSrc);
	}

	MStatus HelixBase::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc) {
		if (plug == aForward && otherPlug == aBackward) {
			MStatus status;
			MObject thisObject = thisMObject();

			// FIXME: There's currently a bug that makes Maya crash if we try to make an aimconstraint on a node that is currently being deleted
			// FIXME: There's also another bug that won't allow us to call removeAllAimConstraints because there _might_ be a new scene-operation, and in that case, Maya crashes

			// Remove constraints
			//

			if (BaseBeingRemoved == thisObject)
				return MPxTransform::connectionBroken(plug, otherPlug, asSrc);

			if (!BaseDeletionInProgress) {
				if (!(status = HelixBase_RemoveAllAimConstraints(thisObject))) {
					status.perror("removeAllAimConstraints");
					return status;
				}
			}

			// Create aimConstraint (perpendicular to the backward base makes it look clearly disconnected)
			//
			// Find the backward target
			//

			MPlug backwardPlug(thisObject, HelixBase::aBackward);
			MPlugArray backward_connections;
			backwardPlug.connectedTo(backward_connections, true, true, &status);

			if (!status) {
				status.perror("MPlug::connectedTo");
				return status;
			}

			if (backward_connections.length() > 0) {
				MFnDagNode this_dagNode(thisObject), target_dagNode(backward_connections[0].node(&status));

				if (!status) {
					status.perror("MPlug::node");
					return status;
				}

				/*if (BaseDeletionInProgress) {
					// We can't do aimConstraints when a deletion is being made and has to postpone it to the callback triggered when the deletion is done

					// FIXME: This call creates an error message if the user has requested the whole helix to be deleted
					// That can be fixed by defining a new command that does this, and that doesn't execute the aimConstraint if the targets don't exist

					if (!(status = MGlobal::executeCommandOnIdle(MString("aimConstraint -aimVector 1.0 0 0 ") + target_dagNode.fullPathName() + " " + this_dagNode.fullPathName() + ";", false))) {
						status.perror("MGlobal::executeCommandOnIdle");
						return status;
					}
				}
				else {
					// DON'T USE executeCommandOnIdle since it will be executed too late in case of a disconnect followed by a new connect (will mess up everything)
					if (!(status = MGlobal::executeCommand(MString("aimConstraint -aimVector 1.0 0 0 ") + target_dagNode.fullPathName() + " " + this_dagNode.fullPathName() + ";", false))) {
						status.perror("MGlobal::executeCommand");
						return status;
					}
				}*/

				if (BaseDeletionInProgress) {
					std::cerr << "RETARGETING BASE AS AN ONIDLE COMMAND" << std::endl;

					if (!(status = MGlobal::executeCommandOnIdle(MString("retargetBase -perpendicular 1 -base ") + this_dagNode.fullPathName() + " -target " + target_dagNode.fullPathName() + ";", false))) {
						status.perror("MGlobal::executeCommandOnIdle");
						return status;
					}

					std::cerr << "RETARGETING BASE AS AN ONIDLE COMMAND DONE" << std::endl;

					BaseDeletionInProgress = false;
				}
				else {
					std::cerr << "RETARGETING BASE IMMEDIATELY" << std::endl;
					if (!(status = MGlobal::executeCommand(MString("retargetBase -perpendicular 1 -base ") + this_dagNode.fullPathName() + " -target " + target_dagNode.fullPathName() + ";", false))) {
						status.perror("MGlobal::executeCommandOnIdle");
						return status;
					}

					std::cerr << "RETARGETING BASE IMMEDIATELY DONE" << std::endl;
				}
			}
		}

		return MPxTransform::connectionBroken(plug, otherPlug, asSrc);
	}


	void *HelixBase::creator() {
		return new HelixBase();
	}

	MStatus HelixBase::initialize() {
		MStatus stat;

		MFnNumericAttribute forwardAttr, backwardAttr, locatorAttr, helixForwardAttr, helixBackwardAttr;
		MFnEnumAttribute labelAttr;

		aForward = forwardAttr.create("forward", "fw", MFnNumericData::kLong, 0, &stat);

		if (!stat) {
			stat.perror("MFnNumericAttribute::create for forward");
			return stat;
		}

		aBackward = backwardAttr.create("backward", "bw", MFnNumericData::kLong, 0, &stat);

		if (!stat) {
			stat.perror("MFnNumericAttribute::create for backward");
			return stat;
		}

		aLabel = labelAttr.create("label", "lb", (short int) DNA::Invalid, &stat);

		if (!stat) {
			stat.perror("MFnEnumAttribute::create");
			return stat;
		}

		for (int i = 0; i < DNA::BASES; ++i) {
			const char label[] = { DNA::ToChar((DNA::Names) i), '\0' };
			labelAttr.addField(label, i);
		}

		// helix_forward and helix_backward are the bindings when the model was created using createHelix, duplicate or import
		// it is used by the exporter to find the original helices
		//

		/*aHelix_Forward = helixForwardAttr.create("helix_forward", "hfw", MFnNumericData::kLong, 0, &stat);

		if (!stat) {
			stat.perror("MFnNumericAttribute::create for helix forward");
			return stat;
		}

		aHelix_Backward = helixBackwardAttr.create("helix_backward", "hbw", MFnNumericData::kLong, 0, &stat);

		if (!stat) {
			stat.perror("MFnNumericAttribute::create for helix backward");
			return stat;
		}*/

		/*aLocator = locatorAttr.create("locator", "loc", MFnNumericData::kLong, 0, &stat);

		if (!stat) {
			stat.perror("MFnNumericAttribute::create for locator");
			return stat;
		}*/

		addAttribute(aForward);
		addAttribute(aBackward);
		addAttribute(aLabel);
		//addAttribute(aHelix_Forward);
		//addAttribute(aHelix_Backward);

		return MStatus::kSuccess;
	}
}
