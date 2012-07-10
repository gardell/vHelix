#include <view/BaseShape.h>
#include <maya/MPoint.h>

namespace Helix {
	namespace View {
		BaseShape::BaseShape() {

		}

		BaseShape::~BaseShape() {

		}

		void BaseShape::postConstructor() {
			setRenderable( true );
		}

		bool BaseShape::isBounded() const {
			return true;
		}

		MBoundingBox BaseShape::boundingBox() const {
			const MPoint min(-0.264 / 2.0, -0.264 / 2.0, -0.719 / 2.0), max(0.264 / 2.0, 0.264 / 2.0, 0.719 / 2.0);

			return MBoundingBox(min, max);
		}

		void *BaseShape::creator() {
			return new BaseShape();
		}

		MStatus BaseShape::initialize() {
			return MStatus::kSuccess;
		}

		MTypeId BaseShape::id(BASE_SHAPE_ID);
	}
}
