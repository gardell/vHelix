/*
 * PaintStrand.cpp
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#include <DNA.h>
#include <PaintStrand.h>
#include <Utility.h>
#include <HelixBase.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPathArray.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>

namespace Helix {
	PaintStrand::PaintStrand() {

	}

	PaintStrand::~PaintStrand() {

	}

	MStatus PaintStrand::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		MDagPathArray targets;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t", &status)) {
			MString targetName;

			if (!(status = argDatabase.getFlagArgument("-t", 0, targetName))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;

			if (!(status = selectionList.add(targetName))) {
				status.perror("MSelectionList::add");
				return status;
			}

			MDagPath targetDagPath;
			MObject targetObject;

			if (!(status = selectionList.getDagPath(0, targetDagPath, targetObject))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}

			targets.append(targetDagPath);
		}

		if (targets.length() == 0) {
			// Find the targets by using the list of currently selected bases
			//

			MObjectArray selectedBases;
			if (!(status = SelectedBases(selectedBases))) {
				status.perror("SelectedHelices");
				return status;
			}

			// Find the MDagPaths of the selected helices

			for(unsigned int i = 0; i < selectedBases.length(); ++i) {
				MDagPath dagPath;
				MFnDagNode dagNode(selectedBases[i]);

				if (!(status = dagNode.getPath(dagPath))) {
					status.perror("MFnDagNode::getPath");
					return status;
				}

				if (!(status = targets.append(dagPath))) {
					status.perror("MDagPathArray::append");
					return status;
				}
			}
		}

		if (targets.length() == 0) {
			MGlobal::displayError("Nothing to paint");
			return MStatus::kFailure;
		}

		return paintStrands(targets);
	}

	MStatus PaintStrand::undoIt () {
		MStatus status;

		unsigned int lastPaintedIndex = (m_currentPaintedColors + 1) % 2;

		// DEBUG

		for(unsigned int i = 0; i < m_paintedColors[lastPaintedIndex].length(); ++i) {
			std::cerr << "index: " << i << " was assigned with color: " << m_paintedColors[lastPaintedIndex][i].asChar() << std::endl;
		}

		// END DEBUG

		if (m_paintedColors[lastPaintedIndex].length() == 1) {
			if (!(status = DNA::AssignMaterialToObjects(m_previousPaintedDagNodes, m_paintedColors[lastPaintedIndex][0]))) {
				status.perror("DNA::AssignMaterialToObjects");
				return status;
			}
		}
		else {
			if (!(status = DNA::AssignMaterialsToObjects(m_previousPaintedDagNodes, m_paintedColors[lastPaintedIndex]))) {
				status.perror("paintBasesWithColors");
				return status;
			}
		}

		// Switch what colors we will use to redo/undo

		m_currentPaintedColors = lastPaintedIndex;

		return MStatus::kSuccess;
	}

	MStatus PaintStrand::redoIt () {
		return undoIt();
	}

	bool PaintStrand::isUndoable () const {
		return true;
	}

	bool PaintStrand::hasSyntax () const {
		return true;
	}

	MSyntax PaintStrand::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *PaintStrand::creator() {
		return new PaintStrand();
	}

	MStatus PaintStrand::paintStrands(MDagPathArray & dagPathArray) {
		MStatus status;

		// Ok, we have a list of bases, now iterate over them and find all the relative bases too
		// This can get quite slow
		//

		MDagPathArray neighbourBase_dagPaths;
		MObjectArray neighbourBase_objects;

		unsigned int targets_length = dagPathArray.length();
		static const MObject directions[] = { HelixBase::aForward, HelixBase::aBackward };

		for(unsigned int i = 0; i < targets_length; ++i) {
			for(int j = 0; j < 2; ++j) {
				MDagPath itDagPath = dagPathArray[i];
				MObject itObject;

				bool firstIt = true; // Ugly, but the easiest way to solve the problem with loops

				do {
					itObject = itDagPath.node(&status);

					if (!status) {
						status.perror("MDagPathArray[i]::node");
						return status;
					}

					// Ok here's a potential neighbour object, if it is already in the list, we must BREAK the search as there might be connection loops

					if (!firstIt) {
						bool alreadyExists = false;

						for (unsigned int k = 0; k < neighbourBase_dagPaths.length(); ++k) {
							if (neighbourBase_objects[k] == itObject) {
								alreadyExists = true;
								break;
							}
						}

						if (alreadyExists)
							break;
					}
					else
						firstIt = false;

					// This base should be colored

					if (!(status = neighbourBase_dagPaths.append(itDagPath))) {
						status.perror("MDagPathArray::append");
						return status;
					}

					if (!(status = neighbourBase_objects.append(itObject))) {
						status.perror("MObjectArray::append");
						return status;
					}
				}
				while (HelixBase_NextBase(itObject, directions[j], itDagPath, &status));

				if (!status) {
					status.perror("HelixBase_NextBase");
					return status;
				}
			}
		}

		return paintBases(neighbourBase_dagPaths);
	}

	MStatus PaintStrand::paintBases(MDagPathArray & dagPathArray) {
		MStatus status;

		// Color the bases

		MStringArray materials;

		if (!(status = DNA::GetMaterials(materials))) {
			status.perror("DNA::GetMaterials");
			return status;
		}

		if (materials.length() == 0) {
			std::cerr << "Found no materials to choose from" << std::endl;
			return MStatus::kFailure;
		}

		m_currentPaintedColors = 0;
		m_previousPaintedDagNodes = dagPathArray;

		if (!(status = DNA::QueryMaterialsOfObjects(dagPathArray, m_paintedColors[(m_currentPaintedColors + 1) % 2]))) {
			status.perror("DNA::GetMaterialsOfObjects");
			return status;
		}

		unsigned int chosenMaterial = ((unsigned int) rand()) % materials.length();

		// Save the current materials into the array...
		m_paintedColors[m_currentPaintedColors].setLength(1);
		m_paintedColors[m_currentPaintedColors] [0] = materials[chosenMaterial];

		if (!(status = DNA::AssignMaterialToObjects(dagPathArray, materials[chosenMaterial]))) {
			status.perror("DNA::AssignMaterialToObjects");
			return status;
		}

		return MStatus::kSuccess;
	}
}
