/*
 * FindFivePrimeEnds.cpp
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#include <FindFivePrimeEnds.h>
#include <HelixBase.h>

#include <model/Helix.h>
#include <model/Base.h>

#include <maya/MSyntax.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

namespace Helix {
	FindFivePrimeEnds::FindFivePrimeEnds() {

	}

	FindFivePrimeEnds::~FindFivePrimeEnds() {

	}

	MStatus FindFivePrimeEnds::doIt(const MArgList & args) {
		MStatus status;
		MSelectionList activeSelectionList;

		// Traverse the scene, find bases and select the ones that does not have a backward connection (i think? :D)
		//

		MItDag itDag(MItDag::kBreadthFirst, MFn::kTransform, &status);

		if (!status) {
			status.perror("MItDag::#ctor");
			return status;
		}

		for (; !itDag.isDone(); itDag.next()) {
			Model::Base base(itDag.currentItem(&status));

			if (!status) {
				status.perror("MItDag::currentItem");
				return status;
			}

			if (base.type(status) == Model::Base::FIVE_PRIME_END)
				activeSelectionList.add(base.getObject(status));

			if (!status) {
				status.perror("Base::type or MSelectionList::add");
				return status;
			}

			/*MDagPath dagPath;

			if (!(status = itDag.getPath(dagPath))) {
				status.perror("MItDag::getPath");
				return status;
			}

			MObject object = dagPath.node(&status);
			if (!status) {
				status.perror("MDagPath::node");
				return status;
			}

			MFnDagNode dagNode(object);

			if (dagNode.typeId(&status) == HelixBase::id) {
				// Got a HelixBase, now figure out if it is connected
				//

				MPlug plug(object, HelixBase::aBackward);

				if (!plug.isConnected()) {
					activeSelectionList.add(dagPath);
				}
				else {
					// There might still be a case where it is not connected to any HelixBases but other objects
					//

					bool hasHelixBaseConnected = false;

					MPlugArray plugArray;
					if (plug.connectedTo(plugArray, true, true, &status)) {
						unsigned int plugArray_length = plugArray.length();

						for (unsigned int i = 0; i < plugArray_length; ++i) {
							MFnDagNode connected_dagNode(plugArray[i].node(&status));

							if (!status) {
								status.perror("MPlugArray[i]::node");
								return status;
							}

							if (connected_dagNode.typeId(&status) == HelixBase::id) {
								hasHelixBaseConnected = true;
								break;
							}
						}
					}

					if (!status) {
						status.perror("MPlug::connectedTo");
						return status;
					}

					if (!hasHelixBaseConnected)
						activeSelectionList.add(dagPath);
				}
			}*/
		}

		// Select all the bases in our array
		//

		m_new_selectionList = activeSelectionList;

		if (!(status = MGlobal::getActiveSelectionList(m_old_selectionList)))
			status.perror("MGlobal::getActiveSelectionList");

		if (!(status = MGlobal::setActiveSelectionList(activeSelectionList, MGlobal::kReplaceList))) {
			status.perror("MGlobal::setActiveSelectionList");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus FindFivePrimeEnds::undoIt () {
		MStatus status;

		if (!(status = MGlobal::setActiveSelectionList(m_old_selectionList))) {
			status.perror("MGlobal::setActiveSelectionList");

			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus FindFivePrimeEnds::redoIt () {
		MStatus status;

		if (!(status = MGlobal::setActiveSelectionList(m_new_selectionList))) {
			status.perror("MGlobal::setActiveSelectionList");

			return status;
		}

		return MStatus::kSuccess;
	}

	bool FindFivePrimeEnds::isUndoable () const {
		return true;
	}

	bool FindFivePrimeEnds::hasSyntax () const {
		return true;
	}

	MSyntax FindFivePrimeEnds::newSyntax () {
		MSyntax syntax;

		return syntax;
	}

	void *FindFivePrimeEnds::creator() {
		return new FindFivePrimeEnds();
	}
}
