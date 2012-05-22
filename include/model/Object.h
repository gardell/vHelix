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

/*
 * Object: Base class for Helix and Base objects in the Model namespace.
 * Encapsulates MDagPath and MObject objects for easy usage of them both
 */

/*
 * Because it has many constructors. This macro helps
 */

#define DEFINE_DEFAULT_INHERITED_OBJECT_CONSTRUCTORS(ClassName)											\
	explicit inline ClassName(const MObject & object) : Object(object) { }								\
	explicit inline ClassName(const MDagPath & dagPath) : Object(dagPath) { }							\
	inline ClassName(const MObject & object, const MDagPath & dagPath) : Object(object, dagPath) { }	\
	inline ClassName(const Object & object) : Object(object) { }										\
	inline ClassName() { }

namespace Helix {
	namespace Model {
		class Object {
		public:
			explicit inline Object(const MObject & object) : m_object(object) {

			}

			explicit inline Object(const MDagPath & dagPath) : m_dagPath(dagPath) {

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

			// I hope it still works even though none of them are const (and can't be)
			inline bool operator==(Object & object) {
				MStatus status;
				MObject thisObject = getObject(status);

				if (!status) {
					status.perror("Object::getObject() on this");
					return status;
				}

				MObject targetObject = object.getObject(status);

				if (!status) {
					status.perror("Object::getObject() on target");
					return status;
				}

				return thisObject == targetObject;
			}

			MObject & getObject(MStatus & status);
			MDagPath & getDagPath(MStatus & status);

			inline operator bool() {
				return isValid();
			}

			inline bool operator!() const {
				return !isValid();
			}

			inline bool isValid() const {
				return !m_object.isNull() || m_dagPath.isValid();
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
	}
}

#endif /* _MODEL_OBJECT_H_ */
