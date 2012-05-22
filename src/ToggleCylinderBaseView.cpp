/*
 * HelixView.cpp
 *
 *  Created on: 14 jul 2011
 *      Author: johan
 */

#include <ToggleCylinderBaseView.h>
#include <Helix.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>

#define VIEW_BASES "base"
#define VIEW_CYLINDERS "cylinder"
#define TOGGLE_VIEWS	VIEW_BASES, VIEW_CYLINDERS
#define TOGGLE_VIEW_FLAGS { { true, false }, { false, true } }


namespace Helix {
	int ToggleCylinderBaseView::CurrentView = 0;

	ToggleCylinderBaseView::ToggleCylinderBaseView() {

	}

	ToggleCylinderBaseView::~ToggleCylinderBaseView() {

	}

	MStatus ToggleCylinderBaseView::toggle(bool _toggle, bool refresh) {
		MStatus status;

		if (_toggle) {
			ToggleCylinderBaseView::CurrentView = (ToggleCylinderBaseView::CurrentView + 1) % 2;
			m_toggled = true;
		}

		if (_toggle || refresh) {
			// Switch to the new view or refresh the current, using MEL

			if (!(status = MGlobal::executeCommand(
					MString(CurrentView == 0 ? "showHidden" : "hide") + (" `ls -long -type HelixBase`;\n"
					"$helices = `ls -long -type vHelix`;\n"
					"for ($helix in $helices) {\n"
					"    ") + (CurrentView == 0 ? "hide" : "showHidden") + (" `ls -long ($helix + \"|cylinderRepresentation\")`;\n"
					"}\n")))) {
				status.perror("MGlobal::executeCommandOnIdle");
				return status;
			}
		}

		return MStatus::kSuccess;
	}

	MStatus ToggleCylinderBaseView::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		//MString setView;
		bool _toggle = false, refresh = false;
		static const char *toggleViewNames[] = { TOGGLE_VIEWS, NULL };

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t", &status)) {
			if (!(status = argDatabase.getFlagArgument("-t", 0, _toggle))) {
				status.perror("MArgDatabase::getFlagArgument 1");
				return status;
			}
		}
		else if (argDatabase.isFlagSet("-r", &status)) {
			if (!(status = argDatabase.getFlagArgument("-r", 0, refresh))) {
				status.perror("MArgDatabase::getFlagArgument 2");
				return status;
			}
		}

		if (!(status = toggle(_toggle, refresh))) {
			status.perror("Toggle");
			return status;
		}

		setResult(toggleViewNames[ToggleCylinderBaseView::CurrentView]);

		return MStatus::kSuccess;
	}

	MStatus ToggleCylinderBaseView::undoIt () {
		return toggle(m_toggled, true);
	}

	MStatus ToggleCylinderBaseView::redoIt () {
		return toggle(m_toggled, true);
	}

	bool ToggleCylinderBaseView::isUndoable () const {
		return true;
	}

	bool ToggleCylinderBaseView::hasSyntax () const {
		return true;
	}

	MSyntax ToggleCylinderBaseView::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-toggle", MSyntax::kBoolean);
		syntax.addFlag("-r", "-refresh", MSyntax::kBoolean);

		return syntax;
	}

	void *ToggleCylinderBaseView::creator() {
		return new ToggleCylinderBaseView();
	}
}
