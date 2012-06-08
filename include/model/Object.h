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

/*
 * Object: Base class for Helix and Base objects in the Model namespace.
 * Encapsulates MDagPath and MObject objects for easy usage of them both
 */

/*
 * Because it has many constructors. This macro helps
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
	inline bool operator!=(ClassName & object) { return Object::operator!=(object); }

namespace Helix {
	namespace Model {
		class Object {
		public:
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
					status.perror("Object::getObject() on target");
					return false;
				}

				return thisObject == targetObject;
			}

			inline bool operator!=(Object & object) {
				return !this->operator==(object);
			}

			MObject & getObject(MStatus & status);
			MDagPath & getDagPath(MStatus & status);

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
	}
}

#endif /* _MODEL_OBJECT_H_ */
