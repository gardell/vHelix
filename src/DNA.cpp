/*
 * DNA.cpp
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#include <DNA.h>

#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>
#include <maya/MDagPathArray.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MItDag.h>

#include <HelixBase.h>
#include <Utility.h>

#include <algorithm>

#include <BackboneArrow.h>

namespace DNA {
	static const char *strands_str[] = { STRAND_NAMES };

	const char *GetStrandName(int index) {
		return strands_str[index];
	}

	MStatus CalculateBasePairPositions(double index, MVector & forward, MVector & backward, double offset, double totalNumBases) {
		/*

x=xstart+(1-sphereRadius)*math.sin(i*(self.PITCH/360.0)*2*math.pi)
z=zstart+(1-sphereRadius)*math.cos(i*(self.PITCH/360.0)*2*math.pi)
y=ystart-self.STEP*numBases/2.0+i*self.STEP+0.165

Där i är basparets index

Och följande för reverse strand:

x=xstart+(1-sphereRadius)*math.sin(((i*self.PITCH+155)/360.0)*2*math.pi)
z=zstart+(1-sphereRadius)*math.cos(((i*self.PITCH+155)/360.0)*2*math.pi)
y=ystart-self.STEP*numBases/2.0+i*self.STEP+0.165

		 */

		double rad = Helix::toRadians(offset) + index * Helix::toRadians(-PITCH);

		forward.x = ONE_MINUS_SPHERE_RADIUS * sin(rad);
		forward.y = ONE_MINUS_SPHERE_RADIUS * cos(rad);
		forward.z = index * STEP + Z_SHIFT - totalNumBases * STEP / 2;

		rad += Helix::toRadians(OPPOSITE_ROTATION);

		backward.x = ONE_MINUS_SPHERE_RADIUS * sin(rad);
		backward.y = ONE_MINUS_SPHERE_RADIUS * cos(rad);
		backward.z = index * STEP + Z_SHIFT - totalNumBases * STEP / 2;

		return MStatus::kSuccess;
	}

	/*
	 * New model handling API
	 */

	MStatus GetMoleculeAndArrowShapes(MDagPathArray & shapes) {
		MStatus status;

		MDagPath node;

		/*
		 * Find out if the BACKBONE_ARROW_NAME node exists
		 */

		{
			MSelectionList selectionList;
			MString name(BACKBONE_ARROW_NAME);

			if (!(status = selectionList.add(name, false))) {
				/*
				 * The node does not exist. Create it
				 */

				if (!(status = MGlobal::executeCommand(CREATE_BACKBONE_ARROW_COMMAND))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				/*
				 * Now try again
				 */

				if (!(status = selectionList.add(name, false))) {
					status.perror("Couldn't find the previously loaded backbone arrow model");
					return status;
				}
			}

			if (!(status = selectionList.getDagPath(0, node))) {
				status.perror("MSelectionList::getDagPath");
				return status;
			}
		}

		/*
		 * Ok we have the dagpath to the backboneArrow transform containing all the shapes required (arrow + molecule)
		 */

		unsigned int numShapes;
		
		if (!(status = node.numberOfShapesDirectlyBelow(numShapes))) {
			status.perror("MDagPath::numberOfShapesDirectlyBelow");
			return status;
		}

		shapes.setLength(numShapes);

		for(unsigned int i = 0; i < numShapes; ++i) {
			MDagPath shape = node;
			
			if (!(status = shape.extendToShapeDirectlyBelow(i))) {
				status.perror("MDagPath::extendToShapeDirectlyBelow");
				return status;
			}

			shapes[i] = shape;
		}

		return MStatus::kSuccess;
	}
}
