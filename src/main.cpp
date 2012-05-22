/*
 * main.cpp
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#include <DNA.h>

#include <Locator.h>
//#include <Tracker.h>

#include <Helix.h>
#include <HelixBase.h>

#include <Creator.h>
#include <CreatorGui.h>
#include <Connect.h>
#include <Disconnect.h>
#include <Duplicate.h>
#include <FindFivePrimeEnds.h>
#include <PaintStrand.h>
#include <ApplySequence.h>
#include <ApplySequenceGui.h>
#include <ExtendHelix.h>
#include <ExtendGui.h>
#include <ToggleCylinderBaseView.h>
#include <ToggleLocatorRender.h>
#include <ExportStrands.h>
#include <JSONTranslator.h>
#include <RetargetBase.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MFnDagNode.h>
#include <maya/MPxTransform.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MCommandResult.h>
#include <maya/MPlugArray.h>
#include <maya/MProgressWindow.h>

#include <ctime>

#define REGISTER_OPERATIONS	\
	/*new RegisterCommand(MEL_ONSELECTIONCHANGED_COMMAND_NATIVE, Helix::HelixTracker::creator, NULL),*/													\
	/*new RegisterCommand(MEL_JSONIMPORT_COMMAND, Helix::JSONImport::creator, Helix::JSONImport::newSyntax),*/											\
	new RegisterCommand(MEL_CREATEHELIX_COMMAND, Helix::Creator::creator, Helix::Creator::newSyntax),													\
	new RegisterCommand(MEL_CREATEHELIX_GUI_COMMAND, Helix::CreatorGui::creator),																		\
	new RegisterCommand(MEL_CONNECTBASES_COMMAND, Helix::Connect::creator, Helix::Connect::newSyntax),													\
	new RegisterCommand(MEL_DISCONNECTBASE_COMMAND, Helix::Disconnect::creator, Helix::Disconnect::newSyntax),											\
	new RegisterCommand(MEL_DUPLICATEHELICES_COMMAND, Helix::Duplicate::creator, Helix::Duplicate::newSyntax),											\
	new RegisterCommand(MEL_FINDFIVEPRIMEENDS_COMMAND, Helix::FindFivePrimeEnds::creator, Helix::FindFivePrimeEnds::newSyntax),							\
	new RegisterCommand(MEL_PAINTSTRAND_COMMAND, Helix::PaintStrand::creator, Helix::PaintStrand::newSyntax),											\
	new RegisterCommand(MEL_APPLYSEQUENCE_COMMAND, Helix::ApplySequence::creator, Helix::ApplySequence::newSyntax),										\
	new RegisterCommand(MEL_APPLYSEQUENCE_GUI_COMMAND, Helix::ApplySequenceGui::creator),																\
	new RegisterCommand(MEL_EXTENDHELIX_COMMAND, Helix::ExtendHelix::creator, Helix::ExtendHelix::newSyntax),											\
	new RegisterCommand(MEL_EXTENDHELIX_GUI_COMMAND, Helix::ExtendGui::creator),																		\
	new RegisterCommand(MEL_TOGGLECYLINDERBASEVIEW_COMMAND, Helix::ToggleCylinderBaseView::creator, Helix::ToggleCylinderBaseView::newSyntax),			\
	new RegisterCommand(MEL_TOGGLELOCATORRENDER_COMMAND, Helix::ToggleLocatorRender::creator, Helix::ToggleLocatorRender::newSyntax),					\
	new RegisterCommand(MEL_EXPORTSTRANDS_COMMAND, Helix::ExportStrands::creator, Helix::ExportStrands::newSyntax),										\
	new RegisterCommand(MEL_RETARGETBASE_COMMAND, Helix::RetargetBase::creator, Helix::RetargetBase::newSyntax),										\
	new RegisterNode("HelixLocator", Helix::HelixLocator::id, &Helix::HelixLocator::creator, &Helix::HelixLocator::initialize, MPxNode::kLocatorNode),	\
	new RegisterTransform(HELIX_HELIXBASE_NAME, Helix::HelixBase::id, Helix::HelixBase::creator, Helix::HelixBase::initialize, MPxTransformationMatrix::creator, MPxTransformationMatrix::baseTransformationMatrixId.id()),	\
	new RegisterTransform(HELIX_HELIX_NAME, Helix::Helix::id, Helix::Helix::creator, Helix::Helix::initialize, MPxTransformationMatrix::creator, MPxTransformationMatrix::baseTransformationMatrixId.id()),					\
	new RegisterFileTranslator(HELIX_CADNANO_JSON_FILE_TYPE, Helix::JSONTranslator::creator)

