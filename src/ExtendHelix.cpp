/*
 * ExtendHelix.cpp
 *
 *  Created on: 11 jul 2011
 *      Author: johan
 */

#include <ExtendHelix.h>
#include <HelixBase.h>
#include <Utility.h>
#include <DNA.h>

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPathArray.h>
#include <maya/MDGModifier.h>

#include <vector>
#include <cmath>

namespace Helix {
	ExtendHelix::ExtendHelix() {

	}

	ExtendHelix::~ExtendHelix() {

	}

	// Dummy class for storaging the base pairs in an array

	class BasePairInfo {
	public:
		inline BasePairInfo(MDagPath base, unsigned int end) : m_end(end) {
			m_dagPaths.append(base);
		}

		inline BasePairInfo(MDagPath left_base, MDagPath right_base, unsigned int end, bool reversed = false) : m_end(end), m_reversed(reversed) {
			m_dagPaths.append(left_base);
			m_dagPaths.append(right_base);
		}

		unsigned int m_end;
		bool m_reversed; // Are the left/right reversed? Only used for naming scaf/stap
		MDagPathArray m_dagPaths;
	};

	// Helper method that extends on an end base using the direction given. Note that it will also find the opposite base and try to make label connections on the new bases if possible
	// (repairing a single strand into two)
	// strandType is 0 for scaf, 1 for stap, 2 for unknown

	MStatus ExtendBase_recursive(MObject & helix, MObject & base, const MObject & forward, const MObject & backward, int count, int index, unsigned int strandType, double direction, MDagPath *opposite = NULL) {
		// Extend this base with a new one. If opposite is non-null, find if it has an extension, in that case, connect its labels (remember connection direction!)
		// Then call us recursively until count reaches zero

		MStatus status;
		MObject new_base;
		MDagPath new_opposite;

		static const char *names[] = { "scaf", "stap", "strand" };
		bool isSource, has_new_opposite = false;

		if (opposite) {
			MPlug labelPlug(base, HelixBase::aLabel);

			isSource = labelPlug.isSource(&status);

			if (!status) {
				status.perror("MPlug::isSource");
				return status;
			}

			MObject opposite_object = opposite->node(&status);

			if (!status) {
				status.perror("MDagPath::node");
				return status;
			}

			// Note that for the opposite, it's backward and not forward (the opposite strand is reversed)
			has_new_opposite = HelixBase_NextBase(opposite_object, backward, new_opposite, &status);

			if (!status) {
				status.perror("HelixBase_NextBase");
				return status;
			}
		}

		// Figure out this base's location

		MVector translation;

		{
			MFnTransform base_transform(base);

			translation = base_transform.getTranslation(MSpace::kTransform, &status);

			if (!status) {
				status.perror("MFnTransform::getTranslation");
				return status;
			}
		}

		// FIXME: Do a correct translation

		double angle = atan2(translation.y, translation.x);// + (direction < 0.0 ? M_PI : 0.0);

		//std::cerr << "for base index: " << index << ", angle is " << angle << ", that is " << RAD2DEG(angle) << " degrees, strandType is " << strandType << ", direction: " << direction << std::endl;

		if (!(status = Helix_CreateBase(helix,
										MString(names[strandType]) + "_extend_" +  index,
										MVector(DNA::RADIUS * cos(angle - direction * DEG2RAD(-DNA::PITCH)), DNA::RADIUS * sin(angle - direction * DEG2RAD(-DNA::PITCH)), translation.z + direction * DNA::STEP),
										new_base))) {
			status.perror("Helix_CreateBase");
			return status;
		}

		MDGModifier dgModifier;

		// Connect this base to the previous base
		//

		if (!(status = dgModifier.connect(base, forward, new_base, backward))) {
			status.perror("MDGModifier::connect 1");
			return status;
		}

		// Connect it to the new_opposite if applicable

		if (has_new_opposite) {
			MObject new_opposite_object = new_opposite.node(&status);

			if (!status) {
				status.perror("MDagPath::node");
				return status;
			}

			if (isSource) {
				if (!(status = dgModifier.connect(new_base, HelixBase::aLabel, new_opposite_object, HelixBase::aLabel))) {
					status.perror("MDGModifier::connect 2");
					return status;
				}
			}
			else {
				if (!(status = dgModifier.connect(new_opposite_object, HelixBase::aLabel, new_base, HelixBase::aLabel))) {
					status.perror("MDGModifier::connect 3");
					return status;
				}
			}
		}

		if (!(status = dgModifier.doIt())) {
			status.perror("MDGModifier::doIt 1");
			return status;
		}

		if (count - 1 > 0)
			return ExtendBase_recursive(helix, new_base, forward, backward, count - 1, index + 1, strandType, direction, has_new_opposite ? &new_opposite : NULL);
		return MStatus::kSuccess;
	}

