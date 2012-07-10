#ifndef _VIEW_HELIX_GEOMETRY_ITERATOR_H_
#define _VIEW_HELIX_GEOMETRY_ITERATOR_H_

#include <Definition.h>

#include <iostream>

#include <maya/MPxGeometryIterator.h>
#include <maya/MFnDagNode.h>

/*
 * The HelixGeometryIterator iterates over the children of a helix giving access to the translation of all the HelixBases.
 * Using this class, Maya can access the bases via the HelixShape thus enabling the user to manipulate the shape as if he/she was manipulating the HelixBases directly
 * Note that userGeometry is castable to a HelixShape in order to query it for the thisMObject that in turn gives access to the Helix
 */

namespace Helix {
	namespace View {
		class HelixGeometryIterator : public MPxGeometryIterator {
		public:
			HelixGeometryIterator( void * userGeometry, MObjectArray & components );
			HelixGeometryIterator( void * userGeometry, MObject & components );

			virtual bool 	 isDone () const;
			virtual void 	 next ();
			virtual void 	 reset ();

			virtual MPoint 	 point () const;
			virtual void 	 setPoint (const MPoint &) const;

		private:
			MFnDagNode m_dagNode;
			unsigned int m_index;
		};
	}
}

#endif /* N _VIEW_HELIX_GEOMETRY_ITERATOR_H_ */