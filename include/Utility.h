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

#include <model/Object.h>

#include <iostream>
#include <vector>
#include <iterator>

#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MDagPath.h>
#include <maya/MVector.h>
#include <maya/MTypeId.h>

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>

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

	template<typename It, typename Functor>
	void for_each_ref_itref(It & it, It end, Functor & func) {
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

	/*
	 * For commands: find all the nodes identified by the names given as arguments to parameters
	 * The original version used member function pointers, thus:
	 * MStatus (MSelectionList::* GetElementFunc) (unsigned int index, ElementT & element) const
	 * worked, but only for getDependNode, as getDagPath has a third argument with a default value
	 */

	template<typename ArrayT, typename ElementT, ElementT (*GetElementFunc) (const MSelectionList & list, unsigned int index)>
	MStatus ArgList_ArgumentNodes(const MArgList & args, const MSyntax & syntax, const char *flag, ArrayT & array) {
		MStatus status;
		MArgDatabase argDatabase(syntax, args, &status);

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		bool isFlagSet = argDatabase.isFlagSet(flag, &status);

		if (!status) {
			status.perror("MArgDatabase::isFlagSet");
			return status;
		}

		if (!isFlagSet)
			return MStatus::kNotFound;

		MSelectionList list;

		unsigned int num = argDatabase.numberOfFlagUses(flag);

		for(unsigned int i = 0; i < num; ++i) {
			MString path;

			if (!(status = argDatabase.getFlagArgument(flag, i, path))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			if (!(status = list.add(path))) {
				status.perror(MString("MSelectionList::add: The object \"") + path + "\" was not found");
				//return status;
			}
		}

		/*
		 * Now extract the dagpaths or objects from the MSelectionList
		 */

		for(unsigned int i = 0; i < list.length(); ++i) {
			ElementT element;

			/*
			if (!(status = (list.*GetElementFunc) (i, element))) {
				status.perror("MSelectionList::*GetElementFunc");
				return status;
			}
			*/

			array.append(GetElementFunc(list, i));
		}

		return MStatus::kSuccess;
	}

	/*
	 * The above method was written to not distinguish between MDagPaths and MObjects, but as its syntax is a bit messy
	 */

	MStatus ArgList_GetObjects(const MArgList & args, const MSyntax & syntax, const char *flag, MObjectArray & result);
	MStatus ArgList_GetDagPaths(const MArgList & args, const MSyntax & syntax, const char *flag, MDagPathArray & result);

	/*
	 * We could work with the lists of helices/bases directly and save some performance
	 */

	template<typename ElementT, typename ArrayT, typename ContainerT, MStatus (*GetElementsFunc) (const MArgList &, const MSyntax &, const char *, ArrayT &) >
	MStatus ArgList_GetModelObjectsT(const MArgList & args, const MSyntax & syntax, const char *flag, ContainerT & result) {
		MStatus status;
		ArrayT objects;

		if (!status = GetElementsFunc(args, syntax, flag, objects)) {
			status.perror("GetElementsFunc");
			return status;
		}

		std::copy(&objects[0], &objects[0] + objects.length(), std::back_insert_iterator<ContainerT> (result));

		return MStatus::kSuccess;
	}

	/*
	 * Some specializations as the syntax is messy for the function above
	 */

	template<typename ElementT, typename ContainerT>
	inline MStatus ArgList_GetModelObjects(const MArgList & args, const MSyntax & syntax, const char *flag, ContainerT & result) {
		return ArgList_GetModelObjectsT<MObject, MObjectArray, ContainerT, ArgList_GetObjects> (args, syntax, flag, result);
	}

	/*
	 * Extract CV curves from an MObject
	 */

	MStatus CVCurveFromObject(const Model::Object & object, MPointArray & array);
}


#endif /* UTILITY_H_ */
