/*
 * DNA.h
 *
 *  Created on: Jun 28, 2011
 *      Author: bjorn
 */

#ifndef DNA_H_
#define DNA_H_

#include <Definition.h>

#include <cmath>
#include <iostream>

#include <maya/MDagPath.h>
#include <maya/MStringArray.h>
#include <maya/MObjectArray.h>
#include <maya/MVector.h>

#ifndef DEG2RAD
#define DEG2RAD(deg)	((deg) / 180.0 * M_PI)
#endif /* N DEG2RAD */

#ifndef RAD2DEG
#define RAD2DEG(rad)	((rad) / M_PI * 180.0)
#endif /* N RAD2DEG */

// FIXME: In the future, this should be a relative path to the plugin, so that it can be easily installed
#define HELIX_GEOMETRY_PATH			""

#define DNASHADERS_SOURCE_FILE	HELIX_GEOMETRY_PATH "DNAshaders.ma"
#define HELIXBASE_MODEL_SOURCE_FILES HELIX_GEOMETRY_PATH "BackboneArrow.ma"

namespace DNA {
	// Some DNA properties as well as visual settings.
	//

	const double PITCH = 720.0 / 21.0, // degrees
				 STEP = 0.334,
				 RADIUS = 1.0,
				 SPHERE_RADIUS = 0.13, // not dna properties, but visual
				 HELIX_RADIUS = RADIUS + SPHERE_RADIUS + 0.20; // TODO: This looked visually correct :-)

	// Since Create, Extend and Import all use the properties above to generate the correct positions for each base
	// This utility method should be used and not the parameters above

	MStatus CalculateBasePairPositions(double index, MVector & forward, MVector & backward, double offset = 0.0);

	const float SEQUENCE_RENDERING_Y_OFFSET = 0.22f; // Multiplied by RADIUS

	// Constants for generating the honeycomb lattice

	const double HONEYCOMB_X_STRIDE = 2.0 * DNA::HELIX_RADIUS * cos(DEG2RAD(30)),
						 HONEYCOMB_Y_STRIDE = 2.0 * DNA::HELIX_RADIUS * (1.0 + sin(DEG2RAD(30))),
						 HONEYCOMB_Y_OFFSET = 1.0 * DNA::HELIX_RADIUS * sin(DEG2RAD(30));

	// Some defines that makes the code look nicer and easier to read, when dealing directly with DNA bases
	//

	enum Names {
		A = 0,
		T = 1,
		G = 2,
		C = 3,
		Invalid = 4
	};

	static const char *strands[] = { "forv", "rev" };

	const long CREATE_DEFAULT_NUM_BASES = 16;

	const int BASES = 4; // Trivial, but still, cleaner code with defines

	Names OppositeBase(Names d);

	const char ToChar(Names d);
	Names ToName(char c);

	// References to some static models often required
	// Note: Just because it has been loaded does not mean it's there, if the user creates a new file etc
	//

	MStatus GetMoleculeModel(MDagPath & result);
	MStatus GetArrowModel(MDagPath & result);

	// Get the DNA*-materials (loads them if they're not valid)
	//

	MStatus GetMaterials(MStringArray & result);

	// Assign a material to a list of objects, use the above method to get a list of available materials
	//

	MStatus AssignMaterialToObjects(const MDagPathArray & dagPaths, const MString & material);
	MStatus AssignMaterialsToObjects(const MDagPathArray & dagPaths, const MStringArray & materials);

	MStatus QueryMaterialOfObject(MObject base, MString & result);
	MStatus QueryMaterialsOfObjects(const MDagPathArray & objects, MStringArray & colors);
}

#endif /* DNA_H_ */
