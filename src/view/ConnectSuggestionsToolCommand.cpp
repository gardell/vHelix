#include <view/ConnectSuggestionsToolCommand.h>
#include <view/ConnectSuggestionsLocatorNode.h>

#include <maya/MSyntax.h>
#include <maya/MArgList.h>

namespace Helix {
	namespace View {
		ConnectSuggestionsToolCommand::ConnectSuggestionsToolCommand() {
			setCommandString(MEL_CONNECT_SUGGESTIONS_TOOL_COMMAND);
		}

		ConnectSuggestionsToolCommand::~ConnectSuggestionsToolCommand() {

		}

		MStatus ConnectSuggestionsToolCommand::doIt(const MArgList& args) {
			std::cerr << "ConnectSuggestionsToolCommand::doIt" << std::endl;

			return MStatus::kSuccess;
		}

		MStatus ConnectSuggestionsToolCommand::redoIt() {
			MStatus status;

			std::cerr << "ConnectSuggestionsToolCommand::redoIt" << std::endl;

			if (!(status = m_operation.redo())) {
				status.perror("Controller::Connect::redo");
				return status;
			}

			if (!(status = m_functor.redo())) {
				status.perror("Controller::PaintMultipleStrandsWithNewColorFunctor::redo");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus ConnectSuggestionsToolCommand::undoIt() {
			MStatus status;

			std::cerr << "ConnectSuggestionsToolCommand::undoIt" << std::endl;

			if (!(status = m_operation.undo())) {
				status.perror("Controller::Connect::undo");
				return status;
			}

			if (!(status = m_functor.undo())) {
				status.perror("Controller::PaintMultipleStrandsWithNewColorFunctor::undo");
				return status;
			}

			/*
			 * The suggestion should be shown again
			 */

			//MModelMessage_ConnectSuggestionsLocatorNode_activeListModified(NULL);

			ConnectSuggestionsLocatorNode::s_closeBasesTable.push_back(ConnectSuggestionsLocatorNode::BasePair(m_operation.m_previous_connections[0][0], m_operation.m_previous_connections[1][0]));

			return MStatus::kSuccess;
		}

		bool ConnectSuggestionsToolCommand::isUndoable() const {
			return true;
		}

		MStatus ConnectSuggestionsToolCommand::finalize() {
			std::cerr << "ConnectSuggestionsToolCommand::finalize" << std::endl;

			MArgList command;
			command.addArg(commandString());

			return MPxToolCommand::doFinalize(command);
		}

		MSyntax ConnectSuggestionsToolCommand::newSyntax() {
			return MSyntax();
		}

		void *ConnectSuggestionsToolCommand::creator() {
			return new ConnectSuggestionsToolCommand();
		}

		void ConnectSuggestionsToolCommand::connect(Model::Base & source, Model::Base & destination) {
			m_operation.connect(source, destination);
			
			m_functor.loadMaterials();
			m_functor(source);

			finalize();
		}
	}
}