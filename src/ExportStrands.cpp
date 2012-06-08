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

#include <list>
#include <cstdio>

namespace Helix {
	ExportStrands::ExportStrands() {

	}

	ExportStrands::~ExportStrands() {

	}

	// Helper class, defines a strand

	class Strand {
	public:
		Strand() {

		}

		// This is the faster way to determine, because a base can only belong to one strand anyway

		bool contains(const MObject & base, MStatus *retStatus = NULL) const {
			for(unsigned int i = 0; i < m_bases.length(); ++i) {
				if (m_bases[i].node(retStatus) == base)
					return true;
			}

			return false;
		}

		// Don't use if you don't really need it, it is SLOW
		//

		bool operator==(const Strand & strand) const {
			// We can't use the MDagPathArray's == operator, because the ordering might be different

			if (m_bases.length() != strand.m_bases.length())
				return false;



			for(unsigned int i = 0; i < m_bases.length(); ++i) {
				bool contains = false;

				for(unsigned int j = 0; j < m_bases.length(); ++j) {
					if (m_bases[i].node() == strand.m_bases[j].node()) {
						contains = true;
						break;
					}
				}

				if (!contains)
					return false;
			}

			return true;
		}

		// FIXME: Add some naming ?
		MDagPathArray m_bases;
	};

	class StrandArray : public std::list<Strand> {
	public:
		bool contains(MObject & base, MStatus *retStatus = NULL) const {
			for(std::list<Strand>::const_iterator it = begin(); it != end(); ++it) {
				if (it->contains(base, retStatus))
					return true;

				if (!*retStatus)
					break;
			}

			return false;
		}
	};

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

			if (!(status = SelectedBases(selectedBases))) {
				status.perror("SelectedBases");
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
			return MStatus::kFailure;
		}

		// Ok, we have a bunch of nodes, but many of them probably belong to the same strand, so when iterating, we need to make sure that the strand the base belongs to has not already been exported
		// this makes the method very slow when there's many nodes.
		// The user can optimize this by selecting only one base of every strand he/she wants to export, this is not very user friendly though
		//

		StrandArray strands;

		for(unsigned int i = 0; i < targets.length(); ++i) {
			MObject object = targets[i].node(&status);
			if (!status) {
				status.perror("MDagPathArray[i]::node");
				return status;
			}

			bool contains = strands.contains(object, &status);

			if (!status) {
				status.perror("StrandArray::contains");
				return status;
			}

			if (!contains) {
				// This base belongs to a scaffold we have not come across before, find all the bases belonging to it
				//

				Strand strand;

				MDagPath it_base_dagPath = targets[i];
				MObject it_base_object, target_object = targets[i].node(&status);

				if (!status) {
					status.perror("MDagPathArray[i]::node");
					return status;
				}

				// Forward
				bool first = true;

				do {
					it_base_object = it_base_dagPath.node(&status);

					if (!status) {
						status.perror("MDagPath::node");
						return status;
					}

					if (first)
						first = false;
					else if (it_base_object == target_object)
						break;

					if (!(status = strand.m_bases.append(it_base_dagPath))) {
						status.perror("MDagPathArray::append");
						return status;
					}
				}
				while (HelixBase_NextBase(it_base_object, HelixBase::aForward, it_base_dagPath, &status));

				if (!status) {
					status.perror("HelixBase_NextBase 1");
					return status;
				}

				// Backward

				it_base_dagPath = targets[i];
				it_base_object = it_base_dagPath.node(&status);

				if (!status) {
					status.perror("MDagPath::node");
					return status;
				}

				first = true;

				while (HelixBase_NextBase(it_base_object, HelixBase::aBackward, it_base_dagPath, &status)) {
					it_base_object = it_base_dagPath.node(&status);

					if (!status) {
						status.perror("MDagPath::node");
						return status;
					}

					if (first)
						first = false;
					else if (it_base_object == target_object)
						break;

					if (!(status = strand.m_bases.append(it_base_dagPath))) {
						status.perror("MDagPathArray::append");
						return status;
					}
				}

				if (!status) {
					status.perror("HelixBase_NextBase 2");
					return status;
				}

				strands.push_back(strand);
			}
		}

		// Now we have a list of strands containing all the bases
		// Show a dialog containing the strand sequences along with options for exporting to excel etc
		//

