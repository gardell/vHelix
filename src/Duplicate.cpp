/*
 * Duplicate.cpp
 *
 *  Created on: Aug 3, 2011
 *      Author: bjorn
 */

#include <Duplicate.h>
#include <Utility.h>
#include <Helix.h>
#include <HelixBase.h>
#include <ToggleCylinderBaseView.h>
#include <Locator.h>
#include <DNA.h>

#include <maya/MSelectionList.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MFnTransform.h>
#include <maya/MDGModifier.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>
#include <maya/MDagModifier.h>
#include <maya/MProgressWindow.h>
#include <maya/MDagPathArray.h>

namespace Helix {
	Duplicate::Duplicate() {

	}

	Duplicate::~Duplicate() {

	}

	MStatus Duplicate::doIt(const MArgList & args) {
		// Find out our targets. First either by -target or selection
		//

		MStatus status;
		MArgDatabase argDatabase(syntax(), args, &status);

		m_target_helices.clear();

		if (!status) {
			status.perror("MArgDatabase::#ctor");
			return status;
		}

		if (argDatabase.isFlagSet("-t", &status)) {
			MString target_str;

			if (!(status = argDatabase.getFlagArgument("-t", 0, target_str))) {
				status.perror("MArgDatabase::getFlagArgument");
				return status;
			}

			MSelectionList selectionList;
			if (!(status = selectionList.add(target_str))) {
				status.perror("MSelectionList::add");
				return status;
			}

			MObject target;

			if (!(status = selectionList.getDependNode(0, target))) {
				status.perror("MSelectionList::getDependNode");
				return status;
			}

			if (!(status = m_target_helices.append(target))) {
				status.perror("MObjectArray::append");
				return status;
			}
		}

		if (m_target_helices.length() == 0) {
			// Find out by select instead

			if (!(status = SelectedHelices(m_target_helices))) {
				status.perror("SelectedHelices");
				return status;
			}

			if (m_target_helices.length() == 0) {
				// If there's still no selected helices, duplicate the whole scene

				MItDag dagIt(MItDag::kBreadthFirst, MFn::kTransform, &status);

				for(; !dagIt.isDone(); dagIt.next()) {
					MObject node = dagIt.currentItem(&status);

					if (!status) {
						status.perror("MItDag::currentItem");
						return status;
					}

					MFnDagNode node_dagNode(node);

					if (node_dagNode.typeId(&status) == Helix::id)
						m_target_helices.append(node);
				}
			}
		}

		// Now find out, if there's any other connected helices connecting to one of ours. In that case, we duplicate them too
		//

		for(unsigned int i = 0; i < m_target_helices.length(); ++i) {
			MObjectArray relatives;

			if (!(status = Helix_Relatives(m_target_helices[i], relatives))) {
				status.perror("HelixRelatives");
				return status;
			}

			if (relatives.length() > 0) {
				m_target_helices.setSizeIncrement(relatives.length());

				for(unsigned int j = 0; j < relatives.length(); ++j) {
					// Make sure the relative doesn't already exist
					bool exists = false;
					for(unsigned int k = 0; k < m_target_helices.length(); ++k) {
						if (m_target_helices[k] == relatives[j]) {
							exists = true;
							break;
						}
					}

					if (!exists)
						m_target_helices.append(relatives[j]);
				}
			}
		}

		return duplicate(m_target_helices);
	}

	MStatus Duplicate::undoIt () {
		// Delete all the duplicates helices in m_duplicated_helices

		/*MStatus status;
		MString command("delete");

		std::cerr << "undo duplicate" << std::endl;

		for(size_t i = 0; i < m_duplicated_helices.length(); ++i) {
			command += MString(" ") + MFnDagNode(m_duplicated_helices[i]).fullPathName(&status);

			if (!status) {
				status.perror("MFnDagNode::fullPathName");
				return status;
			}
		}

		std::cerr << "undo command: " << command.asChar() << std::endl;

		if (!(status = MGlobal::executeCommand(command))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}*/

		MGlobal::displayError("Undo " MEL_DUPLICATEHELICES_COMMAND " is not available yet. Delete the helices manually");

		return MStatus::kSuccess;
	}

