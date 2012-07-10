/*
 * Base.cpp
 *
 *  Created on: 16 feb 2012
 *      Author: johan
 */

#include <model/Base.h>
#include <model/Helix.h>
#include <view/BaseShape.h>
#include <view/HelixShape.h>

#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MDGModifier.h>
#include <maya/MCommandResult.h>
#include <maya/MFnDagNode.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MMaterial.h>

#include <Helix.h>
#include <HelixBase.h>
#include <Utility.h>

#include <DNA.h>

#include <algorithm>

namespace Helix {
	namespace Model {
		MStatus Base::Create(Helix & helix, const MString & name, const MVector & translation, Base & base) {
			MStatus status;

			MObject helix_object = helix.getObject(status);

			if (!status) {
				status.perror("Helix::getObject");
				return status;
			}

			/*
			 * Create the 'HelixBase' object type
			 */

			MFnDagNode base_dagNode;
			MObject base_object = base_dagNode.create(HelixBase::id, name, helix_object, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			/*
			 * Translate the base
			 */

			MFnTransform base_transform(base_object);

			if (!(status = base_transform.setTranslation(translation, MSpace::kTransform))) {
				status.perror("MFnTransform::setTranslation");
				return status;
			}

			/*
			 * Attach the shapes
			 */

			/*
			 * Here, the Shape is instanced, which should save on memory and file size
			 * however, it seems as instancing decreases performance on large models
			 */
			

			/*if (s_shape.isNull()) {
				MFnDagNode shape_dagNode;

				s_shape = shape_dagNode.create(::Helix::View::BaseShape::id, base_object, &status);

				if (!status) {
					status.perror("MFnDagNode::create BaseShape");
					return status;
				}

				 *
				 * If the user creates a new scene, the shape will be deleted and our upcoming calls will crash
				 * by tracking the shape we'll be notified when that occurs
				 *

				MNodeMessage::addNodePreRemovalCallback(s_shape, BaseModel_Shape_NodePreRemovalCallback, NULL, &status);
			}
			else if (!(status = base_dagNode.addChild(s_shape, MFnDagNode::kNextPos, true))) {
				status.perror("MFnDagNode::addChild");
				return status;
			}*/

			/*
			 * Here a new copy of the shapes is made every time
			 */

			MFnDagNode shape_dagNode;

			MObject shape_object = shape_dagNode.create(::Helix::View::BaseShape::id, base_object, &status);

			if (!status) {
				status.perror("MFnDagNode::create BaseShape");
				return status;
			}

			base = base_object;

			return MStatus::kSuccess;
		}

		MStatus Base::AllSelected(MObjectArray & selectedBases) {
			return GetSelectedObjectsOfType(selectedBases, ::Helix::HelixBase::id);
		}

		MStatus Base::setMaterial(const Material & material) {
			if (material.getMaterial().length() == 0)
				return MStatus::kSuccess;

			MStatus status;
			MDagPath base_dagPath = getDagPath(status);

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			if (!(status = MGlobal::executeCommand(MString("sets -noWarnings -forceElement ") + material.getMaterial() + " " + base_dagPath.fullPathName()))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			return MStatus::kSuccess;
		}

		/*
		 * Optimized version that does not call any MEL commands
		 */

		MStatus Base::getMaterialColor(float & r, float & g, float & b, float & a) {
			MStatus status;

			MDagPath & dagPath = getDagPath(status);

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			/*
			 * We must iterate over the models Shape nodes, for the first we find that seem to have a material attached
			 * originally, the idea was to use the listSets -extendToShape but it doesn't seem to work...
			 */

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

				if (MFnDagNode(shape).typeId(&status) == View::BaseShape::id) {
					MPxSurfaceShapeUI *shapeUI = MPxSurfaceShapeUI::surfaceShapeUI(shape, &status);

					if (!status) {
						status.perror("MPxSurfaceShapeUI::surfaceShapeUI");
						return status;
					}

					MMaterial material = shapeUI->material(shape);

					MColor color;
					material.getDiffuse(color);

					r = color.r;
					g = color.g;
					b = color.b;
					a = color.a;

					return MStatus::kSuccess;
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}
			}

			return MStatus::kNotFound;
		}

		MStatus Base::getMaterial(Material & material) {
			MStatus status;

			//Material *materials;
			
			size_t numMaterials;
			Material::Iterator materials_begin = Material::AllMaterials_begin(status, numMaterials);

			if (!status) {
				status.perror("Material::AllMaterials_begin");
				return status;
			}

			MDagPath & dagPath = getDagPath(status);

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			/*
			 * We must iterate over the models Shape nodes, for the first we find that seem to have material attached
			 * originally, the idea was to use the listSets -extendToShape but it doesn't seem to work...
			 */

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

				MCommandResult commandResult;
				MStringArray stringArray;

				if (!(status = MGlobal::executeCommand(MString("listSets -object ") + shape.fullPathName(), commandResult))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				if (!(status = commandResult.getResult(stringArray))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				for(unsigned int j = 0; j < stringArray.length(); ++j) {
					Material::Iterator materialIt;

					if ((materialIt = std::find(materials_begin, Material::AllMaterials_end(), stringArray[j])) != Material::AllMaterials_end()) {
						material = *materialIt;
						return MStatus::kSuccess;
					}
				}
			}

			return MStatus::kNotFound;
		}

		/*Color Base::getColor(MStatus & status) {
			MPlug color_plug(getObject(status), HelixBase::aColor);

			if (!status) {
				status.perror("Base::getObject");
				return Color();
			}

			//color.index = color_plug.asInt();

			status = MStatus::kSuccess;

			return Color(color_plug.asInt());
		}

		MStatus Base::setColor(const Color & color) {
			MStatus status;
			MPlug color_plug(getObject(status), HelixBase::aColor);

			if (!status) {
				status.perror("Base::getObject");
				return status;
			}

			color_plug.setInt(color.index);
			
			return MStatus::kSuccess;
		}*/

		/*
		MStatus Base::setMaterial(const Material & material) {
			if (material.getMaterial().length() == 0)
				return MStatus::kSuccess;

			MStatus status;
			MDagPath base_dagPath = getDagPath(status);

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			if (!(status = MGlobal::executeCommand(MString("sets -noWarnings -forceElement ") + material.getMaterial() + " " + base_dagPath.fullPathName()))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Base::getMaterial(Material & material) {
			MStatus status;

			//Material *materials;
			
			size_t numMaterials;
			Material::Iterator materials_begin = Material::AllMaterials_begin(status, numMaterials);

			if (!status) {
				status.perror("Material::AllMaterials_begin");
				return status;
			}

			MDagPath & dagPath = getDagPath(status);

			if (!status) {
				status.perror("Base::getDagPath");
				return status;
			}

			 *
			 * We must iterate over the models Shape nodes, for the first we find that seem to have material attached
			 * originally, the idea was to use the listSets -extendToShape but it doesn't seem to work...
			 *

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

				MCommandResult commandResult;
				MStringArray stringArray;

				if (!(status = MGlobal::executeCommand(MString("listSets -object ") + shape.fullPathName(), commandResult))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				if (!(status = commandResult.getResult(stringArray))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				for(unsigned int j = 0; j < stringArray.length(); ++j) {
					Material::Iterator materialIt;

					if ((materialIt = std::find(materials_begin, Material::AllMaterials_end(), stringArray[j])) != Material::AllMaterials_end()) {
						material = *materialIt;
						return MStatus::kSuccess;
					}
				}
			}

			return MStatus::kNotFound;
		}
		*/

		Base::Type Base::type(MStatus & status) {
			MObject thisObject(getObject(status));

			if (!status) {
				status.perror("Base::getObject");
				return Base::BASE;
			}

			MPlug forwardPlug(thisObject, ::Helix::HelixBase::aForward),
				  backwardPlug(thisObject, ::Helix::HelixBase::aBackward);

			bool isForwardConnected = forwardPlug.isConnected(&status);

			if (!status) {
				status.perror("forwardPlug::isConnected");
				return Base::BASE;
			}

			bool isBackwardConnected = backwardPlug.isConnected(&status);

			if (!status) {
				status.perror("backwardPlug::isConnected");
				return Base::BASE;
			}

			status = MStatus::kSuccess;

			return (Base::Type) ((!isBackwardConnected ? Base::FIVE_PRIME_END : 0) | (!isForwardConnected ? Base::THREE_PRIME_END : 0));
		}

		MStatus Base::connect_forward(Base & target, bool ignorePreviousConnections) {
			MStatus status;

			/*
			 * Obtain the required objects
			 */

			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return status;
			}

			MObject targetObject = target.getObject(status);

			if (!status) {
				status.perror("Base::getObject() target");
				return status;
			}

			/*
			 * Remove all old connections
			 */

			if (!ignorePreviousConnections) {
				if (!(status = disconnect_forward())) {
					status.perror("Base::disconnect_forward");
					return status;
				}

				if (!(status = target.disconnect_backward())) {
					status.perror("Base::disconnect_backward");
					return status;
				}
			}

			MPlug forwardPlug (thisObject, HelixBase::aForward), backwardPlug (targetObject, HelixBase::aBackward);

			MDGModifier dgModifier;

			if (!(status = dgModifier.connect(backwardPlug, forwardPlug))) {
				status.perror("MDGModifier::connect");
				return status;
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}

			return MStatus::kSuccess;
		}

		/*
		 * Helper method: Disconnect all connections on the given attribute
		 */

		MStatus Base_disconnect_attribute(MObject & object, MObject & attribute) {
			/*
			 * Find all the connected objects on the forward attribute and remove them
			 */

			MStatus status;
			MPlug plug(object, attribute);
			MPlugArray targetPlugs;
			MDGModifier dgModifier;
			
			plug.connectedTo(targetPlugs, true, false, &status);

			if (!status) {
				status.perror("MPlug::connectedTo 1");
				return status;
			}

			/*
			 * Here this plug is a destination
			 */

			for(unsigned int i = 0; i < targetPlugs.length(); ++i) {
				if (!(status = dgModifier.disconnect(targetPlugs[i], plug))) {
					status.perror("MDGModifier::disconnect 1");
					return status;
				}
			}

			targetPlugs.clear(); // Dunno if required though

			plug.connectedTo(targetPlugs, false, true, &status);

			if (!status) {
				status.perror("MPlug::connectedTo 2");
				return status;
			}

			/*
			 * Here this plug is a source
			 */

			for(unsigned int i = 0; i < targetPlugs.length(); ++i) {
				if (!(status = dgModifier.disconnect(plug, targetPlugs[i]))) {
					status.perror("MDGModifier::disconnect 2");
					return status;
				}
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Base::disconnect_forward() {
			MStatus status;
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject()");
				return status;
			}

			return Base_disconnect_attribute(thisObject, ::Helix::HelixBase::aForward);
		}

		MStatus Base::disconnect_backward() {
			MStatus status;
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject()");
				return status;
			}

			return Base_disconnect_attribute(thisObject, ::Helix::HelixBase::aBackward);
		}

		MStatus Base::connect_opposite(Base & target, bool ignorePreviousConnections) {
			MStatus status;

			/*
			 * Obtain the required objects
			 */

			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return status;
			}

			MObject targetObject = target.getObject(status);

			if (!status) {
				status.perror("Base::getObject() target");
				return status;
			}

			/*
			 * Remove all old connections
			 */

			if (!ignorePreviousConnections) {
				if (!(status = disconnect_opposite())) {
					status.perror("Base::disconnect_opposite");
					return status;
				}

				if (!(status = target.disconnect_opposite())) {
					status.perror("Base::disconnect_opposite");
					return status;
				}
			}

			MPlug thisLabelPlug (thisObject, HelixBase::aLabel), targetLabelPlug (targetObject, HelixBase::aLabel);

			MDGModifier dgModifier;

			if (!(status = dgModifier.connect(thisLabelPlug, targetLabelPlug))) {
				status.perror("MDGModifier::connect");
				return status;
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Base::disconnect_opposite() {
			MStatus status;

			/*
			 * Obtain the required objects
			 */

			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return status;
			}

			MPlug thisLabelPlug (thisObject, HelixBase::aLabel);

			bool isConnected = thisLabelPlug.isConnected(&status);

			if (!status) {
				status.perror("MPlug::isConnected on label");
				return status;
			}

			if (!isConnected)
				return MStatus::kSuccess;

			MPlugArray targetLabelPlugs;
			thisLabelPlug.connectedTo(targetLabelPlugs, true, true, &status);

			if (!status) {
				status.perror("MPlug::connectedTo");
				return status;
			}
			
			MDGModifier dgModifier;

			for (unsigned int i = 0; i < targetLabelPlugs.length(); ++i) {
				MPlug targetLabelPlug = targetLabelPlugs[i];

				bool isDestination = targetLabelPlug.isDestination(&status);

				if (!status) {
					status.perror("MPlug::isDestination on label");
					return status;
				}

				if (isDestination) {
					if (!(status = dgModifier.disconnect(thisLabelPlug, targetLabelPlug))) {
						status.perror("MDGModifier::connect");
						return status;
					}
				}
				else {
					if (!(status = dgModifier.disconnect(targetLabelPlug, thisLabelPlug))) {
						status.perror("MDGModifier::connect");
						return status;
					}
				}
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Base::setLabel(DNA::Name label) {
			MStatus status;
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject");
				return status;
			}

			MPlug labelPlug(thisObject, ::Helix::HelixBase::aLabel);

			bool isDestination = labelPlug.isDestination(&status);

			if (!status) {
				status.perror("MPlug::isDestination");
				return status;
			}

			if (isDestination) {
				/*
				 * We have to set the label on the opposite base. Not this one as it is read-only
				 */

				Base oppositeBase = opposite(status);

				if (!status) {
					status.perror("Base::opposite");
					return status;
				}

				MObject oppositeBaseObject = oppositeBase.getObject(status);

				if (!status) {
					status.perror("Base::getObject opposite");
					return status;
				}

				MPlug oppositeBaseLabelPlug(oppositeBaseObject, ::Helix::HelixBase::aLabel);

				if (!(status = oppositeBaseLabelPlug.setInt((int) label.opposite()))) {
					status.perror("MPlug::setInt opposite");
					return status;
				}
			}
			else {
				if (!(status = labelPlug.setInt((int) label))) {
					status.perror("MPlug::setInt");
					return status;
				}
			}

			return MStatus::kSuccess;
		}

		MStatus Base::getLabel(DNA::Name & label) {
			MStatus status;
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject");
				return status;
			}

			MPlug labelPlug(thisObject, ::Helix::HelixBase::aLabel);

			/*
			 * Get the value from the attribute
			 */

			if (!(status = labelPlug.getValue((int &) label))) {
				status.perror("MPlug::getValue");
				return status;
			}

			/*
			 * Figure out if we are the destination on the connection, if we are, we need to inverse the value
			 */

			bool isDestination = labelPlug.isDestination(&status);

			if (!status) {
				status.perror("MPlug::getValue");
				return status;
			}

			if (isDestination)
				label = label.opposite();

			return MStatus::kSuccess;
		}

		/*
		 * Helper method used by both forward and backward
		 */

		inline Base Base_target(MObject & baseObject, MObject & attribute, MStatus & status) {
			MPlug plug(baseObject, attribute);
			MPlugArray targetPlugs;

			bool isConnected = plug.connectedTo(targetPlugs, true, true, &status);

			if (!status) {
				status.perror("MPlug::connectedTo");
				return Base();
			}
			bool hasForward = (isConnected && targetPlugs.length() > 0);

			if (hasForward) {
				status = MStatus::kSuccess;
				return Base(targetPlugs[0].node());
			}
			else {
				status = MStatus::kNotFound;
				return Base();
			}
		}

		Base Base::forward(MStatus & status) {
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			return Base_target(thisObject, ::Helix::HelixBase::aForward, status);
		}

		Base Base::forward() {
			MStatus status;

			MString path = getDagPath(status).fullPathName();

			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			// We could at least print if there are any status errors, but speed!

			return Base_target(thisObject, ::Helix::HelixBase::aForward, status);
		}

		Base Base::backward(MStatus & status) {
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			return Base_target(thisObject, ::Helix::HelixBase::aBackward, status);
		}

		Base Base::backward() {
			MStatus status;
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			// We could at least print if there are any status errors, but speed!

			return Base_target(thisObject, ::Helix::HelixBase::aBackward, status);
		}

		Base Base::opposite(MStatus & status) {
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			// We could at least print if there are any status errors, but speed!

			return Base_target(thisObject, ::Helix::HelixBase::aLabel, status);
		}

		Base Base::opposite() {
			MStatus status;
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			// We could at least print if there are any status errors, but speed!

			return Base_target(thisObject, ::Helix::HelixBase::aLabel, status);
		}

		bool Base::opposite_isDestination(MStatus & status) {
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject() this");
				return Base();
			}

			MPlug plug(thisObject, ::Helix::HelixBase::aLabel);

			return plug.isDestination(&status);
		}

		Helix Base::getParent(MStatus & status) {
			MObject thisObject = getObject(status);

			if (!status) {
				status.perror("Base::getObject");
				return Helix();
			}

			MFnDagNode this_dagNode(thisObject);
			unsigned int numParents = this_dagNode.parentCount(&status);

			if (!status) {
				status.perror("MFnDagNode::parentCount");
				return Helix();
			}

			for(unsigned int i = 0; i < numParents; ++i) {
				MObject parent = this_dagNode.parent(i, &status);

				if (!status) {
					status.perror("MFnDagNode::parent");
					return Helix();
				}

				MFnDagNode parent_dagNode(parent);

				if (parent_dagNode.typeId(&status) == ::Helix::Helix::id) {
					status = MStatus::kSuccess;
					return Helix(parent);
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return Helix();
				}
			}

			status = MStatus::kNotFound;
			return Helix();
		}
	}
}
