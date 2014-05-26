#include <StrandLengthCount.h>

#include <model/Base.h>

#include <maya/MSyntax.h>

namespace Helix {
	StrandLengthCount::~StrandLengthCount() {

	}

	MStatus StrandLengthCount::doIt(const MArgList & args) {
		std::list<MObject> targets;
		MStatus status = ArgList_GetModelObjects(args, syntax(), "-b", targets);
		if (status != MStatus::kNotFound && status != MStatus::kSuccess) {
			HMEVALUATE_RETURN_DESCRIPTION("ArgList_GetModelObjects", status);
		}

		clearResult();
		if (!targets.empty()) {
			for (std::list<MObject>::const_iterator it(targets.begin()); it != targets.end(); ++it) {
				Model::Base base(*it);
				Model::Strand strand(base);
				HPRINT("Calling with base: %s", base.getDagPath(status).fullPathName().asChar());
				appendToResult(int(m_operation.length(strand)));
			}
		} else {
			MObjectArray objects;
			HMEVALUATE_RETURN(status = Model::Base::AllSelected(objects), status);

			for (unsigned int i = 0; i < objects.length(); ++i) {
				Model::Base base(objects[i]);
				Model::Strand strand(base);
				HPRINT("Calling with base: %s", base.getDagPath(status).fullPathName().asChar());
				appendToResult(int(m_operation.length(strand)));
			}
		}

		return MStatus::kSuccess;
	}

	MStatus StrandLengthCount::undoIt() {
		return MStatus::kSuccess;
	}

	MStatus StrandLengthCount::redoIt() {
		return MStatus::kSuccess;
	}

	bool StrandLengthCount::isUndoable() const {
		return false;
	}

	bool StrandLengthCount::hasSyntax() const {
		return true;
	}

	MSyntax StrandLengthCount::newSyntax() {
		MSyntax syntax;
		syntax.addFlag("-b", "-base", MSyntax::kString);
		syntax.makeFlagMultiUse("-b");

		return syntax;
	}

	void *StrandLengthCount::creator() {
		return new StrandLengthCount();
	}
}
