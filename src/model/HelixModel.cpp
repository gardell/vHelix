/*
 * Helix.cpp
 *
 *  Created on: 16 feb 2012
 *      Author: johan
 */

#include <model/Helix.h>
#include <model/Base.h>
#include <view/HelixShape.h>

#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>

#include <Helix.h>
#include <HelixBase.h>
#include <Locator.h>
#include <Utility.h>
#include <ToggleCylinderBaseView.h>

#include <algorithm>

#define CYLINDERREPRESENTATION_NAME "cylinderRepresentation"

namespace Helix {
	namespace Model {
		MStatus Helix::Create(const MString & name, const MMatrix & transform, Helix & helix) {
			MStatus status;

			MFnDagNode helix_dagNode;
			MObject helix_object = helix_dagNode.create(::Helix::Helix::id, name, MObject::kNullObj, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			helix.setObject(helix_object);

			/*
			 * Do a `setAttr m_helix.displayHandle true` to enable the displayHandle view
			 */

			{
				MPlug displayHandle(helix_object, MPxTransform::displayHandle);

				if (!(status = displayHandle.setBool(true)))
					status.perror("MPlug::setBool. The displayHandle will not be visible");
			}

			/*
			 * Now generate the helix locator node that will display visual information
			 */

			MFnDagNode locator_dagNode;
			MObject locator_object = locator_dagNode.create(::Helix::HelixLocator::id, helix_object, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			/*
			 * Generate the helix shape node that will render bases
			 */

			MFnDagNode shape_dagNode;
			MObject shape_object = shape_dagNode.create(::Helix::View::HelixShape::id, helix_object, &status);

			if (!status) {
				status.perror("MFnDagNode::create HelixShape");
				return status;
			}

			/*
			 * Set the transform
			 */

			MDagPath helix_dagPath;
			if (!(status = helix_dagNode.getPath(helix_dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			MFnTransform helix_transform(helix_dagPath);
			MTransformationMatrix transformationMatrix(transform);

			if (!(status = helix_transform.set(transformationMatrix))) {
				status.perror("MFnTransform::set");
				return status;
			}

			return MStatus::kSuccess;
		}

		/*
		 * Helper method, maybe a bit too general, could be put in Utility.cpp
		 */

		/*MStatus FindChildWithNameContaining(MDagPath & object, const char *contains, MDagPath & result) {
			MStatus status;

			unsigned int numChildren = object.childCount(&status);

			if (!status) {
				status.perror("MDagPath::childCount");
				return status;
			}

			for(unsigned int i = 0; i < numChildren; ++i) {
				Object child(object.child(i, &status));

				if (!status) {
					status.perror("MDagPath::child");
					return status;
				}

				MDagPath child_dagPath = child.getDagPath(status);

				if (!status) {
					status.perror("Object::getDagPath");
					return status;
				}

				MString child_fullPathName = child_dagPath.fullPathName();
				
				if (strstr(child_fullPathName.asChar(), CYLINDERREPRESENTATION_NAME)) {
					result = child_dagPath;
					return MStatus::kSuccess;
				}
			}

			return MStatus::kNotFound;
		}*/

		/*
		 * Helper method. Get the cylinder object
		 */

		/*MStatus Helix_getCylinder(MDagPath & helix, MDagPath & cylinder) {
			return FindChildWithNameContaining(helix, CYLINDERREPRESENTATION_NAME, cylinder);
		}*/

		/*
		 * Helper method. Get the bottom and top caps of the cylinder
		 */

		/*MStatus Helix_getCylinderCaps(MDagPath & cylinder, MDagPath & topCap, MDagPath & bottomCap) {
			MStatus status;
			
			if (!(status = FindChildWithNameContaining(cylinder, CYLINDERREPRESENTATION_NAME "_topCap", topCap))) {
				status.perror("FindChildWithNameContaining topCap");
				return status;
			}

			if (!(status = FindChildWithNameContaining(cylinder, CYLINDERREPRESENTATION_NAME "_bottomCap", bottomCap))) {
				status.perror("FindChildWithNameContaining bottomCap");
				return status;
			}

			return MStatus::kSuccess;
		}*/

		/*
		 * Another helper method. This gets the makeNurbCylinder object connected to the cylinders shape
		 */

		/*MStatus Helix_getPolyCylinder(MDagPath & cylinder, MObject & polyCylinder) {
			MStatus status;

			/*
			 * Extend the cylinder to the shape
			 *

			unsigned int numShapes;

			if (!(status = cylinder.numberOfShapesDirectlyBelow(numShapes))) {
				status.perror("MDagPath::numberOfShapesDirectlyBelow");
				return status;
			}


			for(unsigned int i = 0; i < numShapes; ++i) {
				MDagPath cylinderShape = cylinder;

				if (!(status = cylinderShape.extendToShapeDirectlyBelow(i))) {
					status.perror("MDagPath::extendToShapeDirectlyBelow");
					return status;
				}

				MObject cylinderShapeObject = cylinderShape.node(&status);

				if (!status) {
					status.perror("MDagPath::node shape");
					return status;
				}

				MFnDependencyNode cylinder_dependencyNode(cylinderShapeObject);

				/*
				 * Follow the .create attribute on the cylinder shape
				 *

				MObject createAttribute = cylinder_dependencyNode.attribute("inMesh", &status);

				if (!status) {
					status.perror("MFnDependencyNode::attribute");
					return status;
				}

				MPlug createPlug(cylinderShapeObject, createAttribute);

				MPlugArray targetPlugs;
				bool isConnected = createPlug.connectedTo(targetPlugs, true, true, &status);

				if (!status) {
					status.perror("MPlug::connectedTo");
					return status;
				}

				if (!isConnected || targetPlugs.length() < 1) {
					std::cerr << "There is no connected plug on the cylinder shape" << std::endl;
					return MStatus::kFailure;
				}

				polyCylinder = targetPlugs[0].node(&status);

				if (!status) {
					status.perror("MDagPathArray[0]::node");
					return status;
				}

				return MStatus::kSuccess;
			}

			return MStatus::kNotFound;
		}*/

		/*MStatus Helix_getMakeNurbCylinder(MDagPath & cylinder, MObject & makeNurbCylinder) {
			MStatus status;

			 *
			 * Extend the cylinder to the shape
			 *

			MDagPath cylinderShape = cylinder;

			if (!(status = cylinderShape.extendToShape())) {
				status.perror("MDagPath::extendToShape");
				return status;
			}

			MObject cylinderShapeObject = cylinderShape.node(&status);

			if (!status) {
				status.perror("MDagPath::node shape");
				return status;
			}

			MFnDependencyNode cylinder_dependencyNode(cylinderShapeObject);

			 *
			 * Follow the .create attribute on the cylinder shape
			 *

			MObject createAttribute = cylinder_dependencyNode.attribute("create", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute");
				return status;
			}

			MPlug createPlug(cylinderShapeObject, createAttribute);

			MPlugArray targetPlugs;
			bool isConnected = createPlug.connectedTo(targetPlugs, true, true, &status);

			if (!status) {
				status.perror("MPlug::connectedTo");
				return status;
			}

			if (!isConnected || targetPlugs.length() < 1) {
				std::cerr << "There is no connected plug on the cylinder shape" << std::endl;
				return MStatus::kFailure;
			}

			makeNurbCylinder = targetPlugs[0].node(&status);

			if (!status) {
				status.perror("MDagPathArray[0]::node");
				return status;
			}

			return MStatus::kSuccess;
		}*/

		/*
		 * Help method, change the Z coordinate of an MDagPath object via a MFnTransform. Used to move not only the cylinder, but its top and bottom caps
		 */

		/*MStatus SetZTranslationOnDagPath(const MDagPath & dagPath, double z) {
			MStatus status;

			/*
			 * Find the translation of the object
			 *

			MVector translation;
			MFnTransform transform(dagPath);

			translation = transform.getTranslation(MSpace::kTransform, &status);

			if (!status) {
				status.perror("MFnTransform::getTranslation");
				return status;
			}

			/*
			 * Ok, update the translation
			 *

			translation.z = z;

			if (!(status = transform.setTranslation(translation, MSpace::kTransform))) {
				status.perror("MFnTransform::setTranslation");
				return status;
			}

			return MStatus::kSuccess;
		}*/

		MStatus Helix::setCylinderRange(double origo, double height) {
			MStatus status;
			MDagPath helix = getDagPath(status), cylinder;

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			MDagPath helix_shape;
			unsigned int numShapes;
			
			if (!(status = helix.numberOfShapesDirectlyBelow(numShapes))) {
				status.perror("MDagPath::numberOfShapesDirectlyBelow");
				return status;
			}

			for(unsigned int i = 0; i < numShapes; ++i) {
				helix_shape = helix;

				if (!(status = helix_shape.extendToShapeDirectlyBelow(i))) {
					status.perror("MDagPath::extendToShapeDirectlyBelow");
					return status;
				}

				MFnDagNode helix_shape_dagNode(helix_shape);

				if (helix_shape_dagNode.typeId(&status) == View::HelixShape::id) {
					/*
					 * Found the correct shape
					 */

					MObject helix_shape_object = helix_shape.node(&status);

					if (!status) {
						status.perror("MDagPath::node");
						return status;
					}

					MPlug origoPlug(helix_shape_object, View::HelixShape::aOrigo), heightPlug(helix_shape_object, View::HelixShape::aHeight);

					if (!(status = origoPlug.setFloat((float) origo))) {
						status.perror("OrigopPlug::setFloat");
						return status;
					}

					if (!(status = heightPlug.setFloat((float) height))) {
						status.perror("HeightPlug::setFloat");
						return status;
					}

					return MStatus::kSuccess;
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return status; 
				}
			}

			return MStatus::kNotFound;

			/*MStatus status;
			MDagPath helix = getDagPath(status), cylinder;//, topCap, bottomCap;

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			if (!(status = Helix_getCylinder(helix, cylinder))) {
				if (status == MStatus::kNotFound)
					return createCylinder(origo, height);

				status.perror("Helix_getCylinder");
				return status;
			}

			if (!(status = SetZTranslationOnDagPath(cylinder, origo))) {
				status.perror("SetZTranslationOnDagPath on cylinder");
				return status;
			}

			/*if (!(status = Helix_getCylinderCaps(cylinder, topCap, bottomCap))) {
				status.perror("Helix_getCylinderCaps");
				return status;
			}

			std::cerr << __FUNCTION__ << ": SetZTranslationOnDagPath bottom cap: " << std::endl;

			if (!(SetZTranslationOnDagPath(bottomCap, origo))) {
				status.perror("SetZTranslationOnDagPath on bottom cap");
				return status;
			}

			std::cerr << __FUNCTION__ << ": SetZTranslationOnDagPath top cap: " << std::endl;

			if (!(SetZTranslationOnDagPath(topCap, origo + height))) {
				status.perror("SetZTranslationOnDagPath on top cap");
				return status;
			}*/


			/*
			 * Find the makeNurbCylinder object
			 *

			MObject polyCylinder;

			if (!(status = Helix_getPolyCylinder(cylinder, polyCylinder))) {
				status.perror("Helix_getMakeNurbCylinder");
				return status;
			}

			/*
			 * Ok we have the makeNurbCylinder object. Is has an attribute called heightRatio that we're looking for
			 *

			MFnDependencyNode makeNurbCylinder_dependencyNode(polyCylinder);

			MObject heightAttribute = makeNurbCylinder_dependencyNode.attribute("height", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute 1");
				return status;
			}

			/*std::cerr << __FUNCTION__ << ": radius: " << std::endl;

			MObject radiusAttribute = makeNurbCylinder_dependencyNode.attribute("radius", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute 2");
				return status;
			}*

			MPlug heightPlug(polyCylinder, heightAttribute);//, radiusPlug(makeNurbCylinder, radiusAttribute);
			/*double radius;

			std::cerr << __FUNCTION__ << ": get radius: " << std::endl;

			if (!(status = radiusPlug.getValue(radius))) {
				status.perror("MPlug::getValue");
				return status;
			}*/

			/*
			 * Ok update the height
			 *

			std::cerr << __FUNCTION__ << ": set height: " << std::endl;

			if (!(status = heightPlug.setValue(height))) {
				status.perror("MPlug::setValue");
				return status;
			}

			std::cerr << __FUNCTION__ << ": done: " << std::endl;

			// FIXME: Update the translations of the bottom and top cap

			return MStatus::kSuccess;*/
		}

		MStatus Helix::getCylinderRange(double & origo, double & height) {
			MStatus status;
			MDagPath helix = getDagPath(status), cylinder;

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			unsigned int numShapes;

			if (!(status = helix.numberOfShapesDirectlyBelow(numShapes))) {
				status.perror("MDagPath::numberOfShapesDirectlyBelow");
				return status;
			}


			for(unsigned int i = 0; i < numShapes; ++i) {
				MDagPath helix_shape = helix;

				if (!(status = helix_shape.extendToShapeDirectlyBelow(i))) {
					status.perror("MDagPath::extendToShapeDirectlyBelow");
					return status;
				}

				if (MFnDagNode(helix_shape).typeId(&status) == View::HelixShape::id) {
					MObject helix_shape_object = helix_shape.node(&status);

					if (!status) {
						status.perror("MDagPath::node");
						return status;
					}

					MPlug origoPlug(helix_shape_object, View::HelixShape::aOrigo), heightPlug(helix_shape_object, View::HelixShape::aHeight);

					origo = origoPlug.asFloat();
					height = heightPlug.asFloat();

					return MStatus::kSuccess;
				}
				else if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}
			}

			return MStatus::kNotFound;

			/*if (!(status = Helix_getCylinder(helix, cylinder))) {
				status.perror("Helix_getCylinder");
				return status;
			}

			/*
			 * Find the translation of the cylinder
			 *

			MVector translation;
			MFnTransform cylinder_transform(cylinder);

			translation = cylinder_transform.getTranslation(MSpace::kTransform, &status);

			if (!status) {
				status.perror("MFnTransform::getTranslation");
				return status;
			}

			/*
			 * Find the makeNurbCylinder object
			 *

			MObject polyCylinder;

			if (!(status = Helix_getPolyCylinder(cylinder, polyCylinder))) {
				status.perror("Helix_getMakeNurbCylinder");
				return status;
			}

			/*
			 * Ok we have the makeNurbCylinder object. Is has an attribute called heightRatio that we're looking for
			 *

			MFnDependencyNode polyCylinder_dependencyNode(polyCylinder);

			MObject heightAttribute = polyCylinder_dependencyNode.attribute("height", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute 1");
				return status;
			}

			/*MObject radiusAttribute = makeNurbCylinder_dependencyNode.attribute("radius", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute 2");
				return status;
			}*

			MPlug heightPlug(polyCylinder, heightAttribute);//, radiusPlug(makeNurbCylinder, radiusAttribute);
			//double height, radius;

			if (!(status = heightPlug.getValue(height))) {
				status.perror("MPlug::getValue 1");
				return status;
			}

			/*if (!(status = radiusPlug.getValue(radius))) {
				status.perror("MPlug::getValue 2");
				return status;
			}*/

			/*
			 * Ok we have everything we need, calculate the requested values
			 *

			origo = translation.z;
			//height = heightRatio * radius;

			return MStatus::kSuccess;*/
		}

		/*MStatus Helix::getCylinderRange(double & origo, double & height) {
			MStatus status;
			MDagPath helix = getDagPath(status), cylinder;

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			if (!(status = Helix_getCylinder(helix, cylinder))) {
				status.perror("Helix_getCylinder");
				return status;
			}

			 *
			 * Find the translation of the cylinder
			 *

			MVector translation;
			MFnTransform cylinder_transform(cylinder);

			translation = cylinder_transform.getTranslation(MSpace::kTransform, &status);

			if (!status) {
				status.perror("MFnTransform::getTranslation");
				return status;
			}

			 *
			 * Find the makeNurbCylinder object
			 *

			MObject makeNurbCylinder;

			if (!(status = Helix_getMakeNurbCylinder(cylinder, makeNurbCylinder))) {
				status.perror("Helix_getMakeNurbCylinder");
				return status;
			}

			 *
			 * Ok we have the makeNurbCylinder object. Is has an attribute called heightRatio that we're looking for
			 *

			MFnDependencyNode makeNurbCylinder_dependencyNode(makeNurbCylinder);

			MObject heightRatioAttribute = makeNurbCylinder_dependencyNode.attribute("heightRatio", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute 1");
				return status;
			}

			MObject radiusAttribute = makeNurbCylinder_dependencyNode.attribute("radius", &status);

			if (!status) {
				status.perror("MFnDependencyNode::attribute 2");
				return status;
			}

			MPlug heightRatioPlug(makeNurbCylinder, heightRatioAttribute), radiusPlug(makeNurbCylinder, radiusAttribute);
			double heightRatio, radius;

			if (!(status = heightRatioPlug.getValue(heightRatio))) {
				status.perror("MPlug::getValue 1");
				return status;
			}

			if (!(status = radiusPlug.getValue(radius))) {
				status.perror("MPlug::getValue 2");
				return status;
			}

			 *
			 * Ok we have everything we need, calculate the requested values
			 *

			origo = translation.z;
			height = heightRatio * radius;

			return MStatus::kSuccess;
		}*/

		/*bool Helix::hasCylinder(MStatus & status) {
			 *
			 * Find a cylinder that is child to this helix with the name given in CYLINDERREPRESENTATION_NAME
			 *

			return true;

			// REMOVE THIS

			MDagPath thisDagPath = getDagPath(status), cylinder;

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			status = Helix_getCylinder(thisDagPath, cylinder);

			if (status == MStatus::kNotFound)
				return false;

			if (!status) {
				status.perror("Helix_getCylinder");
				return false;
			}

			bool isValid = cylinder.isValid(&status);

			if (!status) {
				status.perror("MDagPath::isValid");
				return status;
			}

			return isValid;
		}*/

		/*MStatus Helix::createCylinder(double origo, double height) {
			MStatus status;

			 *
			 * This is still easiest to do using MEL for a lot of reasons
			 *

			MDagPath helix_dagPath = getDagPath(status);

			if (!status) {
				status.perror("Helix::getDagPath");
				return status;
			}

			if (!(status = MGlobal::executeCommand(
					MString("$cylinder = `polyCylinder -radius ") + DNA::RADIUS + " -height " + height + (" -name \"" CYLINDERREPRESENTATION_NAME "\" -axis 0.0 0.0 1.0`;\n"
					"setAttr ($cylinder[0] + \".overrideEnabled\") 1;"
					"setAttr ($cylinder[0] + \".overrideDisplayType\") 2;"
					"$parented_cylinder = `parent -relative $cylinder[0] ") + helix_dagPath.fullPathName() + ("`;\n"
					"move -relative 0.0 0.0 ") + origo + "$parented_cylinder[0]\n"
					))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			/*
			if (!(status = MGlobal::executeCommand(
					MString("$cylinder = `cylinder -radius ") + DNA::RADIUS + " -heightRatio " + (height / DNA::RADIUS) + (" -name \"" CYLINDERREPRESENTATION_NAME "\" -axis 0.0 0.0 1.0`;\n"
					"$topCap = `planarSrf -name \"" CYLINDERREPRESENTATION_NAME "_topCap\" -ch false ($cylinder[0] + \".u[0]\")`;\n"
					"$bottomCap = `planarSrf -name \"" CYLINDERREPRESENTATION_NAME "_bottomCap\" -ch false ($cylinder[0] + \".u[") + height + ("]\")`;\n"
					"parent -relative $topCap[0] $bottomCap[0] $cylinder[0];\n"
					"setAttr ($cylinder[0] + \".overrideEnabled\") 1;"
					"setAttr ($cylinder[0] + \".overrideDisplayType\") 2;"
					"$parented_cylinder = `parent -relative $cylinder[0] ") + helix_dagPath.fullPathName() + ("`;\n"
					"move -relative 0.0 0.0 ") + origo + "$parented_cylinder[0]\n"
					))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}
			*

			return MStatus::kSuccess;
		}*/

		/*
		 * Helper method for recursively search parents for a helix object
		 */

		bool SearchParentsForType(MObject & object, Object & result, MTypeId & typeId) {
			MFnDagNode dagNode(object);
			MStatus status;

			if (dagNode.typeId(&status) == typeId) {
				result = object;
				return true;
			}

			unsigned int numParents = dagNode.parentCount(&status);

			if (!status) {
				status.perror("MFnDagNode::parentCount");
				return false;
			}

			for(unsigned int i = 0; i < numParents; ++i) {
				MObject parent;

				parent = dagNode.parent(i, &status);

				if (!status) {
					status.perror("MFnDagNode::parent");
					return status;
				}

				if (SearchParentsForType(parent, result, typeId))
					return true;
			}

			return false;
		}

		MStatus Helix::Selected(Object & object, Helix & helix) {
			MStatus status;
			MObject mobject = object.getObject(status);

			if (!status) {
				status.perror("Object::getObject");
				return status;
			}

			return SearchParentsForType(mobject, helix, ::Helix::Helix::id) ? MStatus::kSuccess : MStatus::kFailure;
		}

		MStatus Helix::AllSelected(MObjectArray & selectedHelices) {
			return GetSelectedObjectsOfType(selectedHelices, ::Helix::Helix::id);
		}

		/*
		 * Helper functor for finding and adding relatives
		 */

		class GetRelativesFunctor {
		public:
			inline GetRelativesFunctor(MObjectArray & relatives) : m_relatives(relatives) {

			}

			void operator() (Helix helix) {
				if (find_nonconst(&m_relatives[0], &m_relatives[0] + m_relatives.length(), helix) == &m_relatives[0] + m_relatives.length()) {
					MObject object = helix.getObject(m_status);
				
					if (!m_status) {
						m_status.perror("Helix::getObject");
						return;
					}

					if (!(m_status = m_relatives.append(object))) {
						m_status.perror("MObjectArray::append");
						return;
					}

					/*
					 * Now recursively do the same for this helix relative
					 */

					MObjectArray helix_relatives;

					if (!(m_status = helix.getRelatives(helix_relatives))) {
						m_status.perror("Helix::getRelatives");
						return;
					}

					for_each_ref(&helix_relatives[0], &helix_relatives[0] + helix_relatives.length(), *this);
				}
			}

			inline MStatus status() const {
				return m_status;
			}

		private:
			MStatus m_status;
			MObjectArray & m_relatives;
		};

		MStatus Helix::GetRelatives(const MObjectArray & helices, MObjectArray & relatives) {
			GetRelativesFunctor functor(relatives);

			for_each_ref(&helices[0], &helices[0] + helices.length(), functor);

			return functor.status();
		}

		/*MStatus Helix::findPrimeEnds(MObjectArray & fivePrimeEnds, MObjectArray & threePrimeEnds, Base::Type filter) {
			MStatus status;
			for(BaseIterator it = begin(); it != end(); ++it) {
				Base::Type type = it->type(status);

				if (!status) {
					status.perror("BaseIterator->type");
					return status;
				}

				if ((unsigned int) type & (unsigned int) filter) {
					if ((unsigned int) type & Base::FIVE_PRIME_END)
						fivePrimeEnds.append(it->getObject(status));
					else if ((unsigned int) type & Base::THREE_PRIME_END)
						threePrimeEnds.append(it->getObject(status));

					if (!status) {
						status.perror("BaseIterator->getObject");
						return status;
					}
				}
			}

			return MStatus::kSuccess;
		}*/

		Helix::BaseIterator Helix::begin() {
			BaseIterator it(*this, -1);
			it.GetNextBaseIndex();

			return it;
		}

		Helix::BaseIterator Helix::end() {
			MStatus status;
			MDagPath this_dagPath = getDagPath(status);

			if (!status) {
				status.perror("Helix::getDagPath failed. If you're in a loop we've got a lock :S");
				return BaseIterator(*this, 1000); // Just some high number to avoid a spin
			}

			unsigned int numChildren = this_dagPath.childCount(&status);

			if (!status) {
				status.perror("MDagPath::childCount failed. If you're in a loop we've got a lock :S");
				return BaseIterator(*this, 1000);
			}

			return BaseIterator(*this, numChildren);
		}

		Base Helix::BaseIterator::getChildBase() {
			MStatus status;
			MDagPath helix_dagPath = m_helix->getDagPath(status);
			MObject child;

			if (!status) {
				status.perror("Helix::getObject");
				return Base();
			}

			child = helix_dagPath.child(m_childIndex, &status);

			if (!status) {
				status.perror("MDagPath::child");
				return Base();
			}

			return Base(child);
		}

		void Helix::BaseIterator::GetNextBaseIndex() {
			MStatus status;
			MDagPath helix_dagPath = m_helix->getDagPath(status);

			if (!status) {
				status.perror("Helix::getDagPath");
				return;
			}

			unsigned int numChildren = helix_dagPath.childCount(&status);

			if (!status) {
				status.perror("MDagPath::childCount");
				return;
			}

			for(++m_childIndex; m_childIndex < numChildren; ++m_childIndex) {
				MObject child = helix_dagPath.child(m_childIndex, &status);

				if (!status) {
					status.perror("MDagPath::child");
					continue;
				}

				MFnDagNode child_dagNode(child);

				if (child_dagNode.typeId(&status) == ::Helix::HelixBase::id)
					return; // We're done, we are currently targeting a valid base.
			}

			// If the code goes through here we will be targeting numChildren, and thus we equal the end() function
		}

		
		MStatus Helix::getRelatives(MObjectArray & helices) {
			MStatus status;

			for(BaseIterator it = begin(); it != end(); ++it) {
				Model::Base backward;

				backward = it->backward(status);

				if (!status && status != MStatus::kNotFound) {
					status.perror("Base::backward");
					return status;
				}

				if (status) {
					Model::Helix parent = backward.getParent(status);

					if (!status) {
						status.perror("Base::getParent");
						return status;
					}

					if (!status) {
						status.perror("Helix::getObject");
						return status;
					}

					if (parent != *this) {
						MObject parent_object = parent.getObject(status);

						if (!status) {
							status.perror("Helix::getObject");
							return status;
						}

						if (std::find(&helices[0], &helices[0] + helices.length(), parent_object) == &helices[0] + helices.length()) {
							helices.append(parent_object);
						
							MObjectArray parent_relatives;
							parent.getRelatives(parent_relatives);

							/*
							 * Copy the ones that arent in 'helices'
							 */

							for(unsigned int i = 0; i < parent_relatives.length(); ++i) {
								if (std::find(&helices[0], &helices[0] + helices.length(), parent_relatives[i]) == &helices[0] + helices.length())
									helices.append(parent_relatives[i]);
							}
						}
					}
				}
			}

			return MStatus::kSuccess;
		}

		MStatus Helix::ToggleCylinderOrBases() {
			MStatus status;

			if (!(status = MGlobal::executeCommand(MEL_TOGGLECYLINDERBASEVIEW_COMMAND " -toggle true"))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			return status;
		}

		MStatus Helix::RefreshCylinderOrBases() {
			MStatus status;

			if (!(status = MGlobal::executeCommand(MEL_TOGGLECYLINDERBASEVIEW_COMMAND " -refresh true"))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			return status;
		}

		MStatus Helix::getSelectedChildBases(MObjectArray & selectedBases) {
			MStatus status;
			MObjectArray allSelectedBases;

			if (!(status = Base::AllSelected(allSelectedBases))) {
				status.perror("Base::AllSelected");
				return status;
			}

			/*
			 * Now sort out the ones that are not children to this helix
			 */

			for(unsigned int i = 0; i < allSelectedBases.length(); ++i) {
				Base base(allSelectedBases[i]);

				Helix parent = base.getParent(status);

				if (!status) {
					status.perror("Base::getParent");
					return status;
				}

				if (parent == *this)
					selectedBases.append(allSelectedBases[i]);
			}

			return MStatus::kSuccess;
		}

		unsigned int Helix::numChildren(MStatus & status) {
			MFnDagNode helix_dagNode(getObject(status));

			if (!status) {
				status.perror("Helix::getObject");
				return 0;
			}

			return helix_dagNode.childCount(&status);
		}
	}
}
