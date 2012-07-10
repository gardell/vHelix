/*
 * Object.h
 *
 *  Created on: 9 feb 2012
 *      Author: johan
 */

#ifndef _MODEL_OBJECT_H_
#define _MODEL_OBJECT_H_

#include <iostream>
#include <Definition.h>

#include <maya/MDagPath.h>
#include <maya/MObject.h>
#include <maya/MDGModifier.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

/*
 * Object: Base class for Helix and Base objects in the Model namespace.
 * Encapsulates MDagPath and MObject objects for easy usage of them both
 */

/*
 * Because it has many constructors and operators. These macros helps
 */

#define DEFINE_DEFAULT_INHERITED_OBJECT_CONSTRUCTORS(ClassName)																				\
	inline ClassName(const MObject & object) : Object(object) { }																			\
	inline ClassName(const MDagPath & dagPath) : Object(dagPath) { }																		\
	inline ClassName(const MObject & object, const MDagPath & dagPath) : Object(object, dagPath) { }										\
	inline ClassName(const Object & object) : Object(object) { }																			\
	inline ClassName() { }																													\
	inline ClassName & operator=(const ClassName & object) { Object::operator=(static_cast<const Object &>(object)); return *this; }		\
	inline Object & operator=(const MObject & object) { Object::operator=(object); return *this; }											\
	inline Object & operator=(const MDagPath & dagPath) { Object::operator=(dagPath); return *this; }										\
	inline bool operator==(ClassName & object) { return Object::operator==(object); }														\
	inline bool operator!=(ClassName & object) { return Object::operator!=(object); }														\
	inline bool operator==(const ClassName & object) const { return Object::operator==(object); }											\
	inline bool operator!=(const ClassName & object) const { return Object::operator!=(object); }											\
	inline bool operator==(const MObject & object) { return Object::operator==(object); }													\
	inline bool operator==(const MDagPath & dagPath) { return Object::operator==(dagPath); }												\

// With some std::find and other STL operators, we need to declare the operators backward (i.e. for example MObject == Helix::Base)
#define DEFINE_DEFAULT_INHERITED_OBJECT_INVERSED_ORDER_OPERATORS(ClassName)																	\
	inline bool operator==(MObject object, ClassName & _this) { return _this == object; }													\
	inline bool operator==(MDagPath dagPath, ClassName & _this) { return _this == dagPath; }										

namespace Helix {
	namespace Model {
		class Object {
		public:

			/*
			 * Select: Select a list of Objects. Defined as iterators of any type, thus MObjectArray, MDagPathArray, any container of Object, Base or Helix will work
			 */

			template<typename It>
			static MStatus Select(It it, It end) {
				MSelectionList list;
				MStatus status;

				for(; it != end; ++it) {
					// This is to abstract the type to Object, thus the template will work with many types as described above
					// Notice that MSelectionList did not like add(MObject) as it didn't work. Thus using MDagPaths

					MDagPath dagPath = Object(*it).getDagPath(status);

					if (!status) {
						status.perror("Object::getDagPath");
						return status;
					}

					list.add(dagPath);
				}

				if (!(status = MGlobal::setActiveSelectionList(list))) {
					status.perror("MGlobal::setActiveSelectionList");
					return status;
				}

				return MStatus::kSuccess;
			}

			/*
			 * I left these constructors non-explicit, makes it nice to use std::copy, but might confuse at times
			 */

			inline Object(const MObject & object) : m_object(object) {

			}

			inline Object(const MDagPath & dagPath) : m_dagPath(dagPath) {

			}

			inline Object(const MObject & object, const MDagPath & dagPath) : m_object(object), m_dagPath(dagPath) {

			}

			inline Object(const Object & object) : m_object(object.m_object), m_dagPath(object.m_dagPath) {

			}

			inline Object() {

			}

			inline Object & operator=(const Object & object) {
				m_object = object.m_object;
				m_dagPath = object.m_dagPath;

				return *this;
			}

			inline Object & operator=(const MObject & object) {
				m_object = object;
				m_dagPath = MDagPath();

				return *this;
			}

			inline Object & operator=(const MDagPath & dagPath) {
				m_object = MObject::kNullObj;
				m_dagPath = dagPath;

				return *this;
			}

