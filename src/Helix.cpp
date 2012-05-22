/*
 * Helix.cpp
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#include <Helix.h>
#include <maya/MFnMatrixAttribute.h>

namespace Helix {
	MObject Helix::aLeftStrand, Helix::aRightStrand;
	MTypeId Helix::id(HELIX_HELIX_ID);

	Helix::Helix() {

	}

	Helix::~Helix() {

	}

	void *Helix::creator() {
		return new Helix();
	}

	MStatus Helix::initialize() {
		MStatus status;

		MFnMatrixAttribute leftStrandAttr, rightStrandAttr;

		aLeftStrand = leftStrandAttr.create("left_strand", "ls", MFnMatrixAttribute::kDouble, &status);

		if (!status) {
			status.perror("MFnMatrixAttribute::create");
			return status;
		}

		aRightStrand = rightStrandAttr.create("right_strand", "rs", MFnMatrixAttribute::kDouble, &status);

		if (!status) {
			status.perror("MFnMatrixAttribute::create");
			return status;
		}

		/*leftStrandAttr.setArray(true);
		rightStrandAttr.setArray(true);
		leftStrandAttr.setStorable(false);
		rightStrandAttr.setStorable(false);*/

		addAttribute(aLeftStrand);
		addAttribute(aRightStrand);

		return MStatus::kSuccess;
	}
}
