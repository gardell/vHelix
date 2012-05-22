/*
 * Helix.h
 *
 *  Created on: 9 feb 2012
 *      Author: johan
 */

#ifndef _MODEL_HELIX_H_
#define _MODEL_HELIX_H_

#include <model/Object.h>
#include <model/Base.h>

#include <maya/MString.h>
#include <maya/MMatrix.h>

/*
 * This is the interface used for modifying helices
 */

namespace Helix {
	namespace Model {
		class Helix : public Object {
		public:
			/*
			 * Create: Creates a new helix node, attach a locator node and transforms it to the given position
			 */

			static MStatus Create(const MString & name, MMatrix & transform, Helix & helix);

			/*
			 * Select: Give any object that either identifies a helix or it's children and get a handle to the helix manipulator
			 */

			static MStatus Selected(Object & object, Helix & helix);

			/*
			 * Fills the array of all currently selected helices in the scene. Notice that it uses the above method, so indirect selections will also be returned
			 * No duplicates will be generated
			 */

			static MStatus AllSelected(MObjectArray & selectedHelices);

			DEFINE_DEFAULT_INHERITED_OBJECT_CONSTRUCTORS(Helix)

			/*
			 * Manage the cylinder. Default is along the Z-axis but could change in the future
			 */

			MStatus setCylinderRange(double origo, double height);
			MStatus getCylinderRange(double & origo, double & height);

			/*
			 * BaseIterator: Iterate over the bases belonging to this helix
			 */

			class BaseIterator {
				friend class Helix;
			public:

				inline BaseIterator operator++() {
					BaseIterator bi(*this);

					GetNextBaseIndex();

					return bi;
				}

				inline BaseIterator & operator++(int x) {
					GetNextBaseIndex();

					return *this;
				}

				inline Base * operator->() {
					m_cacheBase = getChildBase();
					return &m_cacheBase;
				}

				inline Base & operator*() {
					m_cacheBase = getChildBase();
					return m_cacheBase;
				}

				inline bool operator== (const BaseIterator & i) const {
					return m_helix == i.m_helix && m_childIndex == i.m_childIndex;
				}

				inline bool operator!= (const BaseIterator & i) const {
					return !operator == (i);
				}

			protected:
				inline BaseIterator(Helix & helix, unsigned int index) : m_helix(&helix), m_childIndex(index) {

				}

				inline BaseIterator(const BaseIterator & copy) : m_helix(copy.m_helix), m_childIndex(copy.m_childIndex) {

				}

				Base getChildBase();
				void GetNextBaseIndex();

			private:
				Helix *m_helix;
				Base m_cacheBase;
				unsigned int m_childIndex;
			};

			BaseIterator begin();
			BaseIterator end();

		private:
			bool hasCylinder(MStatus & status);
			MStatus createCylinder(double origo, double top);
		};
	}
}

#endif /* _MODEL_HELIX_H_ */
