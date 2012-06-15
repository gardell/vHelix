/*
 * Connect.cpp
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#include <Connect.h>
#include <HelixBase.h>
#include <Utility.h>

#include <model/Helix.h>

#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>
#include <maya/MArgDatabase.h>
#include <maya/MDGModifier.h>
#include <maya/MPlugArray.h>


namespace Helix {
	Connect::Connect() {

	}

	Connect::~Connect() {

	}

	MStatus Connect::doIt(const MArgList & args) {
		MObject targets[] = { MObject::kNullObj, MObject::kNullObj };
		static const char *flags[] = { "-f", "-s" };

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		for(size_t i = 0; i < 2; ++i) {
			if (argDatabase.isFlagSet(flags[i], &status)) {
				MString str_target;

				if (!(status = argDatabase.getFlagArgument(flags[i], 0, str_target))) {
					status.perror("MArgDatabase::getFlagArgument");
					return status;
				}

				MSelectionList selectionList;
				if (!(status = selectionList.add(str_target))) {
					status.perror("MSelectionList:add");
					return status;
				}

				if (!(status = selectionList.getDependNode(0, targets[i]))) {
					status.perror("MSelectionList::getDependNode");
					return status;
				}
			}
		}

		// If arguments are missing, use the first two currently selected bases
		//

		size_t missingObjects_count = 0;

		for(size_t i = 0; i < 2; ++i) {
			if (targets[i] == MObject::kNullObj)
				missingObjects_count++;
		}

		if (missingObjects_count > 0) {
			MObjectArray selectedObjects;

			if (!(status = Model::Base::AllSelected(selectedObjects))) {
				status.perror("Helix::AllSelected");
				return status;
			}

			if (missingObjects_count > selectedObjects.length()) {
				MGlobal::displayError("There are not enough bases given, either by argument or by select. Two bases are required");
				return MStatus::kFailure;
			}

			unsigned int selectedObjects_index = 0;

			for(unsigned int i = 0; i < 2; ++i) {
				if (targets[i] == MObject::kNullObj) {
					targets[i] = selectedObjects[selectedObjects_index++];
				}
			}
		}

		/*
		 * Make the connection
		 */

		Model::Base source(targets[0]); // Because the functor takes Strand and the Strand constructor from Base passes by reference

		if (!(status = m_operation.connect(source, targets[1]))) {
			status.perror("Connect::connect");
			return status;
		}

		if (!(status = m_functor.loadMaterials())) {
			status.perror("PaintMultipleStrandsWithNewColorFunctor::loadMaterials");
			return status;
		}

		m_functor(source);

		return m_functor.status();
	}

	MStatus Connect::undoIt () {
		MStatus status;
		if (!(status = m_operation.undo())) {
			status.perror("Connect::undo");
			return status;
		}

		if (!(status = m_functor.undo())) {
			status.perror("PaintMultipleStrandsWithNewColor::undo");
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus Connect::redoIt () {
		MStatus status;

		if (!(status = m_operation.redo())) {
			status.perror("Connect::redo");
			return status;
		}

		return m_functor.redo();
	}

	bool Connect::isUndoable () const {
		return true;
	}

	bool Connect::hasSyntax () const {
		return true;
	}

	MSyntax Connect::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-f", "-first", MSyntax::kString);
		syntax.addFlag("-s", "-second", MSyntax::kString);

		return syntax;
	}

	void *Connect::creator() {
		return new Connect();
	}
}
