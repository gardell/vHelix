/*
 * OxDnaExporterController.cpp
 *
 *  Created on: Jan 28, 2014
 *      Author: johan
 */

#include <controller/OxDnaExporter.h>
#include <model/Helix.h>
#include <Utility.h>

#include <fstream>

namespace Helix {
	namespace Controller {
		MStatus OxDnaExporter::doExecute(Model::Strand & element) {
			Strand outstrand;
			MStatus status;

			// Iterate backwards along the strand to find the first base of the strand if not circular.
			element.rewind();
			Model::Base & base(element.getDefiningBase());
			MDagPath baseDagPath;
			HMEVALUATE(baseDagPath = base.getDagPath(status), status);
			HMEVALUATE(outstrand.name = baseDagPath.fullPathName(&status), status);

			Model::Helix parent(base.getParent(status));
			MEulerRotation parentRotation;
			HMEVALUATE_RETURN(status = parent.getRotation(parentRotation), status);
			MVector parentTranslation;
			parent.getTranslation(parentTranslation, MSpace::kWorld);
			const MVector normal((MVector(0, 0, 1) * parentRotation.asMatrix()).normal());

			Model::Strand::ForwardIterator it = element.forward_begin();
			for(; it != element.forward_end(); ++it) {
				Model::Helix parent(it->getParent(status));

				Base base;
				HMEVALUATE_RETURN(status = it->getLabel(base.label), status);

				HMEVALUATE_RETURN(status = it->getTranslation(base.translation, MSpace::kWorld), status);

				MQuaternion rotation;
				HMEVALUATE_RETURN(status = it->getRotation(rotation), status);

				if (base.label == DNA::Invalid) {
					MGlobal::displayError(MString("The base ") + it->getDagPath(status).fullPathName() + " does not have an assigned label.");
					return MStatus::kInvalidParameter;
				}

				base.tangent = (normal ^ ((base.translation - parentTranslation) ^ normal)).normal();

				int direction;
				HMEVALUATE_RETURN(direction = it->sign_along_axis(MVector::zAxis, MSpace::kTransform, status), status);

				base.normal = normal * direction;
				outstrand.strand.push_back(base);
				HMEVALUATE_RETURN(base.helixName = parent.getDagPath(status).fullPathName(), status);
				HMEVALUATE_RETURN(base.name = it->getDagPath(status).fullPathName(), status);

				if (m_helices.find(base.helixName.asChar()) == m_helices.end()) {
					Helix helix;

					HMEVALUATE_RETURN(status = parent.getTranslation(helix.translation, MSpace::kWorld), status);

					MEulerRotation rotation;
					HMEVALUATE_RETURN(status = parent.getRotation(rotation), status);
					helix.normal = MVector::zAxis * MTransformationMatrix().rotateTo(rotation).asMatrix();

					m_helices.insert(std::make_pair(base.helixName.asChar(), helix));
				}
			}

			outstrand.circular = it.loop();
			m_strands.push_back(outstrand);
			return MStatus::kSuccess;
		}

		MStatus OxDnaExporter::doUndo(Model::Strand & element, Empty & undoData) {
			return MStatus::kSuccess;
		}

		MStatus OxDnaExporter::doRedo(Model::Strand & element, Empty & redoData) {
			return MStatus::kSuccess;
		}

		MStatus OxDnaExporter::write(const char *topology_filename, const char *configuration_filename, const char *vhelix_filename, const MVector & minTranslation, const MVector & maxTranslation) const {

			std::ofstream conf_file(configuration_filename);
			std::ofstream top_file(topology_filename);
			std::ofstream vhelix_file(vhelix_filename);

			if (!conf_file) {
				MGlobal::displayError(MString("Can't open file \"") + configuration_filename + "\" for writing.");
				return MStatus::kFailure;
			}

			if (!top_file) {
				MGlobal::displayError(MString("Can't open file \"") + topology_filename + "\" for writing.");
				return MStatus::kFailure;
			}

			if (!vhelix_file) {
				MGlobal::displayError(MString("Can't open file \"") + vhelix_filename + "\" for writing.");
				return MStatus::kFailure;
			}

			unsigned int numBases = 0;
			for (std::list<Strand>::const_iterator it = m_strands.begin(); it != m_strands.end(); ++it) {
				numBases += it->strand.size();
			}

			top_file << m_strands.size() << " " << numBases << std::endl;

			unsigned int i = 1;
			for (std::list<Strand>::const_iterator it = m_strands.begin(); it != m_strands.end(); ++it, ++i) {
				const int firstIndex = it->circular ? it->strand.size() - 1 : -1;
				const int lastIndex = it->circular ? 0 : -1;

				top_file << "# " << it->name.asChar() << std::endl;

				int j = 0;
				for (std::vector<Base>::const_iterator lit = it->strand.begin(); lit != it->strand.end(); ++lit, ++j)
					top_file << i << " " << lit->label.toChar() << " " <<
								(j == 0 ? firstIndex : j - 1) << " " <<
								(j == int(it->strand.size()) - 1 ? lastIndex : j + 1) << std::endl;

				top_file << std::endl;
			}

			top_file.close();

			const MVector dimensions(maxTranslation - minTranslation);
			conf_file << "t = 0" << std::endl << "b = " << dimensions.x << " " << dimensions.y << " " << dimensions.z << std::endl << "E = 0. 0. 0." << std::endl;

			for (std::list<Strand>::const_iterator it = m_strands.begin(); it != m_strands.end(); ++it, ++i) {
				conf_file << "# " << it->name.asChar() << std::endl;

				for (std::vector<Base>::const_iterator sit = it->strand.begin(); sit != it->strand.end(); ++sit) {
					const MVector translation(sit->translation - minTranslation);

					conf_file << translation.x << " " << translation.y << " " << translation.z << " " <<
							sit->tangent.x << " " << sit->tangent.y << " " << sit->tangent.z << " " <<
							sit->normal.x << " " << sit->normal.y << " " << sit->normal.z <<
							" 0.0 0.0 0.0 0.0 0.0 0.0" << std::endl;
				}

				conf_file << std::endl;
			}

			conf_file.close();

			// Export helices...

			// Export bases...

			vhelix_file.close();

			return MStatus::kSuccess;
		}
	}
}
