#include <view/HelixShape.h>
#include <model/Helix.h>

#include <maya/MFnDagNode.h>
#include <maya/MFnNumericAttribute.h>

#define HELIXSHAPE_MARKING_MENU_COMMAND												\
"global proc " HELIX_SHAPE_NAME "DagMenuProc( string $parent, string $child ) {\n"	\
"	setParent -m $parent;\n"														\
"	string $cmd = \"duplicate -target \" + $child;\n"								\
"	menuItem -l \"Duplicate\" -rp \"N\" -c $cmd;\n"									\
"	$cmd = \"findFivePrimeEnds\";\n"												\
"	menuItem -l \"Find all 5' ends\" -rp \"S\" -c $cmd;\n"							\
"}"

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

			/*
			 * Setup the marking menus for HelixShape using MEL
			 */

			if (!(status = MGlobal::executeCommandOnIdle(HELIXSHAPE_MARKING_MENU_COMMAND)))
				status.perror("No marking menus for HelixShape will be available: ");

			return MStatus::kSuccess;
		}

		MTypeId HelixShape::id(0x80111);
	}
}