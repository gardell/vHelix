#include <TargetHelixBaseBackward.h>
#include <Utility.h>

#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MFnDagNode.h>

#include <list>

namespace Helix {
	TargetHelixBaseBackward::TargetHelixBaseBackward() {

	}

	TargetHelixBaseBackward::~TargetHelixBaseBackward() {

	}

	MStatus TargetHelixBaseBackward::doIt(const MArgList & args) {
		MStatus status;
		std::list<MObject> targets;

		if (!(status = ArgList_GetModelObjects(args, syntax(), "-b", targets))) {
			status.perror("ArgList_GetModelObjects");
			return status;
		}

		for(std::list<MObject>::iterator it = targets.begin(); it != targets.end(); ++it) {
			MString this_fullPathName = MFnDagNode(*it).fullPathName();

			std::cerr << "TargetHelixBaseBackward on " << this_fullPathName.asChar() << std::endl;

			if (!(status = MGlobal::executeCommand(MString("delete -cn ") + this_fullPathName + "; setAttr " + this_fullPathName + ".rotate 0 0 0;\n{\n$backwards = `listConnections " + this_fullPathName + ".backward`;\nfor($backward in $backwards) aimConstraint -aimVector 1.0 0 0 $backward " + this_fullPathName + ";\n}", false)))
				status.perror("MGlobal::executeCommand");
		}

		return MStatus::kSuccess;
	}

	MStatus TargetHelixBaseBackward::undoIt () {
		return MStatus::kSuccess;
	}

	MStatus TargetHelixBaseBackward::redoIt () {
		return MStatus::kSuccess;
	}

	bool TargetHelixBaseBackward::isUndoable () const {
		return true; /* Important */
	}

	bool TargetHelixBaseBackward::hasSyntax () const {
		return false;
	}

	MSyntax TargetHelixBaseBackward::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-b", "-base", MSyntax::kString);

		return syntax;
	}

	void *TargetHelixBaseBackward::creator() {
		return new TargetHelixBaseBackward();
	}
}