#ifndef _CONTROLLER_EXTENDSTRAND_H_
#define _CONTROLLER_EXTENDSTRAND_H_

#include <controller/Operation.h>
#include <model/Helix.h>
#include <model/Base.h>

#include <vector>

namespace Helix {
	namespace Controller {
		/*
		 * ExtendStrand: Extend a number of selected bases in their direction where they have no connection.
		 * - if two bases that are paired are both selected for extension their new extensions must also be paired
		 * - if a base that is paired is selected for extension, and its opposite base already have an extension
		 *    the following bases must be paired along with the extension along the opposite base
		 */

		class VHELIXAPI ExtendStrand : public Operation<Model::Base, std::vector<Model::Base> > {
		public:

			/*
			 * The number of bases to create
			 */

			void setLength(unsigned int length) {
				m_length = length;
				/*m_positive_count = 0;
				m_negative_count = 0;*/
			}

			/*
			 * After a successful run, call to make sure the cylinders are set up properly
			 * Notice that redo is the same as doCylinder
			 */

			MStatus doCylinder();
			MStatus undoCylinder();

		protected:
			MStatus doExecute(Model::Base & element);
			MStatus doUndo(Model::Base & element, std::vector<Model::Base> & undoData);
			MStatus doRedo(Model::Base & element, Empty & redoData);

			virtual void onProgressBegin(int range);
			virtual void onProgressStep();
			virtual void onProgressDone();

			unsigned int m_length;

			/*
			 * Whenever the total length of the structure changes we add them here, positive and negative are along the Z axis
			 */

			//unsigned int m_positive_count, m_negative_count;
			//double m_previous_origo, m_previous_height;

			struct Range {
				inline Range(double _origo, double _height) : origo(_origo), height(_height) { }

				double origo, height;
			};

			std::list< std::pair<Model::Helix, Range> > m_modified_helices;
		};
	}
}

#endif /* N _CONTROLLER_EXTENDSTRAND_H_ */