/*
 * Object.cpp
 *
 *  Created on: 9 feb 2012
 *      Author: johan
 */

#include <model/Object.h>

#include <maya/MFnTransform.h>
#include <maya/MEulerRotation.h>
#include <maya/MVector.h>
#include <maya/MPlug.h>

namespace Helix {
	namespace Model {
		MObject Object::getObject(MStatus & status) const {
			status = MStatus::kSuccess;
			
			if (m_object.isNull()) {
				if (!m_dagPath.isValid()) {
					status = MStatus::kInvalidParameter;
					return MObject::kNullObj;
				}

				MObject object = m_dagPath.node(&status);
				
				if (!status) {
					status.perror("MDagPath::node");
					return MObject::kNullObj;
				}

				return object;
			}

			return m_object;
		}

		MDagPath Object::getDagPath(MStatus & status) const {
			bool isValid = m_dagPath.isValid(&status);

			if (!status) {
				status.perror("MDagPath::isValid");
				return m_dagPath;
			}

			if (!isValid) {
				if (m_object.isNull()) {
					status = MStatus::kInvalidParameter;
					return MDagPath();
				}

				MDagPath dagPath;

				if (!(status = MDagPath::getAPathTo(m_object, dagPath))) {
					status.perror("MDagPath::getAPathTo");
					return MDagPath();
				}

				return dagPath;
			}

			return m_dagPath;
		}

		MObject & Object::getObject(MStatus & status) {
			status = MStatus::kSuccess;						
			
			if (m_object.isNull()) {
				if (!m_dagPath.isValid())
					status = MStatus::kInvalidParameter;
				else if (!(status = LoadObjectFromDagPath()))
					status.perror("Object::LoadObjectFromDagPath");
			}

			return m_object;
		}

		MDagPath & Object::getDagPath(MStatus & status) {
			bool isValid = m_dagPath.isValid(&status);

			if (!status) {
				status.perror("MDagPath::isValid");
				return m_dagPath;
			}

			if (!isValid) {
				if (m_object.isNull())
					status = MStatus::kInvalidParameter;
				else if (!(status = LoadAnyDagPathFromObject()))
					status.perror("Object::LoadAnyDagPathFromObject");
			}

			return m_dagPath;
		}

		MStatus Object::LoadObjectFromDagPath() {
			MStatus status;
			m_object = m_dagPath.node(&status);

			return status;
		}

		MStatus Object::LoadAnyDagPathFromObject() {
			return MDagPath::getAPathTo(m_object, m_dagPath);
		}

		MStatus Object::getTransform(MTransformationMatrix & matrix) {
			MStatus status;

			MFnTransform transform(getDagPath(status));

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			matrix = transform.transformation(&status);

			if (!status) {
				status.perror("MFnTransform::transformation");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Object::getTranslation(MVector & vector, MSpace::Space space) {
			MStatus status;

			MFnTransform transform(getDagPath(status));

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			vector = transform.getTranslation(space, &status);

			if (!status) {
				status.perror("MFnTransform::getTranslation");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Object::getRotation(MEulerRotation & rotation) {
			MStatus status;

			MFnTransform transform(getDagPath(status));

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			if (!(status = transform.getRotation(rotation))) {
				status.perror("MFnTransform::getRotation");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Object::setShapesVisibility(bool visible, MTypeId & typeId) {
			MStatus status;
			MDagPath dagPath = getDagPath(status);

			if (!status) {
				status.perror("Object::getDagPath");
				return status;
			}

			unsigned int numShapes;

			if (!(status = dagPath.numberOfShapesDirectlyBelow(numShapes))) {
				status.perror("MDagPath::numberOfShapesDirectlyBelow");
				return status;
			}

			for(unsigned int i = 0; i < numShapes; ++i) {
				MDagPath shape = dagPath;

				if (!(status = shape.extendToShapeDirectlyBelow(i))) {
					status.perror("MDagPath::extendToShapeDirectlyBelow");
					return status;
				}

				MFnDagNode shape_dagNode(shape);

				if (shape_dagNode.typeId(&status) == typeId) {
					MPlug visibilityPlug(shape.node(), shape_dagNode.findPlug("visibility", &status));

					if (!status) {
						status.perror("MFnDagNode::findPlug");
						return status;
					}

					if (!(status = visibilityPlug.setBool(visible))) {
						status.perror("MPlug::setBool");
						return status;
					}
				}
				else if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}
			}

			return MStatus::kSuccess;
		}

		MStatus Object::toggleShapesVisibility(MTypeId & typeId) {
			MStatus status;
			MDagPath dagPath = getDagPath(status);

			if (!status) {
				status.perror("Object::getDagPath");
				return status;
			}

			unsigned int numShapes;

			if (!(status = dagPath.numberOfShapesDirectlyBelow(numShapes))) {
				status.perror("MDagPath::numberOfShapesDirectlyBelow");
				return status;
			}

			for(unsigned int i = 0; i < numShapes; ++i) {
				MDagPath shape = dagPath;

				if (!(status = shape.extendToShapeDirectlyBelow(i))) {
					status.perror("MDagPath::extendToShapeDirectlyBelow");
					return status;
				}

				MFnDagNode shape_dagNode(shape);

				if (shape_dagNode.typeId(&status) == typeId) {
					MPlug visibilityPlug(shape.node(), shape_dagNode.findPlug("visibility", &status));

					if (!status) {
						status.perror("MFnDagNode::findPlug");
						return status;
					}

					if (!(status = visibilityPlug.setBool(!visibilityPlug.asBool()))) {
						status.perror("MPlug::setBool");
						return status;
					}
				}
			}

			return MStatus::kSuccess;
		}

		bool Object::isAnyShapeVisible(MTypeId & typeId, MStatus & status) {
			MDagPath dagPath = getDagPath(status);

			if (!status) {
				status.perror("Object::getDagPath");
				return false;
			}

			unsigned int numShapes;

			if (!(status = dagPath.numberOfShapesDirectlyBelow(numShapes))) {
				status.perror("MDagPath::numberOfShapesDirectlyBelow");
				return false;
			}

			for(unsigned int i = 0; i < numShapes; ++i) {
				MDagPath shape = dagPath;

				if (!(status = shape.extendToShapeDirectlyBelow(i))) {
					status.perror("MDagPath::extendToShapeDirectlyBelow");
					return false;
				}

				MFnDagNode shape_dagNode(shape);

				if (shape_dagNode.typeId(&status) == typeId) {
					MPlug visibilityPlug(shape.node(), shape_dagNode.findPlug("visibility", &status));

					if (!status) {
						status.perror("MFnDagNode::findPlug");
						return false;
					}

					if (visibilityPlug.asBool()) {
						status = MStatus::kSuccess;
						return true;
					}
				}
			}

			status = MStatus::kSuccess;
			return false;
		}
	}
}