			inline bool operator==(Object & object) {
				MStatus status;
				MObject thisObject = getObject(status);

				if (!status) {
					status.perror("Object::getObject() on this");
					return false;
				}

				MObject targetObject = object.getObject(status);

				if (!status) {
					//status.perror("This error is normal. Object::getObject() on target");
					return false;
				}

				return thisObject == targetObject;
			}

			/*
			 * Some STL code requires the operator== to be const
			 * Notice: The above code is faster as it allows for caching.
			 * According to the specification, even though the compiler could choose the most restrictive
			 * it will always choose the ones above when possible
			 */

			inline bool operator==(const Object & object) const {
				MStatus status;
				MObject thisObject = getObject(status);

				if (!status) {
					status.perror("Object::getObject() on this");
					return false;
				}

				MObject targetObject = object.getObject(status);

				if (!status) {
					//status.perror("This error is normal. Object::getObject() on target");
					return false;
				}

				return thisObject == targetObject;
			}

			inline bool operator!=(Object & object) {
				return !this->operator==(object);
			}

			inline bool operator!=(const Object & object) const {
				return !this->operator==(object);
			}

			inline bool operator==(const MObject & object) {
				MStatus status;

				MObject & thisObject = getObject(status);

				if (!status) {
					status.perror("Object::getObject");
					return false;
				}

				return thisObject == object;
			}

			inline bool operator!=(const MObject & object) {
				return !this->operator==(object);
			}

			inline bool operator==(const MDagPath & dagPath) {
				MStatus status;

				MDagPath & thisDagPath = getDagPath(status);

				if (!status) {
					status.perror("Object::getDagPath");
					return false;
				}

				return thisDagPath == dagPath;
			}

			MObject & getObject(MStatus & status);
			MDagPath & getDagPath(MStatus & status);

			/*
			 * Notice: As mentioned above, these methods are convenient in the STL library
			 * but are slower because they're not able to cache the values for faster access
			 */

			MObject getObject(MStatus & status) const;
			MDagPath getDagPath(MStatus & status) const;

			inline operator bool() const {
				return isValid();
			}

			inline bool operator!() const {
				return !isValid();
			}

			inline bool isValid() const {
				return !m_object.isNull() || m_dagPath.isValid();
			}

			/*
			 * Positions/Orientations
			 */

			MStatus getTransform(MTransformationMatrix & matrix);
			MStatus getTranslation(MVector & vector, MSpace::Space space);
			MStatus getRotation(MEulerRotation & rotation);

			/*
			 * Note that the class invalidates itself after this
			 */

			inline MStatus deleteNode() {
				MDGModifier dgModifier;
				MStatus status;

				/*
				 * There's a bug with MDGModifier::deleteNode that makes Maya crash, using MEL as a last resort
				 */

				/*
				MObject thisObject = getObject(status);

				if (!status) {
					status.perror("Object::getObject");
					return status;
				}

				if (!(status = dgModifier.deleteNode(thisObject))) {
					status.perror("MDGModifier::deleteNode");
					return status;
				}

				if (!(status = dgModifier.doIt())) {
					status.perror("MDGModifier::doIt");
					return status;
				}*/

				MDagPath dagPath = getDagPath(status);

				if (!status) {
					status.perror("Object::getDagPath");
					return status;
				}

				if (!(status = MGlobal::executeCommand(MString("delete ") + dagPath.fullPathName()))) {
					status.perror(MString("MGlobal::executeCommand(\"delete ") + dagPath.fullPathName() + "\")");
					return status;
				}

				/*
				 * Invalidate the object from further usage
				 */

				*this = MObject::kNullObj;
				
				return MStatus::kSuccess;
			}

		protected:
			inline void setObject(MObject object) {
				m_object = object;
			}

			inline void setDagPath(MDagPath & dagPath) {
				m_dagPath = dagPath;
			}

		private:
			MStatus LoadObjectFromDagPath();
			MStatus LoadAnyDagPathFromObject();

			// Inheriting classes must go through above methods and can't access these directly
			MObject m_object;
			MDagPath m_dagPath;
		};

		DEFINE_DEFAULT_INHERITED_OBJECT_INVERSED_ORDER_OPERATORS(Object);
	}
}

#endif /* _MODEL_OBJECT_H_ */