#define MEL_REGISTER_MENU_COMMAND													\
    "menu -tearOff true -label \"Helix\" -allowOptionBoxes true -parent $gMainWindow;\n"

#define MEL_DEREGISTER_MENU_COMMAND													\
    "menu -deleteAllItems"

#ifdef WIN32
#define MLL_EXPORT __declspec (dllexport)
#else
#define MLL_EXPORT
#endif /* WIN32 */

// MOVE THESE
//#define MEL_CREATEHELIX_GUI_COMMAND MEL_CREATEHELIX_COMMAND "_gui"
//#define MEL_APPLYSEQUENCE_GUI_COMMAND MEL_APPLYSEQUENCE_COMMAND "_gui"
//#define MEL_EXTENDHELIX_GUI_COMMAND MEL_EXTENDHELIX_COMMAND "_gui"

#define MENU_ITEMS	\
{	"Create helix", "Create a new helix", MEL_CREATEHELIX_COMMAND, MEL_CREATEHELIX_GUI_COMMAND, false, false, false, -1, ACCEL_CTRL | ACCEL_ALT, 'n' },	\
{	"Duplicate helices", "Duplicate all or the selected helices", MEL_DUPLICATEHELICES_COMMAND, "", false, false, false, -1, ACCEL_CTRL | ACCEL_ALT, 'd' },	\
{	"-", "", ";", "", true, false, false, -1, ACCEL_NONE },	\
{	"Connect bases", "Connect the two selected bases", MEL_CONNECTBASES_COMMAND, "", false, false, false, -1, ACCEL_CTRL | ACCEL_ALT, 'c' },	\
{	"Disconnect bases", "Disconnect the selected base", MEL_DISCONNECTBASE_COMMAND, "", false, false, false, -1, ACCEL_CTRL | ACCEL_ALT, 'v' },	\
{	"Extend strand", "Extend the selected strands", MEL_EXTENDHELIX_GUI_COMMAND, "", false, false, false, -1, ACCEL_NONE },	\
{	"-", "", ";", "", true, false, false, -1, ACCEL_NONE },	\
{	"Find 5' ends", "Find all 5' ends on selected strands", MEL_FINDFIVEPRIMEENDS_COMMAND, "", false, false, false, -1, ACCEL_NONE },	\
{	"Paint strand", "Paint the strand of the currently selected base with a random color", MEL_PAINTSTRAND_COMMAND, "", false, false, false, -1, ACCEL_CTRL | ACCEL_ALT, 'p' },	\
{	"-", "", ";", "", true, false, false, -1, ACCEL_NONE },	\
{	"Apply sequence", "Apply a given sequence string to the currently selected strand and calculate connected staple sequences", MEL_APPLYSEQUENCE_GUI_COMMAND, "", false, false, false, -1, ACCEL_NONE },	\
{	"Export strands", "Export all or selected bases strands to Excel or a text file", MEL_EXPORTSTRANDS_COMMAND, "", false, false, false, -1, ACCEL_NONE },	\
{	"-", "", ";", "", true, false, false, -1, ACCEL_NONE },	\
{	"Toggle cylinder or bases view", "Show the cylinder or base representation of the helices", MEL_TOGGLECYLINDERBASEVIEW_COMMAND " -toggle true", "", false, false, false, -1, ACCEL_CTRL | ACCEL_ALT, 't' },	\
{	"Render", "Toggle rendering of some of the HUD elements", "", "", false, true, false, -1, ACCEL_NONE },	\
{	"Halo", "Display the colored halos around bases", MEL_TOGGLELOCATORRENDER_COMMAND " -toggle \\\"halo\\\"", "", false, false, false, 1, ACCEL_NONE },	\
{	"Pair lines", "Display the base pair connected lines", MEL_TOGGLELOCATORRENDER_COMMAND " -toggle \\\"pair_line\\\"", "", false, false, false, 1, ACCEL_NONE },	\
{	"Sequence", "Display the sequence labels above bases", MEL_TOGGLELOCATORRENDER_COMMAND " -toggle \\\"sequence\\\"", "", false, false, false, 1, ACCEL_NONE },	\
{	"Directional arrow", "Display the helix directional arrow", MEL_TOGGLELOCATORRENDER_COMMAND " -toggle \\\"directional_arrow\\\"", "", false, false, false, 1, ACCEL_NONE },	\
{	"-", "", ";", "", false, false, true, -1, ACCEL_NONE }

