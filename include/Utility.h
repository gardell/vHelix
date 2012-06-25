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
	/*
	 * Legacy API: The following methods are part of the old structure and should be removed
	 * The reason they are still around is because of the JSON importer.
	 * some of the methods are used by the new API to save code space
	 *
	 */

	MStatus SelectedBases(MObjectArray & result);
	MStatus SelectedStrands(MObjectArray & strands, MObjectArray & bases);
	
	bool HelixBase_NextBase(const MObject & base, const MObject & attribute, MDagPath & result, MStatus *retStatus = NULL);

	enum {
		FivePrime = 1,
		ThreePrime = 2,
		Neither = 0
	};

	unsigned int HelixBase_endType(MObject & base, MStatus *retStatus = NULL);

	MStatus Helix_Relatives(const MObject & helix, MObjectArray & result);

	bool HelixBase_isForward(const MObject & base, MStatus *retStatus = NULL);

	MStatus HelixBases_sort(MObjectArray & input, MObjectArray & result);

	MStatus HelixBase_RemoveAllAimConstraints(MObject & helixBase, const char *type = "aimConstraint");

	/*
	 * These are methods introduced with the new STL and MVC oriented API
	 *
	 */

	/*
	 * Returns all selected objects that are of the given type
	 */

	MStatus GetSelectedObjectsOfType(MObjectArray & objects, MTypeId & type);

	/*
	 * If object is of type `type` it is returned. If it's not, all its parents are searched
	 */

	MObject GetObjectOrParentsOfType(MObject & object, MTypeId & type, MStatus & status);

	/*
	 * These are some templates extending the algorithm methods of the STL.
	 * They should not be used where the standard STL methods can be used instead
	 */

	/*
	 * The std::for_each passes by argument, but i need to pass as reference
	 */

	template<typename It, typename Functor>
	void for_each_ref(It it, It end, Functor & func) {
		for(; it != end; ++it)
			func(*it);
	}

	/*
	 * FIXME: Try to use the std::for_each on all, even though in this case we really need the iterator to be passed as reference
	 */

	template<typename It, typename Functor>
	void for_each_itref(It & it, It end, Functor func) {
		for(; it != end; ++it)
			func(*it);
	}

	/*
	 * The std::find passes by const, which is not optimal as the Object class does some caching
	 */

	template<typename It, typename ElementT>
	It find_nonconst(It it, It end, ElementT & element) {
		for(; it != end; ++it) {
			if (*it == element)
				return it;
		}

		return end;
	}

	template<typename It, typename ElementT>
	It find_itref_nonconst(It & it, It end, ElementT & element) {
		for(; it != end; ++it) {
			if (*it == element)
				return it;
		}

		return end;
	}

	/*
	 * This method is for solving a bug in opening Maya files containing helices. FIXME: Move function definition and declaration
	 */

	void MSceneMessage_AfterImportOpen_CallbackFunc(void *callbackData);

	/*
	 * The std lacks this so found a nice template way to do it
	 */

	template <typename T> int sgn(T val) {
		return (T(0) < val) - (val < T(0));
	}
}


#endif /* UTILITY_H_ */
