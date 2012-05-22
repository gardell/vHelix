/*
 * Disconnect.cpp
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#include <HelixBase.h>

#include <Disconnect.h>
#include <Utility.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MDGModifier.h>

namespace Helix {
	Disconnect::Disconnect() {

	}

	Disconnect::~Disconnect() {

	}

	MStatus Disconnect::disconnect(const MObjectArray & targets) {
		MStatus status;

		m_disconnected.clear();
		m_disconnect_targets.clear();
		m_disconnect_targets.setSizeIncrement(targets.length());

		for(unsigned int i = 0; i < targets.length(); ++i)
			m_disconnect_targets.append(targets[i]);

		// Ok, we have what we need, disconnect the targets forward attribute connections
		//

		unsigned int targets_length = targets.length();

		MString info("Disconnecting bases");

		for (unsigned int i = 0; i < targets_length; ++i) {
			MFnDagNode dagNode(targets[i]);

			info = info + " \"" + dagNode.fullPathName() + "\"";
		}

		MGlobal::displayInfo(info);

		for (unsigned int i = 0; i < targets_length; ++i) {
			MObject old_target = MObject::kNullObj;

			if (!(status = DisconnectAllHelixBaseConnections(MPlug(targets[i], HelixBase::aForward), false, &old_target))) {
				status.perror("DisconnectAllHelixBaseConnections");
				return status;
			}

			m_disconnected.push_back(std::make_pair(old_target, targets[i]));
		}

		return MStatus::kSuccess;
	}

	MStatus Disconnect::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		MObjectArray targets;

		if (argDatabase.isFlagSet("-t", &status)) {
			MString target_str;

			if (!(status = argDatabase.getFlagArgument("-t", 0, target_str))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;
			if (!(status = selectionList.add(target_str))) {
				status.perror("MSelectionList::add");
				return status;
			}

			MObject target;

			if (!(status = selectionList.getDependNode(0, target))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			if (!(status = targets.append(target))) {
				status.perror("MObjectArray::append");
				return status;
			}
		}

		if (!status) {
			status.perror("MArgDatabase::isFlagSet");
			return status;
		}

		if (targets.length() == 0) {
			// Find out our targets by select
			//

			std::cerr << "Finding targets by select" << std::endl;

			if (!(status = SelectedBases(targets))) {
				status.perror("SelectedBases");
				return status;
			}
		}

		return disconnect(targets);
	}

	MStatus Disconnect::undoIt () {
		MStatus status;
		MDGModifier dgModifier;

		for(std::vector<std::pair<MObject, MObject> >::iterator it = m_disconnected.begin(); it != m_disconnected.end(); ++it) {
			if (!(status = dgModifier.connect(it->first, HelixBase::aBackward, it->second, HelixBase::aForward)))
				status.perror("MDGModifier::connect");
		}

		if (!(status = dgModifier.doIt())) {
			status.perror("MDGModifier::doIt");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Disconnect::redoIt () {
		// Ok, this is a bit ugly, but since we're passing as a reference, we must copy it

		MObjectArray new_targets;
		new_targets.setSizeIncrement(m_disconnect_targets.length());

		for(unsigned int i = 0; i < m_disconnect_targets.length(); ++i)
			new_targets.append(m_disconnect_targets[i]);
		return disconnect(new_targets);
	}

	bool Disconnect::isUndoable () const {
		return true;
	}

	bool Disconnect::hasSyntax () const {
		return true;
	}

	MSyntax Disconnect::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *Disconnect::creator() {
		return new Disconnect();
	}
}
