#include <controller/Duplicate.h>

#include <model/Helix.h>

#include <vector>
#include <algorithm>
#include <list>
#include <functional>

#include <maya/MFnTransform.h>

#if defined(WIN32) || defined(WIN64)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif /* N Windows */

namespace Helix {
	namespace Controller {
		MStatus Duplicate::duplicate(const MObjectArray & helices) {
			if (helices.length() == 0)
				return MStatus::kSuccess;

			m_helices.setLength(helices.length());
			std::copy(&helices[0], &helices[0] + helices.length(), &m_helices[0]);
			
			return redo();
		}

		MStatus Duplicate::undo() {
			/*
			 * Just erase all newly created helices
			 */

			std::for_each(m_new_helices.begin(), m_new_helices.end(), std::mem_fun_ref(&Model::Helix::deleteNode));

			return MStatus::kSuccess;
		}

		MStatus Duplicate::redo() {
			MStatus status;
			
#if 1 /*MAC_PLUGIN*/
			// TODO: Clean this up and replace with some std::map instead that doesn't require a hash. We can't hash MObjects.
			/*
			 * The implementation of std::tr1::unordered_map is broken in GCC 4.2, which is the version we're using under Mac OS X
			 */

			class : public std::list< std::pair<Model::Base, Model::Base> > {
			public:
				Model::Base operator[](Model::Base & model) {
					for(std::list< std::pair<Model::Base, Model::Base> >::iterator it = begin(); it != end(); ++it) {
						if (it->first == model)
							return it->second;
					}

					return Model::Base();
				}
			} base_translation;
#else
			std::tr1::unordered_map<Model::Base, Model::Base> base_translation;
#endif /* N MAC_PLUGIN */
			
			std::cerr << "Got " << m_helices.length() << " helices to copy" << std::endl;

			/*
			 * Count the total number of bases we're going to create. This is just for the progress bar
			 *
			 */

			unsigned int totalNumBases = 0;

			for(unsigned int i = 0; i < m_helices.length(); ++i) {
				Model::Helix helix(m_helices[i]);

				for(Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it)
					++totalNumBases;
			}

			onProgressStart(0, totalNumBases);

			/*
			 * First step: Create all the helices by copying rotation, translation and cylinder data from the old ones
			 * Create the bases but don't make *any* connections at all
			 */

			m_new_helices.clear();
			m_new_helices.reserve(m_helices.length());

			for(unsigned int i = 0; i < m_helices.length(); ++i) {
				/*
				 * Create helix
				 */

				Model::Helix helix(m_helices[i]);

				MDagPath helix_dagPath = helix.getDagPath(status);

				if (!status) {
					status.perror("Helix::getDagPath");
					return status;
				}

				MString helix_name = MFnDagNode(helix_dagPath).name(&status) + "_copy";
				Model::Helix new_helix;

				if (!status) {
					status.perror("MFnDagNode::name");
					return status;
				}

				MTransformationMatrix transformation_matrix;

				if (!(status = helix.getTransform(transformation_matrix))) {
					status.perror("Helix::getTransform");
					return status;
				}

				MMatrix matrix = transformation_matrix.asMatrix();
				
				if (!(status = Model::Helix::Create(helix_name, matrix, new_helix))) {
					status.perror("Helix::Create");
					return status;
				}

				std::cerr << "Done creating helix" << std::endl;

				/*
				 * Setup the cylinder
				 */

				double cylinder_origo, cylinder_height;

				if (!(status = helix.getCylinderRange(cylinder_origo, cylinder_height))) {
					status.perror("Helix::getCylinderRange");
					return status;
				}

				if (!(status = new_helix.setCylinderRange(cylinder_origo, cylinder_height))) {
					status.perror("Helix::setCylinderRange");
					return status;
				}

				std::cerr << "Done setting up cylinder" << std::endl;

				/*
				 * Create bases
				 */

				for(Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it) {
					Model::Base base(*it);
					MVector translation;
					MDagPath base_dagPath = base.getDagPath(status);

					if (!status) {
						status.perror("Base::getDagPath");
						return status;
					}

					MString base_name = MFnDagNode(base_dagPath).name();

					if (!(status = base.getTranslation(translation, MSpace::kTransform))) {
						status.perror("Base::getTranslation");
						return status;
					}

					Model::Base new_base;
					
					if (!(status = Model::Base::Create(new_helix, base_name, translation, new_base))) {
						status.perror("Base::Create");
						return status;
					}

					std::cerr << "Done creating the base" << std::endl;

					/*
					 * Color the new base using the same material as the old one
					 */

					Model::Material material;

					if (!(status = base.getMaterial(material)))
						status.perror("Base::getMaterial");
					else {
						if (!(status = new_base.setMaterial(material)))
							status.perror("Base::setMaterial");
					}

					std::cerr << "Done setting the base material" << std::endl;

					/*
					 * Add the newly created base paired with the old one in our lookup table for later referencing
					 */

					base_translation[base] = new_base;

					onProgressStep();
				}

				m_new_helices.push_back(new_helix);
			}

			std::cerr << "Done step 1" << std::endl;

			/*
			 * Yet again, loop over all helices and all bases, find out what other bases the current base is connected to
			 * (forward and opposite) then translate them all using the m_base_translation table and create similar connections
			 */

			onProgressDone();
			onProgressStart(1, totalNumBases);

			for(unsigned int i = 0; i < m_helices.length(); ++i) {
				Model::Helix helix(m_helices[i]);

				for(Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it) {
					Model::Base base(*it);
					Model::Base new_base = base_translation[base];

					std::cerr << "base: " << base.getDagPath(status).fullPathName().asChar() << ", new_base: " << new_base.getDagPath(status).fullPathName().asChar() << std::endl;

					Model::Base forward_base = base.forward(status);

					if (!status && status != MStatus::kNotFound) {
						status.perror("Base::forward");
						return status;
					}

					if (status) {
						/*
						 * This base has a forward connection
						 */

						Model::Base new_forward_base = base_translation[forward_base];

						if (new_forward_base) {

							std::cerr << "Connect forward" << std::endl;

							if (!(status = new_base.connect_forward(new_forward_base))) {
								status.perror("Base::connect_forward");
								return status;
							}
						}
					}

					std::cerr << "Looking for opposite connection" << std::endl;

					Model::Base opposite_base = base.opposite(status);

					if (!status && status != MStatus::kNotFound) {
						status.perror("Base::opposite");
						return status;
					}

					if (status) {
						/*
						 * This base has an opposite connection
						 */

						std::cerr << "Got opposite connection" << std::endl;

						bool isDestination = base.opposite_isDestination(status);

						if (!status) {
							status.perror("Base::opposite_isDestination");
							return status;
						}

						std::cerr << "isDestination: " << (isDestination ? "true" : "false") << std::endl;

						Model::Base new_opposite_base = base_translation[opposite_base];

						if (new_opposite_base) {
							/*
							 * The order of the setup is important
							 */

							std::cerr << "Got new_opposite_base" << std::endl;

							std::cerr << "new_base: " << new_base.getDagPath(status).fullPathName().asChar() << ", new_opposite_base: " << new_opposite_base.getDagPath(status).fullPathName().asChar() << std::endl;

							if (isDestination) {
								if (!(status = new_opposite_base.connect_opposite(new_base))) {
									status.perror("Base::connect_opposite");
									return status;
								}

								/*
								 * Set the label of the base
								 */

								DNA::Name label;

								if (!(status = opposite_base.getLabel(label))) {
									status.perror("Base::getLabel");
									return status;
								}

								if (!(status = new_opposite_base.setLabel(label))) {
									status.perror("Base::setLabel");
									return status;
								}
							}
							else {
								if (!(status = new_base.connect_opposite(new_opposite_base))) {
									status.perror("Base::connect_opposite");
									return status;
								}

								/*
								 * Set the label of the base
								 */

								DNA::Name label;

								if (!(status = opposite_base.getLabel(label))) {
									status.perror("Base::getLabel");
									return status;
								}

								if (!(status = new_opposite_base.setLabel(label))) {
									status.perror("Base::setLabel");
									return status;
								}
							}
						}
					}

					onProgressStep();
				}
			}

			onProgressDone();

			/*
			 * Refresh cylinder/base view
			 */

			if (!(status = Model::Helix::RefreshCylinderOrBases()))
				status.perror("Helix::RefreshCylinderOrBases");

			/*
			 * Select all the newly created helices
			 */

			if (!(status = Model::Object::Select(m_new_helices.begin(), m_new_helices.end()))) {
				status.perror("Object::Select");
				return status;
			}

			return MStatus::kSuccess;
		}

		void Duplicate::onProgressStart(unsigned int process, unsigned int range) {

		}

		void Duplicate::onProgressStep() {

		}

		void Duplicate::onProgressDone() {

		}
	}
}
