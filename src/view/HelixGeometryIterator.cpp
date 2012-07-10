#include <view/HelixGeometryIterator.h>
#include <view/HelixShape.h>
#include <HelixBase.h>

#include <maya/MFnTransform.h>

namespace Helix {
	namespace View {
		HelixGeometryIterator::HelixGeometryIterator(void * userGeometry, MObjectArray & components ) : MPxGeometryIterator(userGeometry, components), m_dagNode(MFnDagNode(static_cast<HelixShape *> (userGeometry)->thisMObject()).parent(0)), m_index(0) {
	

		}

		HelixGeometryIterator::HelixGeometryIterator( void * userGeometry, MObject & components ) : MPxGeometryIterator(userGeometry, components), m_dagNode(MFnDagNode(static_cast<HelixShape *> (userGeometry)->thisMObject()).parent(0)), m_index(0) {
	
		}

		bool HelixGeometryIterator::isDone () const {
			return m_index >= m_dagNode.childCount();
		}

		void HelixGeometryIterator::next () {
			do {
				++m_index;
			}
			while (MFnDagNode(m_dagNode.child(m_index)).typeId() != HelixBase::id);
		}

		void 	 HelixGeometryIterator::reset () {
			m_index = 0;
		}

		MPoint HelixGeometryIterator::point () const {
			return MPoint(MFnTransform(m_dagNode.child(m_index)).translation(MSpace::kTransform));
		}

		void HelixGeometryIterator::setPoint (const MPoint & point) const {
			MFnTransform(m_dagNode.child(m_index)).setTranslation(point, MSpace::kTransform);
		}
	}
}