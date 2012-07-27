/*
 * HelixView.cpp
 *
 *  Created on: 14 jul 2011
 *      Author: johan
 */

#include <ToggleCylinderBaseView.h>
#include <Helix.h>
#include <Utility.h>
#include <model/Helix.h>
#include <model/Base.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>

#include <algorithm>
#include <iterator>

#define VIEW_BASES "base"
#define VIEW_CYLINDERS "cylinder"
#define TOGGLE_VIEWS	VIEW_BASES, VIEW_CYLINDERS
//#define TOGGLE_VIEW_FLAGS { { true, false }, { false, true } }


namespace Helix {
	int ToggleCylinderBaseView::CurrentView = 0;

	ToggleCylinderBaseView::ToggleCylinderBaseView() {

	}

	ToggleCylinderBaseView::~ToggleCylinderBaseView() {

	}

	MStatus ToggleCylinderBaseView::toggle(bool _toggle, bool refresh, std::list<MObject> & targets) {
		MStatus status;

		if (targets.empty()) {
			if (_toggle) {
				ToggleCylinderBaseView::CurrentView = (ToggleCylinderBaseView::CurrentView + 1) % 2;
				m_toggled = true;
			}

			/*
			 * Iterate over all helices and set HelixShape and HelixBaseShape visibility dependent on CurrentView
			 */

			if (_toggle || refresh) {
				// Switch to the new view or refresh the current, using MEL

				/*if (!(status = MGlobal::executeCommand(
						MString(CurrentView == 0 ? "showHidden" : "hide") + (" `ls -long -type HelixBase`;\n"
						"$helices = `ls -long -type vHelix`;\n"
						"for ($helix in $helices) {\n"
						"    ") + (CurrentView == 0 ? "hide" : "showHidden") + (" `ls -long ($helix + \"|cylinderRepresentation\")`;\n"
						"}\n")))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}*/

				MItDag itDag(MItDag::kDepthFirst, MFn::kPluginTransformNode, &status);

				if (!status) {
					status.perror("MItDag::#ctor");
					return status;
				}

				for(; !itDag.isDone(); itDag.next()) {
					Model::Helix helix(itDag.currentItem(&status));

					if (!status) {
						status.perror("MItDag::currentItem");
						return status;
					}

					if (!(status = helix.setShapesVisibility(CurrentView == 1))) {
						status.perror("Helix::setShapesVisibility");
						return status;
					}

					for(Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it) {
						if (!(status = it->setShapesVisibility(CurrentView == 0))) {
							status.perror("Base::setShapesVisibility");
							return status;
						}
					}
				}
			}
		}
		else {
			/*
			 * Iterate over targets and *toggle* their HelixShape and HelixBaseShape visibility
			 */

			for(std::list<MObject>::iterator it = targets.begin(); it != targets.end(); ++it) {
				Model::Helix helix(*it);

				if (!(status = helix.toggleShapesVisibility())) {
					status.perror("Helix::toggleShapesVisibility");
					return status;
				}

				for(Model::Helix::BaseIterator bit = helix.begin(); bit != helix.end(); ++bit) {
					if (!(status = bit->toggleShapesVisibility())) {
						status.perror("Base::toggleShapesVisibility");
						return status;
					}
				}
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

		/*
		 * Look for targets either as `-base` or selected nodes
		 */

		m_toggleTargets.clear();

		if (!(status = ArgList_GetModelObjects(args, syntax(), "-b", m_toggleTargets))) {
			if (status != MStatus::kNotFound) {
				status.perror("ArgList_GetModelObjects");
				return status;
			}
		}

		if (m_toggleTargets.empty()) {
			/*
			 * Get targets by selected
			 */

			MObjectArray targets;

			if (!(status = Model::Helix::AllSelected(targets))) {
				status.perror("Helix::AllSelected");
				return status;
			}

			std::copy(&targets[0], &targets[0] + targets.length(), std::back_insert_iterator<std::list<MObject> >(m_toggleTargets));
		}

		/*
		 * If there's no target, toggle all
		 */
		/*if (m_toggleTargets.length() == 0) {
			if (!(status = toggle(_toggle, refresh))) {
				status.perror("Toggle");
				return status;
			}

			setResult(toggleViewNames[ToggleCylinderBaseView::CurrentView]);
		}
		else {
			for(unsigned int i = 0; i < m_toggleTargets.length(); ++i) {
				Model::Helix helix(m_toggleTargets[i]);
				
				std::cerr << "Toggle on helix: " << helix << std::endl;

				helix.toggleShapesVisibility();

				std::for_each(helix.begin(), helix.end(), std::mem_fun_ref(&Model::Base::toggleShapesVisibility));
			}
		}*/

		if (!(status = toggle(_toggle, refresh, m_toggleTargets))) {
			status.perror("Toggle");
			return status;
		}
		
		setResult(toggleViewNames[ToggleCylinderBaseView::CurrentView]);

		return MStatus::kSuccess;
	}

	MStatus ToggleCylinderBaseView::undoIt () {
		return toggle(m_toggled, true, m_toggleTargets);
	}

	MStatus ToggleCylinderBaseView::redoIt () {
		return toggle(m_toggled, true, m_toggleTargets);
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
		
		syntax.addFlag("-b", "-base", MSyntax::kString);
		syntax.makeFlagMultiUse("-b");

		return syntax;
	}

	void *ToggleCylinderBaseView::creator() {
		return new ToggleCylinderBaseView();
	}
}
