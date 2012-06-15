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

#include <model/Helix.h>
#include <model/Strand.h>
#include <model/Base.h>

namespace Helix {
	PaintStrand::PaintStrand() {

	}

	PaintStrand::~PaintStrand() {

	}

	MStatus PaintStrand::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		std::list<Model::Strand> targets;

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
				status.perror("PaintStrand::doIt: MSelectionList::add");
				return status;
			}

			MDagPath targetDagPath;
			MObject targetObject;

			if (!(status = selectionList.getDagPath(0, targetDagPath, targetObject))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}

			Model::Base base(targetDagPath);
			targets.push_back(Model::Strand(base));
		}

		if (targets.size() == 0) {
			// Find the targets by using the list of currently selected bases
			//

			MObjectArray selectedBases;
			if (!(status = Model::Base::AllSelected(selectedBases))) {
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

				Model::Base base(dagPath);
				targets.push_back(Model::Strand(base));
			}
		}

		if (targets.size() == 0) {
			MGlobal::displayError("Nothing to paint");
			return MStatus::kFailure;
		}

		/*
		 * This is the actual code for painting the selected strands (bases)
		 * Using the redefined STL approach, we can just use std::for_each on the strands using the iterators provided by Model::Strand
		 */

		if (!(status = m_functor.loadMaterials())) {
			status.perror("PaintMultipleStrandsFunctor");
			return status;
		}

		for_each_ref(targets.begin(), targets.end(), m_functor);

		return m_functor.status();
	}

	MStatus PaintStrand::undoIt () {
		return m_functor.undo();
	}

	MStatus PaintStrand::redoIt () {
		return m_functor.redo();
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
}
