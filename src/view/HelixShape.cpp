#include <view/HelixShape.h>
#include <view/HelixGeometryIterator.h>
#include <model/Helix.h>

#include <maya/MFnDagNode.h>
#include <maya/MFnNumericAttribute.h>

namespace Helix {
	namespace View {
		MObject HelixShape::aOrigo, HelixShape::aHeight;

		HelixShape::HelixShape() {

		}

		HelixShape::~HelixShape() {

		}

		void HelixShape::postConstructor() {
			setRenderable( true );
		}

		bool HelixShape::isBounded() const {
			return true;
		}

		MBoundingBox HelixShape::boundingBox() const {
			MStatus status;

			/*
			 * To calculate a bounding box, iterate over all base children and set the boundaries
			 */

			MPlug heightPlug(thisMObject(), aHeight), origoPlug(thisMObject(), aOrigo);

			float origo = origoPlug.asFloat(), height = heightPlug.asFloat();

			MPoint min(-DNA::RADIUS, -DNA::RADIUS, origo - height / 2), max(DNA::RADIUS, DNA::RADIUS, origo + height / 2);

			return MBoundingBox(min, max);
		}

		void *HelixShape::creator() {
			return new HelixShape();
		}

		MStatus HelixShape::initialize() {
			MStatus status;

			MFnNumericAttribute origoAttr, heightAttr;

			aOrigo = origoAttr.create("origo", "o", MFnNumericData::kFloat, 0.0, &status);

			if (!status) {
				status.perror("MFnNumericAttribute::create 1");
				return status;
			}

			aHeight = heightAttr.create("height", "h", MFnNumericData::kFloat, 0.0, &status);

			if (!status) {
				status.perror("MFnNumericAttribute::create 2");
				return status;
			}

			addAttribute(aOrigo);
			addAttribute(aHeight);

			return MStatus::kSuccess;
		}

		MTypeId HelixShape::id(0x80111);
	}
}