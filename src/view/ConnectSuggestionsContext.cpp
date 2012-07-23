#include <view/ConnectSuggestionsContext.h>
#include <view/ConnectSuggestionsLocatorNode.h>
#include <view/ConnectSuggestionsToolCommand.h>

#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MDGModifier.h>

namespace Helix {
	namespace View {
		ConnectSuggestionsContext::ConnectSuggestionsContext() : m_locatorNode(MObject::kNullObj) {
			setTitleString("Visual helix base connection tool");
		}

		ConnectSuggestionsContext::~ConnectSuggestionsContext() {

		}

		void ConnectSuggestionsContext::toolOnSetup(MEvent &event) {
			setHelpString("A visual tool for easy detection of potential helix base connections");

			/*
			 * Create our ConnectSuggestionsLocator IF there's not already one existing
			 */

			MStatus status;

			MItDag it(MItDag::kDepthFirst, MFn::kPluginLocatorNode, &status);

			bool found = false;
			for(; !it.isDone(); it.next()) {
				MFnDagNode dagNode(it.currentItem(&status));

				if (dagNode.typeId(&status) == ConnectSuggestionsLocatorNode::id) {
					found = true;
					break;
				}
			}

			if (!found) {
				/*
				 * Create a LocatorNode for the rendering
				 */

				std::cerr << "Creating a ConnectSuggestionsLocatorNode to track potential connections" << std::endl;

				MFnDagNode locator_dagNode;
				m_locatorNode = locator_dagNode.create(ConnectSuggestionsLocatorNode::id, MObject::kNullObj, &status);

				if (!status)
					status.perror("MFnDagNode::create");
			}
		}

		void ConnectSuggestionsContext::toolOffCleanup() {
			if (!m_locatorNode.isNull()) {
				MStatus status;
				MDGModifier dgModifier;

				if (!(status = dgModifier.deleteNode(m_locatorNode)))
					status.perror("MDGModifier::deleteNode");
				else if (!(status = dgModifier.doIt()))
					status.perror("MDGModifier::doIt");

				m_locatorNode = MObject::kNullObj;
			}
		}

		MStatus ConnectSuggestionsContext::doPress(MEvent &event) {
			MStatus status;
			short x, y;

			if (!(status = event.getPosition(x, y))) {
				status.perror("MEvent::getPosition");
				return status;
			}

			M3dView view = M3dView::active3dView(&status);

			if (!status) {
				status.perror("M3dView::active3dView");
				return status;
			}

			ConnectSuggestionsLocatorNode::onPress(x, y, view, *this);

			return MStatus::kSuccess;
		}

		MStatus ConnectSuggestionsContext::doDrag(MEvent &event) {
			MStatus status;
			short x, y;

			if (!(status = event.getPosition(x, y))) {
				status.perror("MEvent::getPosition");
				return status;
			}

			M3dView view = M3dView::active3dView(&status);

			if (!status) {
				status.perror("M3dView::active3dView");
				return status;
			}

			ConnectSuggestionsLocatorNode::onDrag(x, y, view, *this);

			return MStatus::kSuccess;
		}

		MStatus ConnectSuggestionsContext::doRelease(MEvent &event) {
			MStatus status;
			short x, y;

			if (!(status = event.getPosition(x, y))) {
				status.perror("MEvent::getPosition");
				return status;
			}

			M3dView view = M3dView::active3dView(&status);

			if (!status) {
				status.perror("M3dView::active3dView");
				return status;
			}

			ConnectSuggestionsLocatorNode::onRelease(x, y, view, *this);

			return MStatus::kSuccess;
		}

		MStatus ConnectSuggestionsContext::doEnterRegion(MEvent &event) {
			return MStatus::kSuccess;
		}

		void ConnectSuggestionsContext::getClassName(MString & name) const {
			name = "connectSuggestionsContext";
		}

		void ConnectSuggestionsContext::completeAction() {
			std::cerr << __FUNCTION__ << std::endl;
		}

		void ConnectSuggestionsContext::abortAction() {
			std::cerr << __FUNCTION__ << std::endl;
		}

		void ConnectSuggestionsContext::connect(Model::Base & source, Model::Base & destination) {
			ConnectSuggestionsToolCommand & toolCommand = *static_cast<ConnectSuggestionsToolCommand *> (newToolCommand());

			toolCommand.connect(source, destination);
		}
	}
}