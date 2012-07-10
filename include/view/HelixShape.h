#ifndef _HELIXSHAPE_H_
#define _HELIXSHAPE_H_

#include <Definition.h>

#include <iostream>

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxGeometryIterator.h>

/*
 * HelixShape: Manages the rendering information of a helix (Bases etc).
 * Doesn't really do anything as it mostly passes away to HelixShapeUI and HelixGeometryIterator
 */

#define HELIX_SHAPE_NAME "helixShape"

namespace Helix {
	namespace View {
		class HelixShape : public MPxSurfaceShape {
		public:
			HelixShape();
			virtual ~HelixShape();
			virtual void postConstructor();

			virtual bool isBounded() const;
			virtual MBoundingBox boundingBox() const; 

			static  void *creator();
			static  MStatus initialize();

			static  MTypeId id;

			/*
			 * The cylinder rendering attributes
			 */

			static MObject aOrigo, aHeight;
		};
	}
}
#endif /* N _HELIXSHAPE_H_ */
