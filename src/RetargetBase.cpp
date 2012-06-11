/*
 * RetargetBase.cpp
 *
 *  Created on: 27 jan 2012
 *      Author: johan
 */

#include <RetargetBase.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>

#include <Utility.h>

// FIXME: Denna metod ska inte göra några display* och inte heller nån error om baserna inte existerar
// Anropa denna från connectionMade och connectionBroken! OnIdle om det är under en deletion? Kanske inte ens behövs!! Ifall OnIdle alltid körs i rätt ordning!


namespace Helix {
	RetargetBase::RetargetBase() {

	}

	RetargetBase::~RetargetBase() {

	}

	MStatus RetargetBase::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		int perpendicular = 0;
		MString base, target;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-p", &status)) {
			if (!(status = argDatabase.getFlagArgument("-p", 0, perpendicular))) {
				status.perror("MArgDatabase::getFlagArgument perpendicular");
				return status;
			}
		}

		if (argDatabase.isFlagSet("-b", &status)) {
			if (!(status = argDatabase.getFlagArgument("-b", 0, base))) {
				status.perror("MArgDatabase::getFlagArgument base");
				return status;
			}
		}

		if (argDatabase.isFlagSet("-t", &status)) {
			if (!(status = argDatabase.getFlagArgument("-t", 0, target))) {
				status.perror("MArgDatabase::getFlagArgument target");
				return status;
			}
		}

		if (base.length() == 0) {
			MGlobal::displayError(MString("A valid base must be given"));
			return MStatus::kFailure;
		}

		// Convert the given names into MObject's

		MDagPath baseDagPath, targetDagPath;

		{
			MSelectionList selectionList;

			if ((status = selectionList.add(base)) != MStatus::kSuccess) {
				//status.perror("This error is normal, the object probably does not exist. RetargetBase::doIt: MSelectionList::add");
				return status;
			}

			if ((status = selectionList.getDagPath(0, baseDagPath)) != MStatus::kSuccess) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}
		}

		if (target.length() > 0) {
			MSelectionList selectionList;

			if ((status = selectionList.add(target)) != MStatus::kSuccess) {
				//status.perror("This error is normal, the object probably does not exist. RetargetBase::doIt 2: MSelectionList::add");
				return status;
			}

			if ((status = selectionList.getDagPath(0, targetDagPath)) != MStatus::kSuccess) {
				status.perror("RetargetBase::doIt 3: MSelectionList::add");
				return status;
			}
		}

		// Ok, remove all the aimconstraints on the `base' object

		MObject baseObject = baseDagPath.node(&status);

		if (!status) {
			status.perror("MDagPath::node");
			return status;
		}

		if ((status = HelixBase_RemoveAllAimConstraints(baseObject)) != MStatus::kSuccess) {
			status.perror("HelixBase_RemoveAllAimConstraints");
			return status;
		}

		// Now create the new aimconstraint

		if (target.length() > 0) {
			if (!(status = MGlobal::executeCommand(MString("aimConstraint -aimVector ") + (perpendicular > 0 ? "1.0 0 0 ": "0 0 -1.0 ") + targetDagPath.fullPathName() + " " + baseDagPath.fullPathName() + ";", false))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}
		}

		return MStatus::kSuccess;
	}

	MStatus RetargetBase::undoIt () {
		return MStatus::kSuccess;
	}

	MStatus RetargetBase::redoIt () {
		return MStatus::kSuccess;
	}

	bool RetargetBase::isUndoable () const {
		return false;
	}

	bool RetargetBase::hasSyntax () const {
		return true;
	}

	MSyntax RetargetBase::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-p", "-perpendicular", MSyntax::kLong);
		syntax.addFlag("-b", "-base", MSyntax::kString);
		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *RetargetBase::creator() {
		return new RetargetBase();
	}
}