	MStatus ExtendBase(MObject & base, const MObject & forward, const MObject & backward, int count) {
		MStatus status;
		MDagPath opposite_dagPath;
		MObject helix;

		bool hasOpposite = HelixBase_NextBase(base, HelixBase::aLabel, opposite_dagPath, &status), isSource;

		if (!status) {
			status.perror("HelixBase_NextBase");
			return status;
		}

		{
			MPlug labelPlug(base, HelixBase::aLabel);

			isSource = labelPlug.isSource(&status);

			if (!status) {
				status.perror("MPlug::isSource");
				return status;
			}
		}

		MFnDagNode base_dagNode(base);

		helix = base_dagNode.parent(0, &status);

		if (!status) {
			status.perror("MFnDagNode::parent");
			return status;
		}

		// Try to figure out what direction etc to extend along
		//

		unsigned int strandType;
		double direction;

		if (hasOpposite) {
			if (isSource) {
				if (forward == HelixBase::aForward) {
					strandType = 0;
					direction = 1.0;
				}
				else {
					strandType = 0;
					direction = -1.0;
				}
			}
			else {
				if (forward == HelixBase::aBackward) {
					strandType = 1;
					direction = 1.0;
				}
				else {
					strandType = 1;
					direction = -1.0;
				}
			}
		}
		else {
			// This one is a little bit more tricky, extend along the axis by looking at previous bases
			MDagPath prev_base;

			bool has_prev_base = HelixBase_NextBase(base, backward, prev_base, &status);

			if (!status) {
				status.perror("HelixBase_NextBase");
				return status;
			}

			if (has_prev_base) {
				// Calculate the direction using the prev base and this base's positions

				MFnTransform prev_base_transform(prev_base), base_transform(base);

				MVector base_translation = base_transform.getTranslation(MSpace::kTransform, &status);

				if (!status) {
					status.perror("MFnTransform::getTranslation");
					return status;
				}

				MVector prev_base_translation = prev_base_transform.getTranslation(MSpace::kTransform, &status);

				if (!status) {
					status.perror("MFnTransform::getTranslation");
					return status;
				}

#if defined(WIN32)
				direction = _copysign(1.0, (base_translation - prev_base_translation).z);
#else
				direction = copysign(1.0, (base_translation - prev_base_translation).z);
#endif /* WIN32 */

			}
			else {
				// This base doesn't have a prev base and is marked for extension in the forward direction, in other words, it doesn't matter what direction we choose

				strandType = isSource ? 0 : (hasOpposite ? 1 : 2);
			}
		}

		return ExtendBase_recursive(helix, base, forward, backward, count, 1, strandType, direction, hasOpposite ? &opposite_dagPath : NULL);
	}

