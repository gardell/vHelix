#include <controller/ExtendStrand.h>
#include <model/Helix.h>

#include <maya/MFnDagNode.h>
#include <Utility.h>

#include <functional>

namespace Helix {
	namespace Controller {
		MStatus ExtendStrand::doExecute(Model::Base & element) {
			/*
			 * There are two cases to consider here:
			 * - the base has no opposite base, or the opposite base has no connection in the direction (z-axis, not strand direction) we're extracting
			 *		just extend our strand
			 * - the base has an opposite base *and* it has a single stranded extension
			 *		extend our strand but connect each new base to the opposite equivalent base
			 */

			MStatus status;

			onProgressBegin(m_length);

			MDagPath element_dagPath = element.getDagPath(status);

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			MString element_name = MFnDagNode(element_dagPath).name(&status);

			if (!status) {
				status.perror("MFnDagNode::name");
				return status;
			}

			/*
			 * Figure out the bases translation. We add to this in order to create the rotation for the following bases
			 */

			MVector translation;

			if (!(status = element.getTranslation(translation, MSpace::kTransform))) {
				status.perror("Base::getTranslation");
				return status;
			}

			double angle = atan2(translation.y, translation.x);

			/*
			 * This is for the undo process, we track all the newly created bases so we're able to delete them in case of an undo
			 */

			std::vector<Model::Base> created_bases;
			created_bases.reserve(m_length);

			/*
			 * This looks a bit messy, but in order to save some space not duplicating the code for the two different directional cases (extending backward or forward)
			 * we use member pointers
			 */

			Model::Base (Model::Base::* target_forward) (MStatus &),
						(Model::Base::* target_backward) (MStatus &);

			bool hasForward, hasBackward, extendForward, extendPositiveZ;
			double direction;
			unsigned int positive_count = 0, negative_count = 0;

			Model::Base forward_base = element.forward(status);

			if (!status && status != MStatus::kNotFound) {
				status.perror("Base::forward");
				return status;
			}

			hasForward = status != MStatus::kNotFound;

			Model::Base backward_base = element.backward(status);

			if (!status && status != MStatus::kNotFound) {
				status.perror("Base::backward");
				return status;
			}

			hasBackward = status != MStatus::kNotFound;

			/*
			 * Now, depending on wether we have hasForward or hasBackward, decide upon the direction of extension
			 */

			if (!hasForward && hasBackward) {
				// We have no forward, extend forward

				target_forward = &Model::Base::forward;
				target_backward = &Model::Base::backward;
				extendForward = true;

				/*
				 * Figure out what direction using the z-axis distance between this base and its backward base
				 */

				MVector backward_base_translation;
				
				if (!(status = backward_base.getTranslation(backward_base_translation, MSpace::kTransform))) {
					status.perror("Base::getTranslation 1");
					return status;
				}

				int sign = sgn(translation.z - backward_base_translation.z);
				direction = (double) sign;
				extendPositiveZ = sign > 0;
			}
			else if (hasForward && !hasBackward) {
				// We already have a forward connection but no backward, extend backward
				target_forward = &Model::Base::backward;
				target_backward = &Model::Base::forward;
				extendForward = false;

				/*
				 * Figure out what direction using the z-axis distance between this base and its backward base
				 */

				MVector forward_base_translation;
				
				if (!(status = forward_base.getTranslation(forward_base_translation, MSpace::kTransform))) {
					status.perror("Base::getTranslation 2");
					return status;
				}

				int sign = sgn(translation.z - forward_base_translation.z);
				direction = (double) sign;

				extendPositiveZ = sign > 0;
			}
			else {
				/*
				 * Doesn't make sense, the user is trying to extend a base that is already occupied
				 * or he is trying to extend on a single base. We can't do that because we can't figure out what direction to take
				 *	in theory we could just choose any direction, but if the base does in fact have an opposite base, we might mess up the structure
				 */
				

				if (hasForward && hasBackward)
					MGlobal::displayWarning(MString("Ignoring base ") + element.getDagPath(status).fullPathName() + " since it is not and end base");
				else
					MGlobal::displayWarning(MString("Ignoring base ") + element.getDagPath(status).fullPathName() + " since it has no forward or backward connection it is not possible to choose a proper direction and rotation to extend along");

				onProgressDone();

				return MStatus::kSuccess;
			}

			/*
			 * Figure out if the current base has an opposite base and if it already has a connection in the direction we're extending
			 */

			bool hasOppositeStrand = false, isDestinationForOpposite;

			Model::Base opposite_base = element.opposite(status);

			if (!status && status != MStatus::kNotFound) {
				status.perror("Base::opposite");
				return status;
			}

			if (status != MStatus::kNotFound) {
				(opposite_base.*target_backward) (status);

				if (!status && status != MStatus::kNotFound) {
					status.perror("Base::*target_forward");
					return status;
				}

				if (status) {
					/*
					 * The opposite base has connections in the desired direction. They must be taken into account
					 */

					hasOppositeStrand = true;

					std::cerr << "Has opposite base" << std::endl;

					isDestinationForOpposite = element.opposite_isDestination(status);

					if (!status) {
						status.perror("Base::opposite_isDestination");
						return status;
					}
				}
			}

			/*
			 * Setup material handling
			 */

			Model::Material material;

			if (!(status = element.getMaterial(material)))
				status.perror("Base::getMaterial");

			/*
			 * Find the helix this base belongs to
			 */

			Model::Helix helix = element.getParent(status);

			if (!status) {
				status.perror("Base::getParent");
				return status;
			}

			/*
			 * Now finally extend the strand
			 * Note: Whenever we extend and there is no opposite base to attach to, even though hasOppositeStrand is set
			 *	we must add to the *_count variables as we're extending the total length of the helix.
			 *  note that the opposite strand, if it exist, can be shorter than the number we're going to extend
			 *  thus getting the opposite can fail even though hasOppositeStrand is true
			 */

			Model::Base previous_base = element, previous_opposite = opposite_base;

			for(unsigned int i = 0; i < m_length; ++i) {
				/*
				 * Create the new base and position it
				 */

				MVector new_translation(DNA::ONE_MINUS_SPHERE_RADIUS * cos(angle - direction * (i + 1) * DEG2RAD(-DNA::PITCH)),
						DNA::ONE_MINUS_SPHERE_RADIUS * sin(angle - direction * (i + 1) * DEG2RAD(-DNA::PITCH)),
						translation.z + direction * (i + 1) * DNA::STEP);

				Model::Base base;

				MString name = element_name + "_extend_" + (i + 1);

				if (!(status = Model::Base::Create(helix, name, new_translation, base))) {
					status.perror("Base::Create");
					return status;
				}

				/*
				 * Setup material
				 */

				if (!(status = base.setMaterial(material))) {
					status.perror("Base::setMaterial");
					return status;
				}

				/*
				 * Connect the base to the previous one
				 */

				if (extendForward) {
					if (!(status = previous_base.connect_forward(base, true))) {
						status.perror("Base::connect_forward 1");
						return status;
					}
				}
				else {
					if (!(status = base.connect_forward(previous_base, true))) {
						status.perror("Base::connect_forward 2");
						return status;
					}
				}

				if (hasOppositeStrand) {
					/*
					 * Find the opposite base if exists (using previous_opposite) and connect to it, use the isDestinationForOpposite to decide on the direction
					 */

					std::cerr << "Connecting to opposite" << std::endl;

					Model::Base opposite = (previous_opposite.*target_backward) (status);

					if (!status && status != MStatus::kNotFound) {
						status.perror("Base::*target_backward");
						return status;
					}

					if (status) {
						/*
						 * The opposite base exists, connect to it
						 */

						Model::Helix opposite_base_helix = opposite.getParent(status);

						if (!status) {
							status.perror("Base::getParent");
							return status;
						}

						if (helix == opposite_base_helix) {
							if (isDestinationForOpposite) {
								if (!(status = opposite.connect_opposite(base, true))) {
									status.perror("Base::connect_opposite 1");
									return status;
								}
							}
							else {
								if (!(status = base.connect_opposite(opposite, true))) {
									status.perror("Base::connect_opposite 2");
									return status;
								}
							}
						}
						else
							hasOppositeStrand = false;
					}
					else {
						/*
						 * There is no opposite base anymore, to make sure further iterations in the loop don't try again, set hasOppositeStrand = false
						 */

						hasOppositeStrand = false;
					}
					
					previous_opposite = opposite;
				}
				else {

					/*
					 * Since we're extending along a direction in which there does not seem to be any bases further away,
					 *	we are extending the total length of the helix. It is important to track these changes as it requires us to
					 *	rescale the cylinder
					 */

					if (extendPositiveZ)
						++positive_count;
					else
						++negative_count;
				}

				previous_base = base;

				created_bases.push_back(base);

				onProgressStep();
			}

			double previous_origo, previous_height;

			if (!(status = helix.getCylinderRange(previous_origo, previous_height))) {
				status.perror("Helix::getCylinderRange");
				return status;
			}

			double new_height = previous_height + (positive_count + negative_count) * DNA::STEP;
			double new_origo = previous_origo + (new_height - previous_height) / 2.0 - negative_count * DNA::STEP;

			if (!(status = helix.setCylinderRange(new_origo, new_height)))
				status.perror("Helix::setCylinderRange");

			saveUndoRedo(element, created_bases);
			m_modified_helices.push_front(std::make_pair(helix, Range(previous_origo, previous_height)));

			onProgressDone();

			return MStatus::kSuccess;
		}

		MStatus ExtendStrand::doUndo(Model::Base & element, std::vector<Model::Base> & undoData) {
			MStatus status;

			/*
			 * Erase all the bases
			 */

			std::for_each(undoData.begin(), undoData.end(), std::mem_fun_ref(&Model::Base::deleteNode));

			/*
			 * Find the helix this base belongs to
			 */

			Model::Helix helix = element.getParent(status);

			if (!status) {
				status.perror("Base::getParent");
				return status;
			}

			/*
			 * Setup cylinders
			 */

			for(std::list< std::pair<Model::Helix, Range> >::iterator it = m_modified_helices.begin(); it != m_modified_helices.end(); ++it) {
				if (!(status = it->first.setCylinderRange(it->second.origo, it->second.height))) {
					status.perror("Helix::setCylinderRange");
					return status;
				}
			}

			/*
			 * Because redo will add to them again
			 */

			m_modified_helices.clear();

			return MStatus::kSuccess;
		}

		MStatus ExtendStrand::doRedo(Model::Base & element, Empty & redoData) {
			return doExecute(element);
		}

		void ExtendStrand::onProgressBegin(int range) {

		}

		void ExtendStrand::onProgressStep() {

		}

		void ExtendStrand::onProgressDone() {

		}
	}
}