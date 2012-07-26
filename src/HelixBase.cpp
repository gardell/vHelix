/*
 * HelixBase.cpp
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#include <HelixBase.h>
#include <DNA.h>
#include <Utility.h>
#include <TargetHelixBaseBackward.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MPlugArray.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MFileIO.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MModelMessage.h>

#include <list>
#include <algorithm>

#include <model/Base.h>
#include <view/ConnectSuggestionsLocatorNode.h>

namespace Helix {
	/*void MNodeMessage_preRemovalCallbackFunc(MObject & node, void *clientData) {
		/*
		 * Find backward connected if any
		 *
		MStatus status;
		Model::Base backward = Model::Base(node).backward(status);

		if (!status && status != MStatus::kNotFound)
			status.perror("Base::backward");
		
		if (status)
			static_cast<HelixBase *> (MFnDagNode(backward.getObject(status)).userNode())->onForwardConnectedNodePreRemoval(node);

		/*HelixBase & base = *static_cast<HelixBase *> (clientData);
		base.m_nodeIsBeingRemoved = true;
		base.onRemovePreConnectionBroken();*
	}

	void MModelMessage_removedFromModelCallbackFunc(MObject & node, void *clientData) {
		/*HelixBase & base = *static_cast<HelixBase *> (clientData);
		base.onRemovePostConnectionBroken();
		base.m_nodeIsBeingRemoved = false;*
	}

	void MModelMessage_addedToModelCallbackFunc(MObject & node, void *clientData) {
		/*HelixBase & base = *static_cast<HelixBase *> (MFnDagNode(node).userNode());

		base.onPreConnectionMade(base.m_nodeRevived);

		/*
		 * The next trigger will be due to an undo thus reviving the node. First call will be due to creation of the node and has this set to false
		 *
		base.m_nodeRevived = true;*
	}*/

	MObject HelixBase::aForward, HelixBase::aBackward, HelixBase::aLabel;
	MTypeId HelixBase::id(HELIX_HELIXBASE_ID);

	HelixBase::HelixBase()/* : m_nodeRevived(false), m_nodeIsBeingRemoved(false)*/ {

	}

	HelixBase::~HelixBase() {

	}

	//void HelixBase::postConstructor() {
		// See the callback definition at the end of the file for information about what it does.
		//

		/*MStatus status;
		MObject thisObject = thisMObject();

		MNodeMessage::addNodePreRemovalCallback(thisObject, &MNodeMessage_preRemovalCallbackFunc, this, &status);

		if (!status)
			status.perror("MNodeMessage::addNodePreRemovalCallback");

		MModelMessage::addNodeRemovedFromModelCallback(thisObject, &MModelMessage_removedFromModelCallbackFunc, this, &status);

		if (!status)
			status.perror("MModelMessage::addNodeRemovedFromModelCallback");

		MModelMessage::addNodeAddedToModelCallback(thisObject, &MModelMessage_addedToModelCallbackFunc, this, &status);

		if (!status)
			status.perror("MModelMessage::addNodeAddedToModelCallback");*/
	//}

	//MStatus HelixBase::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc) {
		/*if (plug == HelixBase::aForward && otherPlug == HelixBase::aBackward) {
			MStatus status;

			onForwardConnectionMade(otherPlug.node(&status));

			if (!status)
				status.perror("MPlug::node");
		}*/

	//	return MPxTransform::connectionMade(plug, otherPlug, asSrc);
	//}

	//MStatus HelixBase::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc) {
		/*if (plug == aForward && otherPlug == aBackward) {
			MStatus status;

			onForwardConnectionBroken(otherPlug.node(&status));

			if (!status)
				status.perror("MPlug::node");
		}*/

	//	return MPxTransform::connectionBroken(plug, otherPlug, asSrc);
	//}

	void *HelixBase::creator() {
		return new HelixBase();
	}

	MStatus HelixBase::initialize() {
		MStatus stat;

		MFnNumericAttribute forwardAttr, backwardAttr, locatorAttr, helixForwardAttr, helixBackwardAttr, colorAttr;
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
			const char label[] = { DNA::Name(i).toChar(), '\0' };
			labelAttr.addField(label, i);
		}

		addAttribute(aForward);
		addAttribute(aBackward);
		addAttribute(aLabel);

		/*
		 * Try to set DisconnectBehavior on the rotateX, rotateY, rotateZ attributes
		 */

		/*MFnAttribute rotateXAttr(MPxTransform::rotateX, &stat);

		if (!stat) {
			stat.perror("MFnAttribute::#ctor 1");
			return stat;
		}

		if (!(stat = rotateXAttr.setDisconnectBehavior(MFnAttribute::kReset))) {
			stat.perror("MFnAttribute::setDisconnectBehavior 1");
			return stat;
		}

		MFnAttribute rotateYAttr(MPxTransform::rotateY, &stat);

		if (!stat) {
			stat.perror("MFnAttribute::#ctor 2");
			return stat;
		}

		if (!(stat = rotateYAttr.setDisconnectBehavior(MFnAttribute::kReset))) {
			stat.perror("MFnAttribute::setDisconnectBehavior 2");
			return stat;
		}

		MFnAttribute rotateZAttr(MPxTransform::rotateZ, &stat);

		if (!stat) {
			stat.perror("MFnAttribute::#ctor 3");
			return stat;
		}

		if (!(stat = rotateZAttr.setDisconnectBehavior(MFnAttribute::kReset))) {
			stat.perror("MFnAttribute::setDisconnectBehavior 3");
			return stat;
		}*/

		return MStatus::kSuccess;
	}

	/*void HelixBase::onRemovePreConnectionBroken() {
		std::cerr << __FUNCTION__ << std::endl;
	}

	void HelixBase::onRemovePostConnectionBroken() {
		std::cerr << __FUNCTION__ << std::endl;
	}

	void HelixBase::onPreConnectionMade(bool revived) {
		if (!revived) {
			std::cerr << __FUNCTION__ << " first time creation" << std::endl;
		}
		else
			std::cerr << __FUNCTION__ << " after undo" << std::endl;
	}

	void HelixBase::onForwardConnectionMade(MObject & target) {
		MStatus status;
		MString target_fullPathName = MFnDagNode(target).fullPathName(), this_fullPathName = MFnDagNode(thisMObject()).fullPathName();

		/*if (!(status = MGlobal::executeCommand(MString("delete -cn ") + this_fullPathName + "; setAttr " + this_fullPathName + ".rotate 0 0 0; aimConstraint -aimVector 0 0 -1.0 " + target_fullPathName + " " + this_fullPathName + ";", false)))
			status.perror("MGlobal::executeCommand");*
	}

	/*void HelixBase::onForwardConnectionBroken(MObject & target) {
		std::cerr << __FUNCTION__ << std::endl;

		if (!isNodeBeingRemoved()) {
			/* 
			 * If this is due to our target being deleted, there's not much we're allowed to do
			 *

			MStatus status;

			HelixBase & target_base = *static_cast<HelixBase *> (MFnDagNode(target).userNode(&status));

			if (!status) {
				status.perror("MFnDagNode::userNode");
			}

			MString this_fullPathName = MFnDagNode(thisMObject()).fullPathName();

			if (target_base.isNodeBeingRemoved()) {
				/*
				 * Even though we're not interrested in accessing the node being deleted, Maya still crashes if we try to access any other nodes
				 * By using executeCommandOnIdle we can avoid any crashes and just get some MEL error that we won't care about anyway
				 * since the action was made due to a deletion, there's no chance that the delayed execution can mess up any upcoming connections
				 * such as the ones created by the connect tool (up to 2 disconnects followed by a connect)
				 */
				/*if (!(status = MGlobal::executeCommandOnIdle(MString(MEL_TARGET_HELIXBASE_BACKWARD " -base ") + this_fullPathName, false)))
					status.perror("MGlobal::executeCommandOnIdle");*/

				/*
				 * This will be handled by the event below. As Maya didn't allow us to do anything here
				 *

				return;
			}

			/*
			 * This node is not being removed from the scene, we should be able to set up our aimConstraint.
			 * Find our previous object
			 *

			if (!(status = MGlobal::executeCommand(MString("delete -cn ") + this_fullPathName + "; setAttr " + this_fullPathName + ".rotate 0 0 0; $backwards = `listConnections " + this_fullPathName + ".backward`; for($backward in $backwards) aimConstraint -aimVector 1.0 0 0 $backward " + this_fullPathName + ";", false)))
				status.perror("MGlobal::executeCommand");
		}
	}*/

	//void HelixBase::onForwardConnectedNodePreRemoval(MObject & target) {
		/*
		 * Make perpendicular
		 */

		//MStatus status;
		//MString this_fullPathName = MFnDagNode(thisMObject()).fullPathName();//, target_fullPathName = MFnDagNode(target).fullPathName();

		/*if (!(status = MGlobal::executeCommandOnIdle(MString("delete -cn ") + this_fullPathName + "; setAttr " + this_fullPathName + ".rotate 0 0 0; $backwards = `listConnections " + this_fullPathName + ".backward`; for($backward in $backwards) aimConstraint -aimVector 1.0 0 0 $backward " + this_fullPathName + ";", false)))
			status.perror("MGlobal::executeCommand");*/
		/*if (!(status = MGlobal::executeCommandOnIdle(MString(MEL_TARGET_HELIXBASE_BACKWARD " -base ") + this_fullPathName, false)))
			status.perror("MGlobal::executeCommandOnIdle");*/

		/*if (!(status = MGlobal::executeCommand(MString("delete -cn ") + this_fullPathName, false)))
			status.perror("MGlobal::executeCommand");

		MFnTransform transform(thisMObject());

		double rotation[] = { 90.0, 0.0, 0.0 };
		if (!(status = transform.rotateBy(rotation, MTransformationMatrix::kXYZ))) {
			status.perror("MFnTransform::rotateBy");
			return;
		}*/
	//}
}
