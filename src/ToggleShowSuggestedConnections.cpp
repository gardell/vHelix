#include <ToggleShowSuggestedConnections.h>
#include <view/ConnectSuggestionsLocatorNode.h>

#include <maya/MSyntax.h>
#include <maya/MFnDagNode.h>

namespace Helix {
	void MNodeMessage_locatorNode_nodePreRemovalCallback(MObject& node,void *clientData) {
		ToggleShowSuggestedConnections & toggle = *static_cast<ToggleShowSuggestedConnections *>(clientData);
		
		toggle.s_locatorNode = MObject::kNullObj;
		MNodeMessage::removeCallback(toggle.s_locatorNode_preRemovalCallbackId);
	}

	MObject ToggleShowSuggestedConnections::s_locatorNode(MObject::kNullObj);
	MCallbackId ToggleShowSuggestedConnections::s_locatorNode_preRemovalCallbackId = 0;

	ToggleShowSuggestedConnections::ToggleShowSuggestedConnections() {

	}

	ToggleShowSuggestedConnections::~ToggleShowSuggestedConnections() {

	}

	MStatus ToggleShowSuggestedConnections::doIt(const MArgList & args) {
		return redoIt();
	}

	MStatus ToggleShowSuggestedConnections::undoIt () {
		return redoIt();
	}

	MStatus ToggleShowSuggestedConnections::redoIt () {
		MStatus status;

		if (s_locatorNode.isNull()) {
			/*
			 * Create a locatorNode
			 */

			MFnDagNode dagNode;

			s_locatorNode = dagNode.create(View::ConnectSuggestionsLocatorNode::id, MObject::kNullObj, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			s_locatorNode_preRemovalCallbackId = MNodeMessage::addNodePreRemovalCallback(s_locatorNode, MNodeMessage_locatorNode_nodePreRemovalCallback, this, &status);

			if (!status) {
				status.perror("MNodeMessage::addNodePreRemovalCallback");
				return status;
			}
		}
		else {
			/*
			 * Delete the locatorNode
			 */

			MDGModifier dgModifier;

			if (!(status = dgModifier.deleteNode(s_locatorNode))) {
				status.perror("MDGModifier::deleteNode");
				return status;
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}

			s_locatorNode = MObject::kNullObj;

			/*
			 * Notice that the preRemovalCallback will be triggered now and it will thus set m_locatorNode = MObject::kNullObj;
			 * and also unregister the callback
			 */
		}

		return MStatus::kSuccess;
	}

	bool ToggleShowSuggestedConnections::isUndoable () const {
		return true;
	}

	bool ToggleShowSuggestedConnections::hasSyntax () const {
		return true;
	}

	MSyntax ToggleShowSuggestedConnections::newSyntax () {
		return MSyntax();
	}

	void *ToggleShowSuggestedConnections::creator() {
		return new ToggleShowSuggestedConnections();
	}
}