/*
 * I wanted to make a way to register commands that are easier to monitor (for having a progress bar etc)
 * This is an register operation, registerCommand, registerNode or registerTransform etc
 */

class Register {
public:
	virtual MStatus doRegister(MFnPlugin & plugin) {
		return MStatus::kFailure;
	}

	virtual MStatus doDeregister(MFnPlugin & plugin) {
		return MStatus::kFailure;
	}

	virtual bool isValid() const {
		return true;
	}
};

class NullRegister : public Register {
public:
	virtual bool isValid() const {
		return false;
	}
};

class RegisterCommand : public Register {
public:
	inline RegisterCommand(const char *command, void *(*creator)(), MSyntax (*newSyntax)() = NULL) : m_command(command), m_creator(creator), m_newSyntax(newSyntax) {

	}

	MStatus doRegister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.registerCommand(m_command, m_creator, m_newSyntax))) {
			status.perror(MString("registerCommand: ") + m_command);
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus doDeregister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.deregisterCommand(m_command))) {
			status.perror(MString("deregisterCommand: ") + m_command);
			return status;
		}

		return MStatus::kSuccess;
	}

private:
	MString m_command;
	void *(*m_creator)();
	MSyntax (*m_newSyntax)();
};

class RegisterNode : public Register {
public:
	inline RegisterNode(const char *node, const MTypeId & typeId, void *(*creator)(), MStatus (*initialize)(), MPxNode::Type type) : m_node(node), m_typeId(typeId), m_creator(creator), m_initialize(initialize), m_type(type) {

	}

	MStatus doRegister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.registerNode(m_node, m_typeId, m_creator, m_initialize, m_type))) {
			status.perror(MString("registerNode: ") + m_node);
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus doDeregister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.deregisterNode(m_typeId))) {
			status.perror(MString("deregisterNode: ") + m_node);
			return status;
		}

		return MStatus::kSuccess;
	}

private:
	MString m_node;
	MTypeId m_typeId;
	void *(*m_creator)();
	MStatus (*m_initialize)();
	MPxNode::Type m_type;
};

class RegisterTransform : public Register {
public:
	inline RegisterTransform(const char *node, const MTypeId & id, void *(*creator)(), MStatus (*initialize)(), MPxTransformationMatrix *(*xformCreator)(), const MTypeId & xformId) : m_node(node), m_id(id), m_creator(creator), m_initialize(initialize), m_xformCreator(xformCreator), m_xformId(xformId) {

	}

