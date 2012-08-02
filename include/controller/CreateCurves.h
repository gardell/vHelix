#ifndef _CONTROLLER_CREATECURVES_H_
#define _CONTROLLER_CREATECURVES_H_

#include <Definition.h>

#include <model/Helix.h>
#include <model/Base.h>
#include <model/Strand.h>

#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>

#include <list>
#include <iterator>

namespace Helix {
	namespace Controller {

		class VHELIXAPI CreateCurves {
		public:
			/*
			 * Creates strands either from the strands defined by the bases or by the ones defined by the helices
			 * Degree is the number of vertices created per base, normally for CV curves the degree is 3.
			 * in that case we will interpolate the vertices along the helix as if they we're placed on a cylinder.
			 * if degree is 1, the curve is linear between every base
			 */

			inline MStatus createCurves(const MObjectArray & helices, const MObjectArray & bases, int degree) {
				std::list<Model::Helix> h;
				std::list<Model::Strand> strands;

				std::copy(&helices[0], &helices[0] + helices.length(), std::back_insert_iterator< std::list<Model::Helix> > (h));
				std::copy(&bases[0], &bases[0] + bases.length(), std::back_insert_iterator< std::list<Model::Strand> > (strands));
				m_degree = degree;

				return createCurves(h, strands);
			}

			MStatus redo();
			MStatus undo();

		protected:
			/*
			 * Callbacks for progress bars etc. called between strands, total number will be helices * 2 + bases;
			 */

			virtual void onProgressStep();

		private:
			//std::list<Model::Helix> m_helices;
			//std::list<Model::Strand> m_strands;
			struct Curve {
				MPointArray points;
				MDoubleArray knots;
				bool isLoop;

				inline Curve(const MPointArray & points_, const MDoubleArray & knots_, bool isLoop_) : points(points_), knots(knots_), isLoop(isLoop_) { }

				MStatus create(CreateCurves & instance);
			};

			std::list<Curve> m_curve_data;

			MObjectArray m_curves;
			int m_degree;

			MStatus createCurves(std::list<Model::Helix> & helices, std::list<Model::Strand> & strands);
			MStatus createCurve(Model::Strand & strand);
		};
	}
}

#endif /* N _CONTROLLER_CREATECURVES_H_ */
