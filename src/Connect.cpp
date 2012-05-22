/*
 * Connect.cpp
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#include <Connect.h>
#include <HelixBase.h>
#include <Utility.h>

#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>
#include <maya/MArgDatabase.h>
#include <maya/MDGModifier.h>
#include <maya/MPlugArray.h>


namespace Helix {
	Connect::Connect() {

	}

	Connect::~Connect() {

	}

	MStatus Connect::connect(const MObject & target, const MObject & source) {
		MStatus status;

		m_connected_target = target;
		m_connected_source = source;
		m_old_source = MObject::kNullObj;
		m_old_target = MObject::kNullObj;

		// Ok, we have the required arguments
		//

		MGlobal::displayInfo(MString("Connecting base \"") + MFnDagNode(source).fullPathName() + "\" to \"" + MFnDagNode(target).fullPathName());

		MPlug forwardPlug (source, HelixBase::aForward),
			  backwardPlug (target, HelixBase::aBackward);

		// Disconnect all old connections

		if (!(status = DisconnectAllHelixBaseConnections(forwardPlug, false, &m_old_target))) {
			status.perror("DisconnectAllHelixBaseConnections");
			return status;
		}

		if (!(status = DisconnectAllHelixBaseConnections(backwardPlug, true, &m_old_source))) {
			status.perror("DisconnectAllHelixBaseConnections");
			return status;
		}

		// Make the new connection

		MDGModifier dgModifier;

		//std::cerr << "connectAttr " << MFnDagNode(target).fullPathName().asChar() << " " << MFnDagNode(source).fullPathName().asChar() << std::endl;

		if (!(status = dgModifier.connect(backwardPlug, forwardPlug))) {
			status.perror("MDGModifier::connect");
			return status;
		}

		if (!(status = dgModifier.doIt())) {
			status.perror("MDGModifier::doIt @");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Connect::doIt(const MArgList & args) {
		MObject targets[] = { MObject::kNullObj, MObject::kNullObj };
		static const char *flags[] = { "-f", "-s" };

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		for(size_t i = 0; i < 2; ++i) {
			if (argDatabase.isFlagSet(flags[i], &status)) {
				MString str_target;

				if (!(status = argDatabase.getFlagArgument(flags[i], 0, str_target))) {
					status.perror("MArgDatabase::getFlagArgument");
					return status;
				}

				MSelectionList selectionList;
				if (!(status = selectionList.add(str_target))) {
					status.perror("MSelectionList:add");
					return status;
				}

				if (!(status = selectionList.getDependNode(0, targets[i]))) {
					status.perror("MSelectionList::getDependNode");
					return status;
				}
			}
		}

		// If arguments are missing, compensate by using selected bases
		//

		size_t missingObjects_count = 0;

		for(size_t i = 0; i < 2; ++i) {
			if (targets[i] == MObject::kNullObj)
				missingObjects_count++;
		}

		if (missingObjects_count > 0) {
			MObjectArray selectedObjects;

			if (!(status = SelectedBases(selectedObjects))) {
				status.perror("SelectedBases");
				return status;
			}

			if (missingObjects_count > selectedObjects.length()) {
				MGlobal::displayError("There are not enough bases given, either by argument or by select. Two bases are required");
				return MStatus::kFailure;
			}

			unsigned int selectedObjects_index = 0;

			for(unsigned int i = 0; i < 2; ++i) {
				if (targets[i] == MObject::kNullObj) {
					targets[i] = selectedObjects[selectedObjects_index++];
				}
			}
		}

		return connect(targets[1], targets[0]);
	}

	MStatus Connect::undoIt () {
		MStatus status;

		// In the, unlikely event, that the connected pair is already connected. An error would occur if we didn't take care of this here
		//

		if (m_connected_source == m_old_source && m_connected_target == m_old_target) {
			std::cerr << "No need to undo, we're just reverting back to the same state" << std::endl;
			return MStatus::kSuccess;
		}

		{
			MDGModifier dgModifier;

			// Disconnect the newly created connections

			std::cerr << "Disconnecting: " << MFnDagNode(m_connected_target).fullPathName().asChar() << ".backward " << MFnDagNode(m_connected_source).fullPathName().asChar() << ".forward" << std::endl;

			if (!(status = dgModifier.disconnect(m_connected_target, HelixBase::aBackward, m_connected_source, HelixBase::aForward))) {
				status.perror("MDGModifier::disconnect");
				return status;
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt 1");
				return status;
			}

			// Because connectionBroken can't clean up the aimConstraints anymore, we have to do it here (there's a bug in Maya that makes in crash)
		}

		{
			MDGModifier dgModifier;

			if (m_old_target != MObject::kNullObj) {
				if (!(status = dgModifier.connect(m_old_target, HelixBase::aBackward, m_connected_source, HelixBase::aForward))) {
					status.perror("MDGModifier::connect");
					return status;
				}
			}

			if (m_old_source != MObject::kNullObj) {
				if (!(status = dgModifier.connect(m_connected_target, HelixBase::aBackward, m_old_source, HelixBase::aForward))) {
					status.perror("MDGModifier::connect");
					return status;
				}
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}
		}

		return MStatus::kSuccess;
	}

	MStatus Connect::redoIt () {
		// FIXME:

		return MStatus::kSuccess;
	}

	bool Connect::isUndoable () const {
		return true;
	}

	bool Connect::hasSyntax () const {
		return true;
	}

	MSyntax Connect::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-f", "-first", MSyntax::kString);
		syntax.addFlag("-s", "-second", MSyntax::kString);

		return syntax;
	}

	void *Connect::creator() {
		return new Connect();
	}
}