		/*MString outputString;

		for(StrandArray::iterator it = strands.begin(); it != strands.end(); ++it) {
			for(unsigned int i = 0; i < it->m_bases.length(); ++i) {
				MPlug labelPlug(it->m_bases[i].node(&status), HelixBase::aLabel);

				if (!status) {
					status.perror("MDagPath::node");
					return status;
				}

				DNA::Names baseLabel;

				if (!(status = labelPlug.getValue((int &) baseLabel))) {
					status.perror("MPlug::getValue");
					return status;
				}

				bool isDestination = labelPlug.isDestination(&status);

				if (!status) {
					status.perror("MPlug::isDestination");
					return status;
				}

				if (isDestination)
					baseLabel = DNA::OppositeBase(baseLabel);

				char baseLabelChar = DNA::ToChar(baseLabel);
				outputString += MString(&baseLabelChar, 1);
			}

			outputString += "\\n\\n";
		}

		MCommandResult commandResult;

		if (!(status = MGlobal::executeCommand(
				MString("promptDialog -title \"Export strands\" -message \"Available strands for all or the currently selected bases strands:\" -scrollableField true -button \"Export\" -button \"Close\" -defaultButton \"Close\" -cancelButton \"Close\" -dismissString \"Close\" -text \"") + outputString + "\";\n", commandResult))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		MString promptDialogResult;

		if (!(status = commandResult.getResult(promptDialogResult))) {
			status.perror("MCommandResult");
			return status;
		}*/

		MCommandResult commandResult;

		//if (promptDialogResult == "Export") {
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
				//MGlobal::displayError("No valid number of files to export to");
				return MStatus::kSuccess;
			}

			FILE *file;

			if (!(file = fopen(result[0].asChar(), "w"))) {
				MGlobal::displayError("Failed to open file \"" + result[0] + "\" for writing.");
				return MStatus::kFailure;
			}

			char column_delimiter = '\t';

			std::cerr << result[1].asChar() << std::endl;

			if (strstr(result[1].asChar(), "Comma"))
				column_delimiter = ',';
			else if (strstr(result[1].asChar(), "Colon"))
				column_delimiter = ';';

			MString column_delimiter_string(&column_delimiter, 1);

			for(StrandArray::iterator it = strands.begin(); it != strands.end(); ++it) {
				MString names, sequence;
				MString color;

				for(unsigned int i = 0; i < it->m_bases.length(); ++i) {
					MPlug labelPlug(it->m_bases[i].node(&status), HelixBase::aLabel);

					if (!status) {
						status.perror("MDagPath::node");
						return status;
					}

					DNA::Names baseLabel;

					if (!(status = labelPlug.getValue((int &) baseLabel))) {
						status.perror("MPlug::getValue");
						return status;
					}

					bool isDestination = labelPlug.isDestination(&status);

					if (!status) {
						status.perror("MPlug::isDestination");
						return status;
					}

					if (isDestination)
						baseLabel = DNA::OppositeBase(baseLabel);

					char baseLabelChar = DNA::ToChar(baseLabel);
					sequence += MString(&baseLabelChar, 1);

					/*names += it->m_bases[i].fullPathName(&status) + "\t";

					if (!status) {
						status.perror("MDagPathArray[i]::fullPathName");
						return status;
					}*/
				}

				if (it->m_bases.length() >= 2)
					names = it->m_bases[0].fullPathName() + column_delimiter_string + it->m_bases[1].fullPathName();
				else if (it->m_bases.length() == 1)
					names = it->m_bases[0].fullPathName();
				else
					continue; // Nothing to print

				if (!(status = DNA::QueryMaterialOfObject(it->m_bases[it->m_bases.length() - 1].node(), color))) {
					status.perror("DNA::QueryMaterialOfObject");
					return status;
				}

				/*MString colorName;
				MFnDagNode color_dagPath(color);

				colorName = color_dagPath.name(&status);*/

				if (!status) {
					status.perror("MFnDagNode::name");
					return status;
				}

				fwrite(names.asChar(), 1, names.length(), file);
				fputc(column_delimiter, file);
				fwrite(sequence.asChar(), 1, sequence.length(), file);
				fputc(column_delimiter, file);
				fwrite(color.asChar(), 1, color.length(), file);
				fputs("\r\n", file);
			}

			fclose(file);
		//}

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
}
