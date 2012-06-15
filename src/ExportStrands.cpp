/*
 * ExportStrands.cpp
 *
 *  Created on: 12 jul 2011
 *      Author: johan
 */

#include <Utility.h>
#include <ExportStrands.h>
#include <HelixBase.h>
#include <DNA.h>

#include <model/Helix.h>

#include <maya/MSyntax.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MCommandResult.h>
#include <maya/MProgressWindow.h>

#include <list>
#include <functional>
#include <algorithm>
#include <cstdio>

namespace Helix {
	ExportStrands::ExportStrands() {

	}

	ExportStrands::~ExportStrands() {

	}

	MStatus ExportStrands::doIt(const MArgList & args) {
		MStatus status;
		MDagPathArray targets;
		MString excelFilePath;

		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t")) {
			unsigned int numTargets = argDatabase.numberOfFlagUses("-t");

			for(unsigned int i = 0; i < numTargets; ++i) {
				MString target;

				if (!(status = argDatabase.getFlagArgument("-t", i, target))) {
					status.perror("MArgDatabase::getFlagArgument");
					return status;
				}

				MSelectionList selectionList;

				if (!(status = selectionList.add(target))) {
					status.perror("ExportStrands::doIt: MSelectionList::add");
					return status;
				}

				MDagPath target_dagPath;
				MObject target_object;

				if (!(status = selectionList.getDagPath(0, target_dagPath, target_object))) {
					status.perror("MSelectionList::getDagPath");
					return status;
				}

				targets.append(target_dagPath);
			}
		}

		if (argDatabase.isFlagSet("-f")) {
			if (!(status = argDatabase.getFlagArgument("-f", 0, excelFilePath))) {
				status.perror("MArgDatabase::getFlagArgument 1");
				return status;
			}
		}

		if (targets.length() == 0) {
			// Find out target by select, or all targets
			//

			MObjectArray selectedBases;

			if (!(status = Model::Base::AllSelected(selectedBases))) {
				status.perror("Helix::AllSelected");
				return status;
			}

			if (selectedBases.length() == 0) {
				// Export ALL bases, this might be a bit slow..

				MItDag dagIt(MItDag::kBreadthFirst, MFn::kTransform, &status);

				if (!status) {
					status.perror("MItDag::#ctor");
					return status;
				}

				for(; !dagIt.isDone(); dagIt.next()) {
					MDagPath dagPath;

					if (!(status = dagIt.getPath(dagPath))) {
						status.perror("MItDag::getPath");
						return status;
					}

					MFnDagNode dagNode(dagPath);

					if (dagNode.typeId(&status) == HelixBase::id) {
						if (!(status = targets.append(dagPath))) {
							status.perror("MObjectArray::append");
							return status;
						}
					}
				}
			}
			else {
				for(unsigned int i = 0; i < selectedBases.length(); ++i) {
					MFnDagNode dagNode(selectedBases[i]);
					MDagPath dagPath;

					if (!(status = dagNode.getPath(dagPath))) {
						status.perror("MFnDagNode::getPath");
						return status;
					}

					targets.append(dagPath);
				}
			}
		}

		if (targets.length() == 0) {
			MGlobal::displayError("Nothing to export");
			return MStatus::kSuccess;
		}

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("Export strands");
		MProgressWindow::setProgressStatus("Isolating strands");
		MProgressWindow::setProgressRange(0, targets.length());
		MProgressWindow::startProgress();		

		std::list<Model::Strand> strands;
		std::list<Model::Strand>::iterator it;

		for(unsigned int i = 0; i < targets.length(); ++i) {
			Model::Base base(targets[i]);

			if ((it = std::find_if(strands.begin(), strands.end(), std::bind2nd(std::ptr_fun(&Model::Strand::Contains_base), base))) == strands.end()) {
				/*
				 * The strand represented by the Base is not already present, add it.
				 */

				strands.push_back(Model::Strand(base));
			}

			MProgressWindow::advanceProgress(1);
		}

		MProgressWindow::endProgress();
		
		MCommandResult commandResult;

		if (!(status = MGlobal::executeCommand("fileDialog2 -caption \"Export to text file\" -fileFilter \"Comma-separated values (*.csv);;Colon-separated values (*.csv);;Plain text (*.txt);;All files (*.*)\" -rf true -fileMode 0", commandResult))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		MStringArray result;

		if (!(status = commandResult.getResult(result))) {
			status.perror("MCommandResult::getResult");
			return status;
		}

		if (result.length() < 1) {
			/*
			 * User cancelled the export operation
			 */

			return MStatus::kSuccess;
		}

		/*
		 * Do the actual data collection on the strands
		 */

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("Export strands");
		MProgressWindow::setProgressStatus("Extracting strand sequences");
		MProgressWindow::setProgressRange(0, (int) strands.size());
		MProgressWindow::startProgress();

		std::for_each(strands.begin(), strands.end(), m_operation.execute());

		MProgressWindow::endProgress();

		/*
		 * Write to file
		 */

		if (!(status = m_operation.write(result[0], strstr(result[1].asChar(), "Comma") != NULL ? Controller::ExportStrands::COMMA_SEPARATED : Controller::ExportStrands::COLON_SEPARATED))) {
			status.perror("ExportStrands::write");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus ExportStrands::undoIt () {
		std::cerr << "I dont undo" << std::endl;

		return MStatus::kSuccess;
	}

	MStatus ExportStrands::redoIt () {
		std::cerr << "I don't redo" << std::endl;

		return MStatus::kSuccess;
	}

	bool ExportStrands::isUndoable () const {
		return false;
	}

	bool ExportStrands::hasSyntax () const {
		return true;
	}

	MSyntax ExportStrands::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);
		syntax.makeFlagMultiUse("-t");

		syntax.addFlag("-e", "-excel", MSyntax::kString);

		return syntax;
	}

	void *ExportStrands::creator() {
		return new ExportStrands();
	}

	void ExportStrands::ExportStrandsWithProgressBar::onProgressStep() {
		MProgressWindow::advanceProgress(1);
	}
}
