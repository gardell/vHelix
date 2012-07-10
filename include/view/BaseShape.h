#ifndef _VIEW_BASESHAPE_H_
#define _VIEW_BASESHAPE_H_

#include <Definition.h>

#include <maya/MPxSurfaceShape.h>

#define BASE_SHAPE_ID 0x02114115
#define BASE_SHAPE_NAME "helixBaseShape"

namespace Helix {
	namespace View {
		class BaseShape : public MPxSurfaceShape {
		public:
			BaseShape();
			virtual ~BaseShape();
			virtual void postConstructor();

			virtual bool isBounded() const;
			virtual MBoundingBox boundingBox() const; 

			static  void *creator();
			static  MStatus initialize();

			static  MTypeId id;
		};
	}
}

#endif /* N _VIEW_BASESHAPE_H_ */