	MStatus doRegister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.registerTransform(m_node, m_id, m_creator, m_initialize, m_xformCreator, m_xformId))) {
			status.perror(MString("MFnPlugin::registerTransform: ") + m_node);
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus doDeregister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.deregisterNode(m_id))) {
			status.perror(MString("MFnPlugin::deregisterNode: ") + m_node);
			return status;
		}

		return MStatus::kSuccess;
	}

private:
	MString m_node;
	MTypeId m_id;
	void *(*m_creator)();
	MStatus (*m_initialize)();
	MPxTransformationMatrix *(*m_xformCreator)();
	MTypeId m_xformId;
};

class RegisterFileTranslator : public Register {
public:
	inline RegisterFileTranslator(const char *filetype, void *(*creator)()) : m_filetype(filetype), m_creator(creator) {

	}

	MStatus doRegister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.registerFileTranslator(m_filetype, (char *) "", m_creator))) {
			status.perror(MString("MFnPlugin::registerFileTranslator: ") + m_filetype);
			return status;
		}

		return MStatus::kSuccess;
	}

	MStatus doDeregister(MFnPlugin & plugin) {
		MStatus status;

		if (!(status = plugin.deregisterFileTranslator(m_filetype))) {
			status.perror(MString("MFnPlugin::deregisterFileTranslator: ") + m_filetype);
			return status;
		}

		return MStatus::kSuccess;
	}

private:
	MString m_filetype;
	void *(*m_creator)();
};

MString g_menuName;

enum {
	ACCEL_NONE = 0,
	ACCEL_CTRL = 1,
	ACCEL_ALT = 2,
	ACCEL_SHIFT = 4
};

