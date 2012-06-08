/*
 * ToggleLocatorRender.cpp
 *
 *  Created on: Aug 2, 2011
 *      Author: bjorn
 */

#include <ToggleLocatorRender.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>

namespace Helix {
#ifndef MAC_PLUGIN
	unsigned int ToggleLocatorRender::CurrentRender = kRenderAll;
#else
	unsigned int ToggleLocatorRender::CurrentRender = kRenderPairLines | kRenderSequence | kRenderDirectionalArrow;
#endif /* MAC_PLUGIN */

	ToggleLocatorRender::ToggleLocatorRender() {

	}

	ToggleLocatorRender::~ToggleLocatorRender() {

	}

	MStatus ToggleLocatorRender::updateRender(unsigned int view) {
		CurrentRender = view;

		return MGlobal::executeCommand("refresh");
	}

	MStatus ToggleLocatorRender::doIt(const MArgList & args) {
		m_lastRender = CurrentRender;

		unsigned int render = CurrentRender;
		static const char *modes[] = { RENDER_MODES, NULL };

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-e", &status)) {
			unsigned int numArguments = argDatabase.numberOfFlagUses("-e");

			for (unsigned int i = 0; i < numArguments; ++i) {
				MString arg;

				if (!(status = argDatabase.getFlagArgument("-e", i, arg))) {
					status.perror("MArgDatabase::getFlagArgument i");
					return status;
				}

				if (arg == "all") {
					render = kRenderAll;
					break;
				}

				for(size_t j = 0; modes[j] != NULL; ++j) {
					if (arg == modes[j]) {
						render |= 0x1 << j;
						break;
					}
				}
			}
		}

		if (argDatabase.isFlagSet("-d", &status)) {
			unsigned int numArguments = argDatabase.numberOfFlagUses("-d");

			for (unsigned int i = 0; i < numArguments; ++i) {
				MString arg;

				if (!(status = argDatabase.getFlagArgument("-d", i, arg))) {
					status.perror("MArgDatabase::getFlagArgument i");
					return status;
				}

				if (arg == "all") {
					render = 0;
					break;
				}

				for(size_t j = 0; modes[j] != NULL; ++j) {
					if (arg == modes[j]) {
						render &= !(0x1 << j);
						break;
					}
				}
			}
		}

		if (argDatabase.isFlagSet("-t", &status)) {
			unsigned int numArguments = argDatabase.numberOfFlagUses("-t");

			for (unsigned int i = 0; i < numArguments; ++i) {
				MString arg;

				if (!(status = argDatabase.getFlagArgument("-t", i, arg))) {
					status.perror("MArgDatabase::getFlagArgument i");
					return status;
				}

				if (arg == "all") {
					render = (render > 0 ? 0 : kRenderAll);
					break;
				}

				for(size_t j = 0; modes[j] != NULL; ++j) {
					if (arg == modes[j]) {
						render ^= 0x1 << j;

						break;
					}
				}
			}
		}

		return updateRender(render);
	}

	MStatus ToggleLocatorRender::undoIt () {
		int render = m_lastRender;
		m_lastRender = CurrentRender;

		return updateRender(render);
	}

	MStatus ToggleLocatorRender::redoIt () {
		int render = m_lastRender;
		m_lastRender = CurrentRender;

		return updateRender(render);
	}

	bool ToggleLocatorRender::isUndoable () const {
		return true;
	}

	bool ToggleLocatorRender::hasSyntax () const {
		return true;
	}

	MSyntax ToggleLocatorRender::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-e", "-enable", MSyntax::kString);
		syntax.makeFlagMultiUse("-e");

		syntax.addFlag("-d", "-disable", MSyntax::kString);
		syntax.makeFlagMultiUse("-d");

		syntax.addFlag("-t", "-toggle", MSyntax::kString);
		syntax.makeFlagMultiUse("-t");

		return syntax;
	}

	void *ToggleLocatorRender::creator() {
		return new ToggleLocatorRender();
	}
}
