/*
 * Disconnect.cpp
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#include <HelixBase.h>

#include <Disconnect.h>
#include <Utility.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MDGModifier.h>

#include <list>

#include <model/Base.h>
#include <controller/Disconnect.h>

#include <iterator>

namespace Helix {
	Disconnect::Disconnect() {

	}

	Disconnect::~Disconnect() {

	}

	MStatus Disconnect::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		std::list<Model::Base> targets;

		if (argDatabase.isFlagSet("-t", &status)) {
			MString target_str;

			if (!(status = argDatabase.getFlagArgument("-t", 0, target_str))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;
			if (!(status = selectionList.add(target_str))) {
				status.perror("Disconnect::doIt: MSelectionList::add");
				return status;
			}

			MObject target;

			if (!(status = selectionList.getDependNode(0, target))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			targets.push_back(Model::Base(target));
		}

		if (!status) {
			status.perror("MArgDatabase::isFlagSet");
			return status;
		}

		if (targets.size() == 0) {
			// Find out our targets by select
			//

			MObjectArray bases;
			
			if (!(status = Model::Base::AllSelected(bases))) {
				status.perror("Base::AllSelected");
				return status;
			}

			/*
			 * Copy over to our std::list of Base objects
			 */

			std::copy(&bases[0], &bases[0] + bases.length(), std::back_insert_iterator< std::list<Model::Base> > (targets));
		}

		/*
		 * Ok we have our targets, now actually disconnect them using the DisconnectController
		 */

		std::for_each(targets.begin(), targets.end(), m_operation.execute());

		/*
		 * Now color all the recently disconnected bases in a new color
		 */

		if (!(status = m_functor.loadMaterials())) {
			status.perror("PaintMultipleStrandsWithNewColorFunctor::loadMaterials");
			return status;
		}

		for_each_ref(targets.begin(), targets.end(), m_functor);

		if (!m_operation.status())
			return m_operation.status();

		return m_functor.status();
	}

	MStatus Disconnect::undoIt () {
		MStatus status = m_operation.undo();

		if (!status) {
			status.perror("Controller::Disconnect::undo");
			return status;
		}

		return m_functor.undo();
	}

	MStatus Disconnect::redoIt () {
		MStatus status = m_operation.redo();

		if (!status) {
			status.perror("Controller::Disconnect::redo");
			return status;
		}

		return m_functor.redo();
	}

	bool Disconnect::isUndoable () const {
		return true;
	}

	bool Disconnect::hasSyntax () const {
		return true;
	}

	MSyntax Disconnect::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *Disconnect::creator() {
		return new Disconnect();
	}
}