	MStatus ExtendHelix::doIt(const MArgList & args) {
		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);
		MObjectArray targets;
		int bases = 0;

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-b")) {
			if (!(status = argDatabase.getFlagArgument("-b", 0, bases))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}
		}

		if (bases == 0) {
			MGlobal::displayError("No valid number of bases to generate given");
			return MStatus::kFailure;
		}

		if (argDatabase.isFlagSet("-t")) {
			MString targetName;

			if (!(status = argDatabase.getFlagArgument("-t", 0, targetName))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;

			if (!(status = selectionList.add(targetName))) {
				status.perror("ExtendHelix::doIt: MSelectionList::add");
				return status;
			}

			MDagPath target_dagPath;
			MObject target_object;

			if (!(status = selectionList.getDagPath(0, target_dagPath, target_object))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}

			if (!(status = targets.append(target_object))) {
				status.perror("MObjectArray::append");
				return status;
			}
		}
		else {
			// Find argument by select
			//

			if (!(status = SelectedBases(targets))) {
				status.perror("SelectedBases");
				return status;
			}
		}

		// Iterate over all bases, make sure they're end bases.
		// If, for example, we're extending a base which pair already has a single strand longer, we will automatically pair the new ones with the old strand
		// If it's the other way around, no new pairs will be created
		// If the user selects a base pair connected, it will work as in the first case
		// If the user selects two bases that are not an immediate pair, it will work as in the first case too
		//

		for (unsigned int i = 0; i < targets.length(); ++i) {
			unsigned int endType = HelixBase_endType(targets[i], &status);

			if (!status) {
				status.perror("HelixBase_endType");
				return status;
			}

			if (endType == Neither) {
				MGlobal::displayWarning(MString("The base named \"") + MFnDagNode(targets[i]).fullPathName() + "\" can not be extruded. It is not an end base");
				continue;
			}
			else if (endType == FivePrime) {
				if (!(status = ExtendBase(targets[i], HelixBase::aBackward, HelixBase::aForward, bases))) {
					status.perror("ExtendBase FivePrime");
					return status;
				}
			}
			else if (endType == ThreePrime) {
				if (!(status = ExtendBase(targets[i], HelixBase::aForward, HelixBase::aBackward, bases))) {
					status.perror("ExtendBase ThreePrime");
					return status;
				}
			}
		}

		return MStatus::kSuccess;

		// Now, iterate over the objects, find their base pairs, and append them as a pair, if they're not already appended
		//

		//std::vector < std::pair<MDagPath, MDagPath> > base_pairs;
		/*std::vector<BasePairInfo> base_pairs;

		for (size_t i = 0; i < targets.length(); ++i) {
			MDagPath this_dagPath, opposite_dagPath;
			MPlug labelPlug(targets[i], HelixBase::aLabel);

			bool isOpposite = labelPlug.isDestination(&status);

			if (!status) {
				status.perror("MPlug::isDestination");
				return status;
			}

			MFnDagNode this_dagNode(targets[i]);

			if (!(status = this_dagNode.getPath(this_dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			unsigned int this_endType = HelixBase_endType(targets[i], &status);

			if (!status) {
				status.perror("HelixBase_endType");
				return status;
			}

			if (!HelixBase_NextBase(targets[i], HelixBase::aLabel, opposite_dagPath, &status)) {
				// This base does not have an opposite base, we're only extending the scaffold

				// Is it already added?
				bool alreadyExists = false;

				for(std::vector<BasePairInfo>::iterator it = base_pairs.begin(); it != base_pairs.end(); ++it) {
					if (it->m_dagPaths[0].node() == this_dagPath.node()) {
						alreadyExists = true;
						break;
					}
				}

				if (!alreadyExists)
					base_pairs.push_back(BasePairInfo(this_dagPath, MDagPath(), this_endType));
				continue;
			}

			if (!status) {
				status.perror("HelixBase_NextBase");
				return status;
			}

			MObject opposite_object = opposite_dagPath.node(&status);

			if (!status) {
				status.perror("MDagPath::node");
				return status;
			}

			unsigned int opposite_endType = HelixBase_endType(opposite_object, &status);

			if (!status) {
				status.perror("HelixBase_endType 2");
				return status;
			}

			if (this_endType == Neither && opposite_endType == Neither) {
				MGlobal::displayWarning(MString("The selected bases \"") + this_dagPath.fullPathName() + "\" and \"" + opposite_dagPath.fullPathName() + "\" is neither an end base. The base pair will not be extended");
				continue;
			}

			// Ok we have a base pair, if it's not already added, add it
			//

			std::cerr << "Base pair: " << this_dagPath.fullPathName().asChar() << ", " << opposite_dagPath.fullPathName().asChar() << std::endl;

			if (isOpposite) {
				bool alreadyExists = false;

				for(std::vector<BasePairInfo>::iterator it = base_pairs.begin(); it != base_pairs.end(); ++it) {
					if (it->m_dagPaths[1].node() == this_dagPath.node() && it->m_dagPaths[0].node() == opposite_dagPath.node()) {
						alreadyExists = true;
						break;
					}
				}

				if (!alreadyExists)
					base_pairs.push_back(BasePairInfo(opposite_dagPath, this_dagPath, ((this_endType & FivePrime) & (opposite_endType & FivePrime)) | ((this_endType & ThreePrime) & (opposite_endType & ThreePrime)), true));
			}
			else {
				bool alreadyExists = false;

				for(std::vector<BasePairInfo>::iterator it = base_pairs.begin(); it != base_pairs.end(); ++it) {
					if (it->m_dagPaths[0].node() == this_dagPath.node() && it->m_dagPaths[1].node() == opposite_dagPath.node()) {
						alreadyExists = true;
						break;
					}
				}

				if (!alreadyExists)
					base_pairs.push_back(BasePairInfo(opposite_dagPath, this_dagPath, ((this_endType & FivePrime) & (opposite_endType & FivePrime)) | ((this_endType & ThreePrime) & (opposite_endType & ThreePrime))));
			}
		}

		// We have a list of base pairs that should be unique, now do the actual extend
		//

		MString info("Extend on the following base pairs: ");

		for(std::vector<BasePairInfo>::iterator it = base_pairs.begin(); it != base_pairs.end(); ++it)
			info += MString("<") + it->m_dagPaths[0].fullPathName() + (it->m_dagPaths.length() > 1 ? MString(", ") + it->m_dagPaths[1].fullPathName() : "") + "> ";

		MGlobal::displayInfo(info + " by " + bases + " new bases");

		// Note, there are several cases that can occur here:
		// 1: There's only a valid end base on the first attribute and none on the second, in this case we extend a single scaffold
		// 2: There are two valid end bases
		// 3: One of the bases is an end base while the other has a scaffold beoynd this index, in this case we need to make sure the new bases appended to the end base gets connected to the already existing scaffold

		for(std::vector <BasePairInfo>::iterator it = base_pairs.begin(); it != base_pairs.end(); ++it) {
			// Extend this end by `bases` new bases.

			MStatus status[2];
			MDagPathArray prev_dagPath = it->m_dagPaths;
			MObjectArray prev_object;

			for(size_t i = 0; i < prev_dagPath.length(); ++i) {
				prev_object.append(prev_dagPath[i].node(&status[i]));

				if (!status[i]) {
					status[0].perror("MDagPath::node");
					return status[0];
				}
			}

			// Setup some generic information required, like the axis to extend along, and the attributes to connect
			//

			MVector extend_direction;
			MObject forward_attribute, backward_attribute; // Use these instead of HelixBase::a*.

			if (it->m_end == FivePrime) {
				extend_direction = MVector::zNegAxis;

				forward_attribute = HelixBase::aBackward;
				backward_attribute = HelixBase::aForward;

			}
			else if (it->m_end == ThreePrime) {
				extend_direction = MVector::zAxis;

				forward_attribute = HelixBase::aForward;
				backward_attribute = HelixBase::aBackward;
			}
			else {
				// FIXME ? We have a single base that we could extend in two ways..
				MGlobal::displayWarning(MString("Can't extend the base pair \"") + it->m_dagPaths[0].fullPathName() + "\" " + (it->m_dagPaths.length() > 1 ? it->m_dagPaths[1].fullPathName() + " " : "") + " because the direction of extension is unknown");
				continue;
			}

			for (int i = 0; i < bases; ++i) {
				// Create new bases where required and make sure their labels get connected, connect the bases to the prev_* objects
				MDagPathArray next_dagPath;
				MObjectArray next_object;

				for(size_t j = 0; j < prev_dagPath.length(); ++j) {
					MDagPath result;
					bool hasNextBase = HelixBase_NextBase(prev_object[j], forward_attribute, result, &status[0]);

					if (!status[0]) {
						status[0].perror("HelixBase_NextBase");
						return status[0];
					}

					if (!hasNextBase) {
						// Create the next base

						// FIXME: Create the next base, connect it's backward_attribute to the prev

						// Append it to the next_* arrays
					}
					else {
						next_dagPath.append(result);
						next_object.append(result.node(&status[0]));

						if (!status[0]) {
							status[0].perror("MDagPath::node");
							return status[0];
						}
					}
				}

				// Connect the bases to each others label attributes

				// FIXME: Connect them!

				// Finally set the prev_* objects to these bases

				prev_dagPath = next_dagPath;

				// For some reason, the MObjectArray.operator = is private and can not be used
				prev_object.clear();
				for (size_t j = 0; j < next_object.length(); ++j)
					prev_object.append(next_object[i]);
			}

			// Now extend the cylinders accordingly
			//

			// FIXME: Cylinders
		}

		return MStatus::kSuccess;*/
	}

	MStatus ExtendHelix::undoIt () {
		// FIXME

		std::cerr << "Can't do undo yet" << std::endl;

		return MStatus::kSuccess;
	}

	MStatus ExtendHelix::redoIt () {
		// FIXME

		std::cerr << "Can't do redo yet" << std::endl;

		return MStatus::kSuccess;
	}

	bool ExtendHelix::isUndoable () const {
		return true;
	}

	bool ExtendHelix::hasSyntax () const {
		return true;
	}

	MSyntax ExtendHelix::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-b", "-base", MSyntax::kLong);
		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *ExtendHelix::creator() {
		return new ExtendHelix();
	}
}
