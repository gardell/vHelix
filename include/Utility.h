/*
 * Utility.h
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#ifndef UTILITY_H_
#define UTILITY_H_

/*
 * Utility methods reused at various places
 * TODO: Move all helper methods into this file
 */

#include <Definition.h>

#include <iostream>
#include <vector>

#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MDagPath.h>
#include <maya/MVector.h>
#include <maya/MTypeId.h>

namespace Helix {
	// These are some utility methods used all around the project
	// Some cleanup and possibly making some kind of object oriented stuff would be great
	// Exporting some Utility methods into MEL would also be great as it would make it easier to do MEL and Python extensions to the plugin
	//

	// FIXME: Do some functor classes, one BaseUtility, HelixUtility etc that can be used to manipulate helices and bases
	//	The class should have both internal MDagPath and MObject and private getters that loads the DagPath only when requested (save some mem and CPU)
	//	Makes it easier to use these functions as our code now heavily depends on whether the method has access to the dagpath or object
	// FIXME: Export some of the functionality as MEL functions as mentioned above

	// SelectedBases: Get all currently selected HelixBase objects (even though a child might be selected)
	//

	MStatus SelectedBases(MObjectArray & result);
	MStatus SelectedStrands(MObjectArray & strands, MObjectArray & bases);
	MStatus SelectedHelices(MObjectArray & result);

	// Disconnect all connections to HelixBase objects on the targetPlug, if targetPlug is to be considered as a connection destination, set isSource to false
	//

	MStatus DisconnectAllHelixBaseConnections(MPlug targetPlug, bool isSource, MObject *old_target = NULL);

	// Find the first connected HelixBase connected to the base's attribute (for example aForward or aBackward
	// Returns true if it finds a HelixBase, false if not (retStatus will be set if there was an error)
	//

	bool HelixBase_NextBase(const MObject & base, const MObject & attribute, MDagPath & result, MStatus *retStatus = NULL);

	// FIXME: Use the method below instead
	bool HelixBase_isEnd(MObject & base, MStatus *retStatus = NULL);

	enum {
		FivePrime = 1,
		ThreePrime = 2,
		Neither = 0
	};

	unsigned int HelixBase_endType(MObject & base, MStatus *retStatus = NULL);

	// Get all strands under a helix, does NOT put bases of the strands that does not belong to this base!
	// A bit buggy, puts the same bases multiple times

	MStatus Helix_Strands(MDagPath helix, std::vector<MDagPathArray> & strands);

	// Get all the bases forward or backward connected to this base
	MStatus Base_Strands(MDagPath base, MDagPathArray & neighbours);

	// Create a new base, used by JSONTranslator, CreateHelix, Extend etc
	MStatus Helix_CreateBase(MObject & helix, MString name, MVector translation, MObject & result);

	// Find all connected helices (connected by base connections)
	MStatus Helix_Relatives(const MObject & helix, MObjectArray & result);

	template<typename Array, typename Object>
	bool ArrayContainsObject(Array & array, Object & object);

	// This method is new, and uses the Helix_Forward and Helix_Backward attributes instead of forward, backward. It is used ONLY by the exporter
	// Result will contain all the bases along with the helix endType enum
	// Note that if helices have been deleted, there will be holes in the structure, and even helix_forward and helix_backward won't work
	MStatus Helix_Creation_Ends(MObject & helix, std::vector<std::pair<MObject, int> > & result);

	// Tries to guess if this base is directed along the cylinder in the positive direction (uses z axis). If it's neither, its assumed not to (a staple is in the negative)
	bool HelixBase_isForward(const MObject & base, MStatus *retStatus = NULL);

	// Used only by the exporter to sort the bases along the Z-axis, uses plain linear sort.
	// NOTE: input will be EMPTY when done, saves some memory/CPU
	MStatus HelixBases_sort(MObjectArray & input, MObjectArray & result);

	// The MDagPath::extendToShape does only work for shapes directly below the node, but this method will traverse over the nodes one level below this one too
	// It's just for finding sets of an HelixBase's molecule and arrows

	MStatus HelixBase_isMemberOfSet(MDagPath & base, const MString & set, bool & result);

	// Used by the HelixBase to clear all aimconstraints before applying new ones
	MStatus HelixBase_RemoveAllAimConstraints(MObject & helixBase, const char *type = "aimConstraint");

	/*
	 * This method is introduced with the new API. Returns all selected objects that are of the given type
	 */

	MStatus GetSelectedObjectsOfType(MObjectArray & objects, MTypeId & type);

	/*
	 * Also part of the new API. If object is of type `type` it is returned. If it's not, all its parents are searched
	 */

	MObject GetObjectOrParentsOfType(MObject & object, MTypeId & type, MStatus & status);

	/*
	 * Many methods in the new API still work with MObjectArray or MDagPathArray for performance reasons, this template converts them to containers of Model::Base or Model::Helix objects
	 */

	/*template<typename T, typename ArrayT, typename ContainerT>
	void ArrayToContainer(const ArrayT & array, ContainerT & container) {
		for(
	}*/

	/*
	 * The std::for_each passes by argument, but i need to pass as reference
	 */

	template<typename It, typename Functor>
	void for_each_ref(It it, It end, Functor & func) {
		for(; it != end; ++it)
			func(*it);
	}
}


#endif /* UTILITY_H_ */
