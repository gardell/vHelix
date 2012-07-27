#include <CreateCurves.h>

#include <Utility.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>

#include <Helix.h>
#include <HelixBase.h>

namespace Helix {
	CreateCurves::CreateCurves() {

	}

	CreateCurves::~CreateCurves() {

	}

	MStatus CreateCurves::doIt(const MArgList & args) {
		MStatus status;
		std::list<MObject> targets;
		MObjectArray helices, bases;
		int degree = 3;

		MArgDatabase argDatabase(syntax(), args, &status);

		if (argDatabase.isFlagSet("-d")) {
			if (!(status = argDatabase.getFlagArgument("-d", 0, degree))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}

		if (!(status = ArgList_GetModelObjects(args, syntax(), "-t", targets))) {
			if (status != MStatus::kNotFound) {
				status.perror("ArgList_GetModelObjects");
				return status;
			}
		}

		if (targets.empty()) {
			/*
			 * Get targets by select
			 */

			MSelectionList activeSelectionList;

			if (!(status = MGlobal::getActiveSelectionList(activeSelectionList))) {
				status.perror("MGlobal::getActiveSelectionList");
			}

			if (activeSelectionList.length() > 0) {
				for(unsigned int i = 0; i < activeSelectionList.length(); ++i) {
					MObject object;
				
					if (!(status = activeSelectionList.getDependNode(i, object))) {
						status.perror("MSelectionList::getDependNode");
						return status;
					}

					MFnDagNode dagNode(object);
					MTypeId typeId = dagNode.typeId(&status);

					if (!status) {
						status.perror("MFnDagNode::typeId");
						return status;
					}

					if (typeId == HelixBase::id)
						bases.append(object);
					else if (typeId == Helix::id)
						helices.append(object);
					else {
						MGlobal::displayWarning(MString("Ignoring object \"") + dagNode.fullPathName() + "\"");
					}
				}
			}
			else {
				/*
				 * Extract all helices
				 */

				MItDag itDag(MItDag::kDepthFirst, MFn::kPluginTransformNode, &status);

				if (!status) {
					status.perror("MItDag::#ctor");
					return status;
				}

				for(; !itDag.isDone(); itDag.next()) {
					MObject object = itDag.currentItem(&status);

					if (!status) {
						status.perror("MItDag::currentItem");
						return status;
					}

					MFnDagNode dagNode(object);
					if (dagNode.typeId(&status) == Helix::id)
						helices.append(object);

					if (!status) {
						status.perror("MFnDagNode::typeId");
						return status;
					}
				}
			}
		}
		else {
			for(std::list<MObject>::iterator it = targets.begin(); it != targets.end(); ++it) {
				MFnDagNode dagNode(*it);
				MTypeId typeId = dagNode.typeId(&status);

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}

				if (typeId == HelixBase::id)
					bases.append(*it);
				else if (typeId == Helix::id)
					helices.append(*it);
				else {
					MGlobal::displayWarning(MString("Ignoring object \"") + dagNode.fullPathName() + "\"");
				}
			}
		}

		return m_operation.createCurves(helices, bases, degree);
	}

	MStatus CreateCurves::undoIt () {
		return m_operation.undo();
	}

	MStatus CreateCurves::redoIt () {
		return m_operation.redo();
	}

	bool CreateCurves::isUndoable () const {
		return true;
	}

	bool CreateCurves::hasSyntax () const {
		return true;
	}

	void *CreateCurves::creator() {
		return new CreateCurves();
	}

	MSyntax CreateCurves::newSyntax() {
		MSyntax syntax;

		syntax.addFlag("-d", "-degree", MSyntax::kLong);
		syntax.addFlag("-t", "-target", MSyntax::kString);
		syntax.makeFlagMultiUse("-t");

		return syntax;
	}
}
