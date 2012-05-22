/*
 * Utility.cpp
 *
 *  Created on: 10 jul 2011
 *      Author: johan
 */

#define NOMINMAX /* Cause visual c++ breaks the std::numeric_limits */

#include <Utility.h>

#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlugArray.h>
#include <maya/MDGModifier.h>
#include <maya/MDagPathArray.h>
#include <maya/MCommandResult.h>
#include <maya/MFnSet.h>
#include <maya/MDagModifier.h>

#include <HelixBase.h>
#include <Helix.h>
#include <DNA.h>

#include <limits>

namespace Helix {
	/*
	 * Introduced with the new API
	 */

	MStatus GetSelectedObjectsOfType(MObjectArray & objects, MTypeId & type) {
		MStatus status;
		MSelectionList selectionList;

		if (!(status = MGlobal::getActiveSelectionList(selectionList))) {
			status.perror("MGlobal::getActiveSelectionList");
			return status;
		}

		if (!(status = objects.clear())) {
			status.perror("MObjectArray::clear");
			return status;
		}

		unsigned int selectionList_length = selectionList.length(&status);

		if (!status) {
			status.perror("MSelectionList::length");
			return status;
		}

		for(unsigned int i = 0; i < selectionList_length; ++i) {
			//MDagPath dagPath;
			MObject object;

			if (!(status = selectionList.getDependNode(i, object))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			/*if (!(status = GetSelectedObjectsOrParentsOfType(objects, dagPath, type))) {
				status.perror("GetSelectedObjectsOrParentsOfType");
				return status;
			}*/

			MObject relative = GetObjectOrParentsOfType(object, type, status);

			if (!status) {
				status.perror("GetObjectOrParentsOfType");
				return status;
			}

			if (relative != MObject::kNullObj) {
				/*
				 * Make sure it's not already added
				 */

				bool contains = false;
				for(unsigned int i = 0; i < objects.length(); ++i) {
					if (objects[i] == object) {
						contains = true;
						break;
					}
				}

				if (!contains)
					objects.append(relative);
			}
		}

		return MStatus::kSuccess;
	}

	MObject GetObjectOrParentsOfType(MObject & object, MTypeId & type, MStatus & status) {
		MFnDagNode dagNode(object);

		if (dagNode.typeId(&status) == type)
			return object;

		unsigned int numParents = dagNode.parentCount(&status);

		if (!status) {
			status.perror("MFnDagNode::parentCount");
			return MObject::kNullObj;
		}

		for(unsigned int i = 0; i < numParents; ++i) {
			MObject parent;

			parent = dagNode.parent(i, &status);

			if (!status) {
				status.perror("MFnDagNode::parent");
				return MObject::kNullObj;
			}

			MObject relative = GetObjectOrParentsOfType(parent, type, status);

			if (relative != MObject::kNullObj)
				return relative;
		}

		return MObject::kNullObj;
	}

	// Helper method, try to find all the HelixBase's selected, even when a child object is selected (molecule, arrow, etc)
	//

	MStatus SelectedBases(MObjectArray & result) {
		MStatus status;
		MSelectionList selectionList;

		if (!(status = MGlobal::getActiveSelectionList(selectionList))) {
			status.perror("MGlobal::getActiveSelectionList");
			return status;
		}

		if (!(status = result.clear())) {
			status.perror("MDagPathArray::clear");
			return status;
		}

		unsigned int selectionList_length = selectionList.length(&status);

		if (!status) {
			status.perror("MSelectionList::length");
			return status;
		}

		for(unsigned int i = 0; i < selectionList_length; ++i) {
			MObject depNode;
			if (!(status = selectionList.getDependNode(i, depNode))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			MFnDagNode dagNode(depNode);

			if (dagNode.typeId(&status) == HelixBase::id)
				result.append(depNode);
			else {
				// This is not a HelixBase, but it's parent might be?

				unsigned int parentCount = dagNode.parentCount(&status);

				if (!status) {
					status.perror("MFnDagNode::parentCount");
					continue;
				}

				for(unsigned int j = 0; j < parentCount; ++j) {
					MObject parent = dagNode.parent(j, &status);

					if (!status) {
						status.perror("MFnDagNode::parent");
						break;
					}

					MFnDagNode parent_dagNode(parent);

					if (parent_dagNode.typeId(&status) == HelixBase::id) {
						if (!(status = result.append(parent))) {
							status.perror("MObjectArray::append");
							return status;
						}
					}
				}
			}
		}

		return MStatus::kSuccess;
	}

	MStatus SelectedStrands(MObjectArray & strands, MObjectArray & bases) {
		MStatus status;

		if (!(status = SelectedBases(bases))) {
			status.perror("SelectedBases");
			return status;
		}

		for(unsigned int i = 0; i < bases.length(); ++i) {

			MFnDagNode dagNode(bases[i]);
			MDagPath dagPath;

			if (!(status = dagNode.getPath(dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			static const MObject directions[] = { HelixBase::aForward, HelixBase::aBackward };

			for(size_t j = 0; j < 2; ++j) {
				MObject it_object;
				MDagPath it_dagPath = dagPath;

				bool firstIt = true; // Ugly, but the easiest way to solve the problem with loops

				do {
					it_object = it_dagPath.node(&status);

					if (!status) {
						status.perror("MDagPathArray[i]::node");
						return status;
					}

					// Ok here's a potential neighbour object, if it is already in the list, we must BREAK the search as there might be connection loops

					if (!firstIt) {
						bool alreadyExists = false;

						for (unsigned int k = 0; k < strands.length(); ++k) {
							if (strands[k] == it_object) {
								alreadyExists = true;
								break;
							}
						}

						if (alreadyExists)
							break;
					}
					else
						firstIt = false;

					if (!(status = strands.append(it_object))) {
						status.perror("MDagPathArray::append");
						return status;
					}
				}
				while (HelixBase_NextBase(it_object, directions[j], it_dagPath, &status));

				if (!status) {
					status.perror("HelixBase_NextBase");
					return status;
				}
			}
		}

		return MStatus::kSuccess;
	}

	MStatus SelectedHelices(MObjectArray & result) {
		MStatus status;
		MSelectionList selectionList;

		if (!(status = MGlobal::getActiveSelectionList(selectionList))) {
			status.perror("MGlobal::getActiveSelectionList");
			return status;
		}

		if (!(status = result.clear())) {
			status.perror("MDagPathArray::clear");
			return status;
		}

		unsigned int selectionList_length = selectionList.length(&status);

		if (!status) {
			status.perror("MSelectionList::length");
			return status;
		}

		for(unsigned int i = 0; i < selectionList_length; ++i) {
			MObject depNode;
			if (!(status = selectionList.getDependNode(i, depNode))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			MFnDagNode dagNode(depNode);

			if (dagNode.typeId(&status) == Helix::id) {
				std::cerr << "Found base named: " << dagNode.name() << std::endl;

				result.append(depNode);
			}
			else {
				// This is not a HelixBase, but it's parent might be?

				MFnDagNode parent_dagNode = dagNode;

				for(unsigned int j = 0; j < 2; ++j) {
					unsigned int parentCount = parent_dagNode.parentCount(&status);

					if (!status) {
						status.perror("MFnDagNode::parentCount");
						continue;
					}

					bool foundValidParent = false;

					for(unsigned int k = 0; k < parentCount; ++k) {
						MObject parent_object = parent_dagNode.parent(j, &status);

						if (!status) {
							status.perror("MFnDagNode::parent");
							break;
						}

						MFnDagNode parent_parent_dagNode(parent_object);

						if (parent_parent_dagNode.typeId(&status) == Helix::id) {
							std::cerr << "Found parent base named: " << parent_parent_dagNode.name() << std::endl;

							if (!(status = result.append(parent_object))) {
								status.perror("MObjectArray::append");
								return status;
							}

							foundValidParent = true;
						}
					}

					if (foundValidParent)
						break;
				}

				/*size_t parentCount = dagNode.parentCount(&status);

				if (!status) {
					status.perror("MFnDagNode::parentCount");
					continue;
				}

				for(size_t j = 0; j < parentCount; ++j) {
					MObject parent = dagNode.parent(j, &status);

					if (!status) {
						status.perror("MFnDagNode::parent");
						break;
					}

					MFnDagNode parent_dagNode(parent);

					if (parent_dagNode.typeId(&status) == HelixBase::id) {
						std::cerr << "Found parent base named: " << parent_dagNode.name() << std::endl;

						if (!(status = result.append(parent))) {
							status.perror("MObjectArray::append");
							return status;
						}
					}
				}*/
			}
		}

		return MStatus::kSuccess;
	}

	// Helper method, disconnects all plugs with nodes of type HelixBase connected to the given plug
	//

	MStatus DisconnectAllHelixBaseConnections(MPlug targetPlug, bool isSource, MObject *old_target) {
		MStatus status;

		if (targetPlug.isConnected(&status)) {
			MPlugArray plugArray;
			MDGModifier dgModifier;

			if (targetPlug.connectedTo(plugArray, true, true, &status)) {
				unsigned int plugArray_length = plugArray.length();

				//std::cerr << MFnDagNode(targetPlug.node()).fullPathName().asChar() << " is connected to an array of " << plugArray_length << " elements" << std::endl;

				for (unsigned int i = 0; i < plugArray_length; ++i) {
					MObject connected_target = plugArray[i].node(&status);

					if (!status) {
						status.perror("MPlugArray[i]::node");
						return status;
					}

					MFnDagNode connected_target_dagNode(connected_target);

					if (connected_target_dagNode.typeId(&status) == HelixBase::id) {
						// This is a connected HelixBase, it must be disconnected
						//

						//std::cerr << "disconnectAttr " << MFnDagNode(targetPlug.node()).fullPathName().asChar() << " " << MFnDagNode(plugArray[i].node()).fullPathName().asChar() << std::endl;

						if (isSource) {
							if (!(status = dgModifier.disconnect(targetPlug, plugArray[i]))) {
								status.perror("MDGModifier::disconnect");
								return status;
							}
						}
						else {
							if (!(status = dgModifier.disconnect(plugArray[i], targetPlug))) {
								status.perror("MDGModifier::disconnect");
								return status;
							}
						}

						if (old_target)
							*old_target = connected_target;
					}
				}
			}

			if (!status) {
				status.perror("MPlug::connectedTo");
				return status;
			}

			if (!(status = dgModifier.doIt())) {
				status.perror("MDGModifier::doIt @@");
				return status;
			}
		}

		if (!status) {
			status.perror("MPlug::isConnected");
			return status;
		}

		return MStatus::kSuccess;
	}

	// Helper method, returns true if the bases attribute has a HelixBase connected
	//

	bool HelixBase_NextBase(const MObject & base, const MObject & attribute, MDagPath & result, MStatus *retStatus) {
		MStatus status;

		MPlug plug(base, attribute);
		MPlugArray plugArray;

		if (plug.connectedTo(plugArray, true, true, &status)) {
			unsigned int plugArray_length = plugArray.length();

			for(unsigned int i = 0; i < plugArray_length; ++i) {
				MObject target = plugArray[i].node(&status);

				if (!status) {
					status.perror("MPlugArray[i]::node");

					if (retStatus)
						*retStatus = status;
					return false;
				}

				MFnDagNode dagNode(target);

				if (dagNode.typeId(&status) == HelixBase::id) {
					if (!(status = dagNode.getPath(result))) {
						status.perror("MFnDagNode::getPath");

						if (retStatus)
							*retStatus = status;
						return false;
					}

					return true;
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");

					if (retStatus)
						*retStatus = status;
					return false;
				}
			}
		}

		if (!status) {
			status.perror("MPlug::connectedTo");

			if (retStatus)
				*retStatus = status;
			return false;
		}

		return false;
	}

	template<typename Array, typename Object>
	bool ArrayContainsObject(Array & array, Object & object) {
		size_t length = array.length();

		for (size_t i = 0; i < length; ++i) {
			if (array [i] == object)
				return true;
		}

		return false;
	}

	bool HelixBase_isEnd(MObject & base, MStatus *retStatus) {
		MStatus status;

		MPlug plugs[] = { MPlug(base, HelixBase::aForward), MPlug(base, HelixBase::aBackward) };

		for(size_t i = 0; i < 2; ++i) {
			bool isConnected = plugs[i].isConnected(&status);

			if (!status) {
				status.perror("MPlug::isConnected");

				if (retStatus)
					*retStatus = status;
				return false;
			}

			if (!isConnected)
				return true;

			// It can still be an end, if it's not connected to any HelixBases

			MPlugArray plugArray;
			bool connected = false;

			if (plugs[i].connectedTo(plugArray, true, true, &status)) {
				for(unsigned int j = 0; j < plugArray.length(); ++j) {
					MFnDagNode dagNode(plugArray[j].node(&status));

					if (!status) {
						status.perror("MPlugArray[j]::node");
						if (retStatus)
							*retStatus = status;
						return false;
					}

					if (dagNode.typeId(&status) == HelixBase::id) {
						connected = true;
						break;
					}

					if (!status) {
						status.perror("MFnDagNode::typeId");
						if (retStatus)
							*retStatus = status;
						return false;
					}
				}
			}

			if (!connected)
				return true;

		}

		return false;
	}

	unsigned int HelixBase_endType(MObject & base, MStatus *retStatus) {
		MStatus status;

		MPlug plugs[] = { MPlug(base, HelixBase::aForward), MPlug(base, HelixBase::aBackward) };
		bool plugHasHelix[] = { false, false };

		for(size_t i = 0; i < 2; ++i) {
			bool isConnected = plugs[i].isConnected(&status);

			if (!status) {
				status.perror("MPlug::isConnected");

				*retStatus = status;
				return false;
			}

			// This plug is not connected to anything and can't have a helix, which is the default value, just continue

			if (!isConnected)
				continue;

			// It can still be an end, if it's not connected to any HelixBases

			MPlugArray plugArray;

			if (plugs[i].connectedTo(plugArray, true, true, &status)) {
				for(unsigned int j = 0; j < plugArray.length(); ++j) {
					MFnDagNode dagNode(plugArray[j].node(&status));

					if (!status) {
						status.perror("MPlugArray[j]::node");

						if (retStatus)
							*retStatus = status;
					}

					if (dagNode.typeId(&status) == HelixBase::id) {
						plugHasHelix[i] = true;
						break;
					}

					if (!status) {
						status.perror("MFnDagNode::typeId");

						if (retStatus)
							*retStatus = status;
					}
				}
			}

		}

		if (plugHasHelix[0] && plugHasHelix[1])
			return Neither;
		else if (plugHasHelix[0])
			return FivePrime;
		else if (plugHasHelix[1])
			return ThreePrime;
		else
			return FivePrime | ThreePrime;

		return false;
	}

	MStatus Helix_Strands(MDagPath helix, std::vector<MDagPathArray> & strands) {
		MStatus status;

		MFnDagNode helix_dagNode(helix);
		unsigned int childCount = helix_dagNode.childCount(&status);

		if (!status) {
			status.perror("MFnDagNode::childCount");
			return status;
		}

		for(unsigned int i = 0; i < childCount; ++i) {
			MObject child = helix_dagNode.child(i, &status);

			if (!status) {
				status.perror("MFnDagNode::child");
				return status;
			}

			MFnDagNode child_dagNode(child);

			if (child_dagNode.typeId(&status) == HelixBase::id) {
				// Found a HelixBase belonging to this Helix, find out if it belongs to a strand

				MDagPath child_dagPath;

				if (!(status = child_dagNode.getPath(child_dagPath))) {
					status.perror("MFnDagNode::getPath");
					return status;
				}

				bool alreadyExists = false;

				for(std::vector<MDagPathArray>::iterator it = strands.begin(); it != strands.end(); ++it) {
					for(unsigned int j = 0; j < it->length(); ++j) {
						if ((*it)[j].node(&status) == child) {
							alreadyExists = true;
							break;
						}
					}

					if (alreadyExists)
						break;
				}

				if (!alreadyExists) {
					// The base is part of a new strand. Find the whole strand and append it to strands

					MDagPathArray new_strand;

					if (!(status = Base_Strands(child_dagPath, new_strand))) {
						status.perror("Base_Strands");
						return status;
					}

					strands.push_back(new_strand);
				}
			}
		}

		return MStatus::kSuccess;
	}

	MStatus Base_Strands(MDagPath base, MDagPathArray & neighbours) {
		MStatus status;

		MObject it_object, base_object = base.node(&status);
		MDagPath it_dagPath = base;

		if (!status) {
			status.perror("MDagPath::node");
			return status;
		}

		static const MObject directions[] = { HelixBase::aForward, HelixBase::aBackward };

		for(size_t i = 0; i < 2; ++i) {
			bool firstIt = true;

			do {
				it_object = it_dagPath.node(&status);

				if (!status) {
					status.perror("MDagPath::node");
					return status;
				}

				if (firstIt)
					firstIt = false;
				else {
					if (it_object == base_object) {
						// The strand is a loop
						break;
					}
				}

				neighbours.append(it_dagPath);
			}
			while(HelixBase_NextBase(it_object, directions[i], it_dagPath, &status));
		}

		return MStatus::kSuccess;
	}

	//
	// FIXME: The DNA.cpp to load molecules as well as this method need a lot of cleanup that could increase performance when creating bases
	//

	MStatus Helix_CreateBase(MObject & helix, MString name, MVector translation, MObject & result) {
		MStatus status;

		// Create the base
		//

		MFnDagNode base_dagNode;
		result = base_dagNode.create(HelixBase::id, name, helix, &status);

		if (!status) {
			status.perror("MFnDagNode::create");
			return status;
		}

		// Translate the base
		//

		MFnTransform base_transform(result);

		if (!(status = base_transform.setTranslation(translation, MSpace::kTransform))) {
			status.perror("MFnTransform::setTranslation");
			return status;
		}

		// Attach the models
		//

		MDagPath arrowModel, moleculeModel;

		if (!(status = DNA::GetArrowModel(arrowModel))) {
			status.perror("DNA::GetArrowModel");
			return status;
		}

		if (!(status = DNA::GetMoleculeModel(moleculeModel))) {
			status.perror("DNA::GetMoleculeModel");
			return status;
		}

		// DEBUG: Trying to obtain the Shape from the moleculeModel

		//MDagPath moleculeModelShape = moleculeModel;

		if (!(status = moleculeModel.extendToShape())) {
			status.perror("MDagPath::extendToShape 1");
		}

		//std::cerr << "Extended to shape: " << moleculeModel.fullPathName().asChar() << std::endl;

		// The arrow contains multiple shapes so extendToShape doesn't work..

		/*if (!(status = arrowModel.extendToShape())) {
			status.perror("MDagPath::extendToShape 2");
		}*/

		//std::cerr << "Extended to shape: " << moleculeModel.fullPathName().asChar() << std::endl;

		//std::cerr << "Extended to shape: " << arrowModel.fullPathName().asChar() << std::endl;

		/*MObject arrowModelObject = arrowModel.node(&status);

		if (!status) {
			status.perror("MDagPath::node doesn't work on Shapes 1");
		}*/

		MObject moleculeModelObject = moleculeModel.node(&status);

		if (!status) {
			status.perror("MDagPath::node doesn't work on Shapes 2");
		}

		/*if (!(status = base_dagNode.addChild(arrowModelObject, MFnDagNode::kNextPos, true))) {
			status.perror("MFnDagNode::addChild 1");
			return status;
		}*/

		if (!(status = base_dagNode.addChild(moleculeModelObject, MFnDagNode::kNextPos, true))) {
			status.perror("MFnDagNode::addChild 2");
			return status;
		}

		// Now the arrow that contains multiple elements

		/*unsigned int numArrowShapes;

		if (!(status = arrowModel.numberOfShapesDirectlyBelow(numArrowShapes))) {
			status.perror("MDagPath::numberOfShapesDirectlyBelow");
			return status;
		}

		std::cerr << "Number of shapes directly below arrowModel: " << numArrowShapes << std::endl;*/

		// DEBUG 2

		unsigned int numArrowChildren = arrowModel.childCount(&status);

		if (!status) {
			status.perror("MDagPath::childCount");
			return status;
		}

		//std::cerr << "Number of children below arrowModel: " << numArrowChildren << std::endl;

		for(unsigned int i = 0; i < numArrowChildren; ++i) {
			MObject childObject = arrowModel.child(i, &status);

			if (!status) {
				status.perror("MDagPath::child");
				continue;
			}

			MDagPath childDagPath;

			if (!(status = MDagPath::getAPathTo(childObject, childDagPath))) {
				status.perror("MDagPath::getAPathTo");
				continue;
			}

			if (!(status = childDagPath.extendToShape())) {
				status.perror("MDagNode::extendToShape");
				continue;
			}

			MObject childShapeObject = childDagPath.node(&status);

			if (!status) {
				status.perror("MDagPath::node arrow shape");
				continue;
			}

			if (!(status = base_dagNode.addChild(childShapeObject, MFnDagNode::kNextPos, true))) {
				status.perror("MFnDagNode::addChild 2");
				return status;
			}
		}

		// END DEBUG 2

		/*for(unsigned int i = 0; i < numArrowShapes; ++i) {
			MDagPath arrowShapeDagPath = arrowModel;

			if (!(status = arrowShapeDagPath.extendToShapeDirectlyBelow(i))) {
				status.perror("MDagPath::extendToShapeDirectlyBelow");
				continue;
			}

			std::cerr << "Extended to arrow shape " << i << " named: " << arrowShapeDagPath.fullPathName().asChar() << std::endl;

			MObject arrowShapeObject = arrowShapeDagPath.node(&status);

			if (!status) {
				status.perror("MDagPath::node arrowShapeObject");
				continue;
			}

			if (!(status = base_dagNode.addChild(arrowShapeObject, MFnDagNode::kNextPos, true))) {
				status.perror("MFnDagNode::addChild arrowShape");
				return status;
			}
		}*/

		//std::cerr << std::endl;

		// END DEBUG

		/*MFnDagNode arrow_dagNode(arrowModel),
				   molecule_dagNode(moleculeModel);

		MObject arrow_object = arrow_dagNode.duplicate(true, false, &status);

		if (!status) {
			status.perror("MDagNode::duplicate 1");
			return status;
		}

		MObject molecule_object = molecule_dagNode.duplicate(true, false, &status);

		if (!status) {
			status.perror("MDagNode::duplicate 2");
			return status;
		}

		if (!(status = base_dagNode.addChild(arrow_object, MFnDagNode::kNextPos, false))) {
			status.perror("MFnDagNode::addChild 1");
			return status;
		}

		if (!(status = base_dagNode.addChild(molecule_object, MFnDagNode::kNextPos, false))) {
			status.perror("MFnDagNode::addChild 2");
			return status;
		}*/

		return MStatus::kSuccess;
	}

	MStatus Helix_Relatives(const MObject & helix, MObjectArray & result) {
		MStatus status;

		// Get all the children of the helix, find their connected bases and see if any of them are children of other helices
		//

		MFnDagNode helix_dagNode(helix);

		size_t numChildren = helix_dagNode.childCount(&status);

		if (!status) {
			status.perror("MFnDagNode::childCount");
			return status;
		}

		for(unsigned int i = 0; i < numChildren; ++i) {
			MObject child_object = helix_dagNode.child(i, &status);

			if (!status) {
				status.perror("MFnDagNode::child");
				return status;
			}

			MFnDagNode child_dagNode(child_object);

			if (child_dagNode.typeId(&status) == HelixBase::id) {
				// This child is a base, find it's connected bases

				static const MObject directions[] = { HelixBase::aForward, HelixBase::aBackward };

				for (size_t j = 0; j < 2; ++j) {
					MDagPath connected;
					if (HelixBase_NextBase(child_object, directions[j], connected, &status)) {
						MFnDagNode connected_dagNode(connected);

						// Assume only one parent
						MObject parent_object = connected_dagNode.parent(0, &status);

						if (!status) {
							status.perror("MFnDagNode::parent");
							return status;
						}

						MFnDagNode parent_dagNode(parent_object);

						if (parent_dagNode.typeId(&status) == Helix::id) {
							// The parent is a valid helix (which we probably could assume..

							// Make sure it does not already exist in the `result` array, if not, append it

							bool exists = false;

							for(unsigned int k = 0; k < result.length(); ++k) {
								if (result[k] == parent_object) {
									exists = true;
									break;
								}
							}

							if (!exists)
								result.append(parent_object);
						}
					}

					if (!status) {
						status.perror("HelixBase_NextBase");
						return status;
					}
				}
			}
		}

		return MStatus::kSuccess;
	}

	bool HelixBase_isForward(const MObject & base, MStatus *retStatus) {
		MStatus status;

		static const MObject directions[] = { HelixBase::aForward, HelixBase::aBackward };
		static const bool directions_forward[] = { true, false };

		MFnTransform base_transform(base, &status);

		if (!status) {
			status.perror("MFnTransform::#ctotr");

			if (retStatus)
				*retStatus = status;

			return false;
		}

		MVector translation = base_transform.getTranslation(MSpace::kTransform, &status);

		if (!status) {
			status.perror("MFnTransform::getTranslation");

			if (retStatus)
				*retStatus = status;

			return false;
		}

		for(size_t i = 0; i < 2; ++i) {
			MDagPath nextBase;

			if (HelixBase_NextBase(base, directions[i], nextBase, &status)) {
				MFnTransform nextBase_transform(nextBase);

				MVector nextBase_translation = nextBase_transform.getTranslation(MSpace::kTransform, &status);

				if (!status) {
					status.perror("MFnTransform::getTranslation 2");

					if (retStatus)
						*retStatus = status;

					return false;
				}

				return directions_forward[i] ? ((nextBase_translation - translation).z > 0.0) : ((nextBase_translation - translation).z < 0.0);
			}

			if (!status) {
				status.perror("HelixBase_NextBase");

				if (retStatus)
					*retStatus = status;

				return false;
			}
		}

		std::cerr << "Failed to find prev/next base, assuming staple!" << std::endl;

		return false;
	}

	// FIXME: Try to use the STL for sorting instead

	MStatus HelixBases_sort(MObjectArray & input, MObjectArray & result) {
		MStatus status;

		result.setSizeIncrement(input.length());

		while(input.length() > 0) {
			double z = std::numeric_limits<double>::max();
			unsigned int z_index = 0;

			for(unsigned int i = 0; i < input.length(); ++i) {
				MFnTransform transform(input[i]);

				MVector translation = transform.getTranslation(MSpace::kTransform, &status);

				if (!status) {
					status.perror("MFnTransform::getTranslation");
					return status;
				}

				if (translation.z < z) {
					//std::cerr << "replacing " << z << " with " << translation.z << " at index " << z_index << std::endl;
					z = translation.z;
					z_index = i;
				}
			}

			//std::cerr << "chosen z: " << z << ", index: " << z_index << " named: " << MFnDagNode(input[z_index]).fullPathName().asChar() << std::endl;

			result.append(input[z_index]);
			input.remove(z_index);
		}

		return MStatus::kSuccess;
	}

	// Helper method. Note that dagPath can't be a reference cause we need a copy!
	MStatus DagPath_isMemberOfSet(MDagPath dagPath, const MString & set, bool & result) {
		MStatus status;

		unsigned int numberOfShapes;

		if (!(status = dagPath.numberOfShapesDirectlyBelow(numberOfShapes))) {
			status.perror("MDagPath::numberOfShapesDirectlyBelow");
			return status;
		}

		if (numberOfShapes > 0) {
			/*MFnSet set_mfn(set);

			bool isMember = set_mfn.isMember(dagPath.node(), &status);

			if (!status) {
				status.perror("MFnSet::isMember");
				return status;
			}

			if (isMember)
				result = true;*/

			if (!(status = dagPath.extendToShapeDirectlyBelow(0))) {
				status.perror("MDagPath::extendToShapeDirectlyBelow");
				return status;
			}

			MCommandResult commandResult;
			int commandRet;

			if (!(status = MGlobal::executeCommand(MString("sets -isMember ") + set + " " + dagPath.fullPathName(), commandResult))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			if (!(status = commandResult.getResult(commandRet))) {
				status.perror("MCommandResult::getResult");
				return status;
			}

			result = commandRet > 0;
		}
		else {
			result = false;
		}

		return MStatus::kSuccess;
	}

	MStatus HelixBase_isMemberOfSet(MDagPath & base, const MString & set, bool & result) {
		MStatus status;

		if (!(status = DagPath_isMemberOfSet(base, set, result))) {
			status.perror("DagPath_isMemberOfSet");
			return status;
		}

		if (result)
			return MStatus::kSuccess;

		// Try the children

		MFnDagNode base_dagNode(base);

		unsigned int numChildren = base_dagNode.childCount(&status);

		if (!status) {
			status.perror("MFnDagNode::childCount");
			return status;
		}

		for(unsigned int i = 0; i < numChildren; ++i) {
			MDagPath child_dagPath;
			MObject child_object = base_dagNode.child(i, &status);

			if (!status) {
				status.perror("MFnDagNode::child");
				return status;
			}

			MFnDagNode child_dagNode(child_object);

			if (!(status = child_dagNode.getPath(child_dagPath))) {
				status.perror("MFnDagNode::getPath");
				return status;
			}

			if (!(status = DagPath_isMemberOfSet(child_dagPath, set, result))) {
				status.perror("DagPath_isMemberOfSet");
				return status;
			}

			if (result)
				return MStatus::kSuccess;
		}

		return MStatus::kSuccess;
	}

	MStatus HelixBase_RemoveAllAimConstraints(MObject & helixBase, const char *type) {
		MStatus status;

		MFnDagNode this_dagNode(helixBase, &status);

		if (!status) {
			status.perror("MFnDagNode::#ctor");
			return status;
		}

		unsigned int this_childCount = this_dagNode.childCount(&status);
		bool foundAimConstraint = false;
		MDagModifier dagModifier;

		if (!status) {
			status.perror("MFnDagNode::childCount");
			return status;
		}

		for(unsigned int i = 0; i < this_childCount; ++i) {
			MObject child_object = this_dagNode.child(i, &status);

			if (!status) {
				status.perror("MFnDagNode::child");
				return status;
			}

			MFnDagNode child_dagNode(child_object, &status);

			if (!status) {
				status.perror("MFnDagNode::#ctor");
				return status;
			}

			if (child_dagNode.typeName() == "aimConstraint") {
				foundAimConstraint = true;
				//std::cerr << "Found an old aimconstraint to delete named: " << child_dagNode.name().asChar() << std::endl;

				// DEBUG:

				/*if (child_object.isNull()) {
					std::cerr << "The object is null! terminating!" << std::endl;
					return MStatus::kSuccess;
				}*/

				// This is the operation that crashes Maya when creating a "New scene"

				if (!(status = dagModifier.deleteNode(child_object))) {
					status.perror("MDagModifier::deleteNode");
					return status;
				}
			}
		}

		// Execute deletion and reset transformation into translation only

		if (foundAimConstraint) {
			if (!(status = dagModifier.doIt())) {
				status.perror("MDagModifier::doIt");
				return status;
			}

			MFnTransform this_transform(helixBase, &status);

			if (!status) {
				status.perror("MFnTransform::#ctor");
				return status;
			}

			double rotation[] = { 0.0, 0.0, 0.0 };
			if (!(status = this_transform.setRotation(rotation, MTransformationMatrix::kXYZ))) {
				status.perror("MFnTransform::setRotation");
				return status;
			}
		}

		return MStatus::kSuccess;
	}
}