MLL_EXPORT MStatus initializePlugin(MObject obj) {
	MStatus status;

	// Data for initialization
	//

	static Register *register_operations[] = { REGISTER_OPERATIONS, new NullRegister() };

	static struct {
		MString title, annotation, command, option_command;
		bool divider, subMenu, closeSubMenu;
		int checkbox;
		unsigned int accel;
		char key;
	} menuItems[] = { MENU_ITEMS, { "", "", "", "", false, false, false, false } };

	// FIXME: In the future, make this a define instead
	int total_operations = 0;
	for(size_t i = 0; register_operations[i]->isValid(); ++i)
		total_operations++;

	for(size_t i = 0; menuItems[i].title.length() > 0; ++i)
		total_operations++;

	// Setup the plugin
	//

	MFnPlugin plugin(obj, PLUGIN_VENDOR, PLUGIN_VERSION, "Any");

	// Display a progress bar
	//

	if (!MProgressWindow::reserve())
		MGlobal::displayWarning("Can't reserve the progress bar.");
	MProgressWindow::setTitle("vHelix");
	MProgressWindow::setProgressStatus("Loading the vHelix plugin...");
	MProgressWindow::setInterruptable(false);
	MProgressWindow::startProgress();

	// Seed the random number generator, PaintStrand calls rand
	//

	srand((unsigned int) time(NULL));

	// Load the nodes and commands
	//

	MProgressWindow::setProgressRange(0, total_operations);

	for(size_t i = 0; register_operations[i]->isValid(); ++i) {
		if (!(status = register_operations[i]->doRegister(plugin)))
			return status;

		delete register_operations[i];

		MProgressWindow::advanceProgress(1);
	}

	/*if (!(status = Helix::HelixTracker::registerOnSelectionChangedListener())) {
		status.perror("registerOnSelectionChangedListener");
		return status;
	}*/

	// Create the menus
	//

	// Create the menu and obtain the menu name

	{
		MCommandResult commandResult;
		if (!(status = MGlobal::executeCommand(MEL_REGISTER_MENU_COMMAND, commandResult))) {
			status.perror("MGlobal::executeCommand");
			return status;
		}

		if (!(status = commandResult.getResult(g_menuName))) {
			status.perror("MCommandResult::getResult");
		}
	}

	// Create all the menu items

	// FIXME: Since it looks like we'll be sticking to MEL here, we should buffer the commands into one instead of executing them all separately
	// Can save us a little loading time?

	for(size_t i = 0; menuItems[i].title.length() > 0; ++i) {
		if (menuItems[i].accel != ACCEL_NONE) {
			MGlobal::executeCommand(
					MString("nameCommand -command \"") + menuItems[i].command + "\" -annotation \"" + menuItems[i].command + "\" \"" + "HelixNameCommand_" + double(i) + "\";\n" +
					MString("hotkey -name \"") + "HelixNameCommand_" + double(i) + "\"" + (MString((menuItems[i].accel & ACCEL_ALT) ? " -altModifier" : "") + ((menuItems[i].accel & ACCEL_CTRL) ? " -ctrlModifier" : "") + ((menuItems[i].accel & ACCEL_SHIFT) ? " -shiftModifier true" : "") + " -keyShortcut \"" + MString(&menuItems[i].key, 1) + "\"")
					);
		}
		if (menuItems[i].divider)
			MGlobal::executeCommand("menuItem -divider true");
		else if (menuItems[i].subMenu)
			MGlobal::executeCommand(MString("menuItem -subMenu true -label \"") + menuItems[i].title + "\";\n");
		else if (menuItems[i].closeSubMenu)
			MGlobal::executeCommand("setParent -menu ..;\n");
		else
			MGlobal::executeCommand(MString("menuItem -label \"") + menuItems[i].title + "\"" + (i == 0 ? (" -parent \"" + g_menuName + "\"") : "") + " -command \"" + menuItems[i].command + "\"" +
					(menuItems[i].checkbox != -1 ? (MString(" -checkBox ") + (menuItems[i].checkbox ? "on" : "off") ) : "") +
					(menuItems[i].accel != ACCEL_NONE ? (MString((menuItems[i].accel & ACCEL_ALT) ? " -altModifier true" : "") + ((menuItems[i].accel & ACCEL_CTRL) ? " -ctrlModifier true" : "") + ((menuItems[i].accel & ACCEL_SHIFT) ? " -shiftModifier true" : "") + " -keyEquivalent " + MString(&menuItems[i].key, 1)) : "") + ";\n" +
					(menuItems[i].option_command.length() > 0 ? (MString("menuItem -optionBox true -command \"") + menuItems[i].option_command + "\"") : "") + ";\n");

		// For some reason, spaces are changed into undercores, _ when using this command, that's why we use the MEL command above instead
		//plugin.addMenuItem(menuItems[i].title, menuName, menuItems[i].command, "", menuItems[i].option_command.length() > 0, menuItems[i].option_command.length() > 0 ? &menuItems[i].option_command : NULL, &status);

		if (!status) {
			status.perror("MFnPlugin::addMenuItem");
			return status;
		}

		MProgressWindow::advanceProgress(1);
	}

	MProgressWindow::endProgress();

	return MStatus::kSuccess;
}


MLL_EXPORT MStatus uninitializePlugin(MObject obj)
{
        MStatus status;
        MFnPlugin plugin(obj);

		/*if (!(status = Helix::HelixTracker::deregisterOnSelectionChangedListener())) {
			status.perror("HelixTracker::deregisterOnSelectionChangedListener");
			return status;
		}*/

        static Register *register_operations[] = { REGISTER_OPERATIONS, new NullRegister() };

		for(size_t i = 0; register_operations[i]->isValid(); ++i) {
			if (!(status = register_operations[i]->doRegister(plugin)))
				return status;
		}

		// Remove all the menu items

		/*for(size_t i = 0; menuItems[i].title.length() > 0; ++i) {

			if (!status) {
				status.perror("MFnPlugin::addMenuItem");
				return status;
			}

			MProgressWindow::advanceProgress(1);
		}*/

		MGlobal::executeCommand(MString(MEL_DEREGISTER_MENU_COMMAND " \"") + g_menuName + "\"", false);

		return MStatus::kSuccess;
}
