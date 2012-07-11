#include <view/BaseShape.h>
#include <maya/MPoint.h>
#include <maya/MGlobal.h>

#define BASESHAPE_MARKING_MENU_COMMAND												\
"global proc " BASE_SHAPE_NAME "DagMenuProc( string $parent, string $child ) {\n"	\
"	setParent -m $parent;\n"														\
"	string $cmd = \"connectBases -second \" + $child;\n"							\
"	menuItem -l \"Connect to\" -rp \"N\" -c $cmd;\n"								\
"	$cmd = \"disconnectBase -target \" + $child;\n"									\
"	menuItem -l \"Disconnect\" -rp \"S\" -c $cmd;\n"								\
"	$cmd = \"paintStrand -target \" + $child;\n"									\
"	menuItem -l \"Paint strand\" -rp \"E\" -c $cmd;\n"								\
"	$cmd = \"extendStrand_gui -target \" + $child;\n"								\
"	menuItem -l \"Extend\" -rp \"W\" -c $cmd;\n"									\
"	$cmd = \"applySequence_gui -target \" + $child;\n"								\
"	menuItem -l \"Apply sequence\" -rp \"SE\" -c $cmd;\n"							\
"	$cmd = \"exportStrands -target \" + $child;\n"									\
"	menuItem -l \"Export strand\" -rp \"SW\" -c $cmd;\n"							\
"}"

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
			/*
			 * Setup the marking menus for BaseShape using MEL
			 */

			MStatus status;

			if (!(status = MGlobal::executeCommandOnIdle(BASESHAPE_MARKING_MENU_COMMAND)))
				status.perror("No marking menus for BaseShape will be available: ");

			return MStatus::kSuccess;
		}

		MTypeId BaseShape::id(BASE_SHAPE_ID);
	}
}
