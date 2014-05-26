#include <FillStrandGaps.h>
#include <Utility.h>

#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MSyntax.h>

#include <Helix.h>
#include <HelixBase.h>

namespace Helix {
	FillStrandGaps::FillStrandGaps() {

	}

	FillStrandGaps::~FillStrandGaps() {

	}

	MStatus FillStrandGaps::doIt(const MArgList & args) {
		std::list<MObject> targets;
		MStatus status = ArgList_GetModelObjects(args, syntax(), "-t", targets);
		if (status != MStatus::kNotFound && status != MStatus::kSuccess) {
			HMEVALUATE_RETURN_DESCRIPTION("ArgList_GetModelObjects", status);
		}

		if (targets.empty()) {
			MSelectionList activeSelectionList;
			HMEVALUATE_RETURN(status = MGlobal::getActiveSelectionList(activeSelectionList), status);

			if (activeSelectionList.length() > 0) {
				for (unsigned int i = 0; i < activeSelectionList.length(); ++i) {
					MObject object;

					HMEVALUATE_RETURN(status = activeSelectionList.getDependNode(i, object), status);

					MFnDagNode dagNode(object);
					MTypeId typeId;
					HMEVALUATE_RETURN(typeId = dagNode.typeId(&status), status);

					if (typeId == HelixBase::id || typeId == Helix::id)
						targets.push_back(object);
					else
						MGlobal::displayWarning(MString("Ignoring unknown object \"") + dagNode.fullPathName() + "\"");
				}
			}
			else {
				/*
				 * Extract all helices
				 */

				MItDag itDag(MItDag::kDepthFirst, MFn::kPluginTransformNode, &status);
				HMEVALUATE_RETURN_DESCRIPTION("MItDag::#ctor", status);

				for (; !itDag.isDone(); itDag.next()) {
					MObject object;
					HMEVALUATE_RETURN(object = itDag.currentItem(&status), status);

					bool is_helix;
					HMEVALUATE_RETURN(is_helix = MFnDagNode(object).typeId(&status) == Helix::id, status);
					if (is_helix)
						targets.push_back(object);
				}
			}
		}

		return m_operation.fill(targets);
	}

	MStatus FillStrandGaps::undoIt() {
		return m_operation.undo();
	}

	MStatus FillStrandGaps::redoIt() {
		return m_operation.redo();
	}

	bool FillStrandGaps::isUndoable() const {
		return true;
	}

	bool FillStrandGaps::hasSyntax() const {
		return true;
	}


	MSyntax FillStrandGaps::newSyntax() {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);
		syntax.makeFlagMultiUse("-t");

		return syntax;
	}

	void *FillStrandGaps::creator() {
		return new FillStrandGaps();
	}
}
