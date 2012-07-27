#include <controller/CreateCurves.h>
#include <Utility.h>

#include <maya/MGlobal.h>
#include <maya/MPointArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MDGModifier.h>
#include <maya/MQuaternion.h>


#include <algorithm>

namespace Helix {
	namespace Controller {
		MStatus CreateCurves::undo() {
			MStatus status;
			
			for(unsigned int i = 0; i < m_curves.length(); ++i) {
				std::cerr << "DeleteNode: " << MFnDagNode(m_curves[i]).fullPathName().asChar() << std::endl;

				if (!(status = MGlobal::removeFromModel(m_curves[i]))) {
					status.perror("MGlobal::removeFromModel");
					return status;
				}
			}

			m_curves.clear();

			return MStatus::kSuccess;
		}

		MStatus CreateCurves::redo() {
			MStatus status;

			for(std::list<Curve>::iterator it = m_curve_data.begin(); it != m_curve_data.end(); ++it) {
				if (!(status = it->create(*this))) {
					status.perror("Curve::create");
					return status;
				}
			}

			return status;
		}

		void CreateCurves::onProgressStep() {

		}

		MStatus CreateCurves::createCurves(std::list<Model::Helix> & helices, std::list<Model::Strand> & strands) {
			MStatus status;

			/*
			 * As some curves are circular, we must check every base and what strand it belongs to
			 */

			std::list<Model::Strand> added_strands;

			for(std::list<Model::Helix>::iterator it = helices.begin(); it != helices.end(); ++it) {
				for(Model::Helix::BaseIterator bit = it->begin(); bit != it->end(); ++bit) {
					Model::Strand strand(*bit);

					if (find_nonconst(added_strands.begin(), added_strands.end(), strand) == added_strands.end()) {
						if (!(status = createCurve(strand))) {
							status.perror("createCurve");
							return status;
						}

						added_strands.push_back(strand);

						onProgressStep();
					}
				}
			}

			for(std::list<Model::Strand>::iterator it = strands.begin(); it != strands.end(); ++it) {
				if (find_nonconst(added_strands.begin(), added_strands.end(), *it) == strands.end()) {
					if (!(status = createCurve(*it))) {
						status.perror("createCurve");
						return status;
					}
				}
			}

			/*
			 * Select the created curves
			 */

			MSelectionList selectionList;

			//std::for_each(&m_curves[0], &m_curves[0] + m_curves.length(), SelectionListAddFunctor(selectionList));
			for(unsigned int i = 0; i < m_curves.length(); ++i)
				selectionList.add(m_curves[i]);

			if (!(status = MGlobal::setActiveSelectionList(selectionList))) {
				status.perror("MGlobal::setActiveSelectionList");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus CreateCurves::createCurve(Model::Strand & strand) {
			MStatus status;

			std::cerr << "Working with strand defined by base: " << strand.getDefiningBase().getDagPath(status).fullPathName().asChar() << std::endl;

			/*
			 * Make sure this base is an end base (5')
			 */

			Model::Strand::BackwardIterator base_it = std::find_if(strand.reverse_begin(), strand.reverse_end(), Model::Strand::BaseTypeFunc(Model::Base::FIVE_PRIME_END));

			/*
			 * If we found an end base, use it, if not, it's a loop and it wont matter but we have to close the curve
			 */

			bool isLoop = base_it == strand.reverse_end();
			Model::Strand strand_(isLoop ? strand.getDefiningBase() : *base_it);

			std::cerr << "5' end or loop base: " << strand_.getDefiningBase().getDagPath(status).fullPathName().asChar() << std::endl;

			MPointArray pointArray;
			MDoubleArray knotSequences;
			
			/*
			 * Notice that the first point is different, it is as if it was inserted 4 times
			 * Also notice that radius is probably normally never changed, but is taken into account
			 * radius is only measured on the X,Y plane
			 */
			MVector translation;

			for(Model::Strand::ForwardIterator it = strand_.forward_begin(); it != strand_.forward_end(); ++it) {
				/*
				 * Extract our coordinates, then interpolate between the last coordinates and these m_degree times
				 */

				if (!(status = it->getTranslation(translation, MSpace::kWorld))) {
					status.perror("Base::getTranslation");
					return status;
				}

				pointArray.append(translation);
			}

			/*
			 * Now create the CV curve
			 */

			knotSequences.setSizeIncrement(pointArray.length() + 2 * (unsigned int) m_degree - 1);
			for(int i = 0; i < m_degree - 1; ++i)
				knotSequences.append(0);
			for(unsigned int i = 0; i < pointArray.length() - 2; ++i)
				knotSequences.append(i);
			for(int i = 0; i < m_degree - 1; ++i)
				knotSequences.append(pointArray.length() - 3);

			m_curve_data.push_back(Curve(pointArray, knotSequences, isLoop));
			
			if (!(status = m_curve_data.back().create(*this))) {
				status.perror("Curve::create");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus CreateCurves::Curve::create(CreateCurves & instance) {
			MStatus status;

			MFnNurbsCurve curve;

			MObject curveObject = curve.create(points, knots, instance.m_degree, isLoop ? MFnNurbsCurve::kClosed : MFnNurbsCurve::kOpen, false, false, MObject::kNullObj, &status);

			if (!status) {
				status.perror("MFnNurbsCurve::create");
				return status;
			}

			MDagPath path;
			if (!(status = curve.getPath(path))) {
				status.perror("MFnNurbsCurve::getPath");
				return status;
			}

			instance.m_curves.append(path.transform());

			return MStatus::kSuccess;
		}
	}
}