	MStatus Duplicate::redoIt () {
		return duplicate(m_target_helices);
	}

	bool Duplicate::isUndoable () const {
		return true;
	}

	bool Duplicate::hasSyntax () const {
		return true;
	}

	MSyntax Duplicate::newSyntax () {
		MSyntax syntax;

		syntax.addFlag("-t", "-target", MSyntax::kString);

		return syntax;
	}

	void *Duplicate::creator() {
		return new Duplicate();
	}

	// Dummy wrapper class

	class Connection {
	public:
		//MObject source, sourceAttr, destination, destinationAttr;
		MObject targets[2], attributes[2]; // 0 = source, 1 = destination
		//bool translate_source, translate_destination;
		bool translate[2];

		inline Connection(MObject _source = MObject::kNullObj, bool _translate_source = false, MObject & _sourceAttr = MObject::kNullObj, MObject _destination = MObject::kNullObj, bool _translate_destination = false, MObject & _destinationAttr = MObject::kNullObj) {
			targets[0] = _source;
			targets[1] = _destination;

			attributes[0] = _sourceAttr;
			attributes[1] = _destinationAttr;

			translate[0] = _translate_source;
			translate[1] = _translate_destination;
		}
	};

	MStatus Duplicate::duplicate(const MObjectArray & targets) {
		MStatus status;

		m_duplicated_helices.clear();

		// Duplicate all the helices in `targets`, create a translation table for their children (translates the old base to the new base for upcoming connections)
		//

		std::vector<std::pair<MObject, MObject> > translation; // translation is from old -> new

		// Add all connections to be made here, if translate_* is set, the translation array above will be used to translate a given object node here to a new one
		// Used because some of the target nodes does not exist yet when adding to `connections`
		std::vector<Connection> connections;

		// Setup the progress window, currently only increases for each helix
		//

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("Duplicate helices");
		MProgressWindow::setProgressStatus("Generating new helix bases...");
		MProgressWindow::setProgressRange(0, targets.length());
		MProgressWindow::startProgress();


		for(unsigned int i = 0; i < targets.length(); ++i) {
			// Duplicate this helix
			//

			MFnDagNode new_helix_dagNode, helix_dagNode(targets[i]);
			MObject helix_object = new_helix_dagNode.create(Helix::id, MObject::kNullObj, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			// Do a setAttr m_helix.displayHandle true equivalent
			//

			{
				MPlug displayHandle(helix_object, MPxTransform::displayHandle);

				if (!(status = displayHandle.setBool(true)))
					status.perror("MPlug::setBool");
			}

			MTransformationMatrix transformationMatrix = MFnTransform(targets[i]).transformation(&status);

			if (!status) {
				status.perror("MFnTransform::transformation");
				return status;
			}

			if (!(status = MFnTransform(helix_object).set(transformationMatrix))) {
				status.perror("MFnTransform::set");
				return status;
			}

			// Create a LocatorNode that will show UI information about the helix

			MFnDagNode locator_dagNode;
			MObject locator_object = locator_dagNode.create(HelixLocator::id, helix_object, &status);

			if (!status) {
				status.perror("MFnDagNode::create");
				return status;
			}

			// Iterate over its bases

			unsigned int numChildren = helix_dagNode.childCount(&status);

			if (!status) {
				status.perror("MFnDagNode::childCount");
				return status;
			}

			for(unsigned int j = 0; j < numChildren; ++j) {
				MObject child_object = helix_dagNode.child(j, &status);
				MFnDagNode child_dagNode(child_object);

				if (!status) {
					status.perror("MFnDagNode::child");
					return status;
				}

				if (child_dagNode.typeId(&status) == HelixBase::id) {
					// This is a base, copy it

					MFnTransform child_transform(child_object);
					MVector child_translation = child_transform.getTranslation(MSpace::kTransform, &status);
					MObject new_child;

					if (!status) {
						status.perror("MFnTransform::getTranslation");
						return status;
					}

					if (!(status = Helix_CreateBase(helix_object, child_dagNode.name(), child_translation, new_child))) {
						status.perror("Helix_CreateBase");
						return status;
					}

					translation.push_back(std::make_pair(child_object, new_child));

					// Get the base this child is connected to

					MDagPath target;

					if (HelixBase_NextBase(child_object, HelixBase::aForward, target, &status))
						connections.push_back(Connection(target.node(&status), true, HelixBase::aBackward, new_child, false, HelixBase::aForward));

					if (!status) {
						status.perror("HelixBase_NextBase 1");
						return status;
					}

					// Get the helix_forward connection, works the same as above

					/*if (HelixBase_NextBase(child_object, HelixBase::aHelix_Forward, target, &status))
						connections.push_back(Connection(target.node(&status), true, HelixBase::aHelix_Backward, new_child, false, HelixBase::aHelix_Forward));

					if (!status) {
						status.perror("HelixBase_NextBase 2");
						return status;
					}*/

					// Get the pair connection

					if (HelixBase_NextBase(child_object, HelixBase::aLabel, target, &status)) {
						MPlug labelPlug(child_object, HelixBase::aLabel);

						bool isSource = labelPlug.isSource(&status);

						if (!status) {
							status.perror("MPlug::isSource");
							return status;
						}

						if (isSource)
							//labels.push_back(std::make_pair(child_object, target.node(&status)));
							connections.push_back(Connection(new_child, false, HelixBase::aLabel, target.node(&status), true, HelixBase::aLabel));
						else
							//labels.push_back(std::make_pair(target.node(&status), child_object));
							connections.push_back(Connection(target.node(&status), true, HelixBase::aLabel, new_child, false, HelixBase::aLabel));
					}

					if (!status) {
						status.perror("HelixBase_NextBase 3");
						return status;
					}

					// Get the currently assigned material of the base and apply it to the new base
					//

					MString material;

					if (!(status = DNA::QueryMaterialOfObject(child_object, material))) {
						status.perror("DNA::QueryMaterialOfObject");
						return status;
					}

					//std::cerr << "Got material " << material << " out of base " << child_dagNode.fullPathName().asChar() << std::endl;

					MDagPathArray baseToAssignMaterial;
					baseToAssignMaterial.setLength(1);

					if (!(status = MFnDagNode(new_child).getPath(baseToAssignMaterial[0]))) {
						status.perror("MFnDagNode::getPath");
						return status;
					}

					if (!(status = DNA::AssignMaterialToObjects(baseToAssignMaterial, material))) {
						status.perror("DNA::AssignMaterialToObjects");
						return status;
					}

					//m_duplicated_helices.append(new_child);
				}
				else if (strstr(child_dagNode.name().asChar(), "cylinder") != NULL) {
					// Can't find a way to return multiple results from MEL scripts, so we have to call MEL twice

					double radius, heightRatio;

					{
						MCommandResult commandResult;

						if (!(status = MGlobal::executeCommand(MString("cylinder -q -radius ") + child_dagNode.fullPathName(), commandResult))) {
							status.perror("MGlobal::executeCommand 1");
							return status;
						}

						if (!(status = commandResult.getResult(radius))) {
							status.perror("MCommandResult::getResult 1");
							return status;
						}
					}

					{
						MCommandResult commandResult;

						if (!(status = MGlobal::executeCommand(MString("cylinder -q -heightRatio ") + child_dagNode.fullPathName(), commandResult))) {
							status.perror("MGlobal::executeCommand 2");
							return status;
						}

						if (!(status = commandResult.getResult(heightRatio))) {
							status.perror("MCommandResult::getResult 2");
							return status;
						}
					}

					MFnTransform cylinder_transform(child_object);

					MTransformationMatrix cylinder_transformationMatrix = cylinder_transform.transformation(&status);

					if (!status) {
						status.perror("MFnTransform::transformation");
						return status;
					}

					// We have all the data we need to create a new cylinder

					MStringArray stringArrayResult;

					{
						MCommandResult commandResult;

						if (!(status = MGlobal::executeCommand(MString("cylinder -radius ") + radius + " -heightRatio " + heightRatio + " -name \"cylinderRepresentation\" -axis 0.0 0.0 1.0", commandResult))) {
							status.perror("MGlobal::executeCommand");
							return status;
						}

						if (!(status = commandResult.getResult(stringArrayResult))) {
							status.perror("MCommandResult::getResult 1");
							return status;
						}
					}

					{
						MCommandResult commandResult;

						// Grab the cylinder so that we can apply the transform and set its parent

						std::cerr << "StringArray res, length: " << stringArrayResult.length() << ", first: " << stringArrayResult[0].asChar() << std::endl;
						std::cerr << "The full path to the cylinder should be something like: " << (stringArrayResult[0]) << std::endl;

						if (!(status = MGlobal::executeCommand(MString("parent ") + stringArrayResult[0] + " " + new_helix_dagNode.fullPathName(), commandResult))) {
							status.perror("MGlobal::executeCommand 2");
							return status;
						}

						MStringArray stringArrayResultParented;

						if (!(status = commandResult.getResult(stringArrayResultParented))) {
							status.perror("MCommandResult::getResult 2");
							return status;
						}

						std::cerr << "Path resolved into " << stringArrayResultParented[0].asChar() << std::endl;

						MSelectionList selectionList;
						MDagPath cylinder_dagPath;
						MObject cylinder_object;

						selectionList.add(stringArrayResultParented[0]);
						if (!(status = selectionList.getDagPath(0, cylinder_dagPath, cylinder_object))) {
							status.perror("MSelectionList::getDagPath");
							return status;
						}

						MFnTransform new_cylinder_transform(cylinder_dagPath);

						if (!(status = new_cylinder_transform.set(cylinder_transformationMatrix))) {
							status.perror("MFnTransform::set");
							return status;
						}
					}
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}
			}

			m_duplicated_helices.append(helix_object);

			MProgressWindow::advanceProgress(1);
		}

		// In this step, create all the connections to the new bases

		MProgressWindow::setProgressStatus("Setting up bindings");
		MProgressWindow::setProgress(0);
		MProgressWindow::setProgressRange(0, int(connections.size()));

		MDGModifier dgModifier;

		for (std::vector<Connection>::iterator it = connections.begin(); it != connections.end(); ++it) {
			MObject new_targets[2] = { MObject::kNullObj, MObject::kNullObj };

			for(size_t i = 0; i < 2; ++i) {
				if (it->translate[i]) {
					for (std::vector<std::pair<MObject, MObject> >::iterator tit = translation.begin(); tit != translation.end(); ++tit) {
						if (tit->first == it->targets[i]) {
							new_targets[i] = tit->second;
							break;
						}
					}

					if (new_targets[i] == MObject::kNullObj) {
						std::cerr << "Translation failed, can't make connection" << std::endl;
						return MStatus::kFailure;
					}
				}
				else
					new_targets[i] = it->targets[i];
			}

			// Make the actual connection

			if (!(status = dgModifier.connect(new_targets[0], it->attributes[0], new_targets[1], it->attributes[1]))) {
				status.perror("MDGModifier::connect");
				return status;
			}

			MProgressWindow::advanceProgress(1);
		}

		if (!(status = dgModifier.doIt())) {
			status.perror("MDGModifier::doIt");
			return status;
		}

		if (!(status = MGlobal::executeCommand(MEL_TOGGLECYLINDERBASEVIEW_COMMAND " -refresh true"))) {
			status.perror("MGlobal::executeCommand toggleCylinderBaseView");
			return status;
		}

		MProgressWindow::endProgress();

		// Select the helices
		//

		// DOESN'T WORK?!?!

		/*MSelectionList selectionList;

		for (unsigned int i = 0; i < m_duplicated_helices.length(); ++i) {
			std::cerr << "adding helix to selectionlist at " << i << std::endl;
			if (!(status = selectionList.add(m_duplicated_helices[i]))) {
				status.perror("MSelectionList::add");
				return status;
			}
		}

		if (!(status = MGlobal::setActiveSelectionList(selectionList, MGlobal::kReplaceList))) {
			status.perror("MGlobal::setActiveSelectionList");
			return status;
		}*/

		return MStatus::kSuccess;
	}
}
