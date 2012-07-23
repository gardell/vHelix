/*
 * HelixTracker.h
 *
 *  Created on: 20 jun 2011
 *      Author: johan
 */

#ifndef HELIXTRACKER_H_
#define HELIXTRACKER_H_

#include <maya/MPxCommand.h>
#include <maya/MDagPathArray.h>

#define MEL_ONSELECTIONCHANGED_CACHE_COMMAND		 \
		"UpdateHelixLocatorCache"

// Debug:
/*print "selectedBases:\n";

for($obj in $selectedBases) {
    print ($obj + "\n");
}
print "neighbourBases:\n";

for($obj in $neighbourBases) {
    print ($obj + "\n");
}*/

/*#define MEL_REGISTER_ONSELECTIONCHANGED_COMMAND																					\
		"// Because our search is recursive\n"																					\
		"proc appendNeighbours_Helix(string $base, string $starting_base, int $recursive, string $direction, string $neighbourBases[], string $ends[], string $adjacent[]) {\n"		\
		"    // FIXME: If $base equals $starting_base we need to STOP, or a infinite loop will occur on circular strands!\n"	\
		"    // Note, we cant just == because names can be relative, absolute etc\n"											\
		"    if ($base == $starting_base && $recursive > 0) {\n"																\
		"        print \"breaking\\n\";\n"																						\
		"        return;\n"																										\
		"    }\n"																												\
		"    print (\"appending base: \" + $base + \"\\n\");\n"																	\
		"    $neighbourBases[size($neighbourBases)] = $base;\n"																	\
		"	 $adjacents_local = `listConnections ($base + \".label\")`;\n"														\
		"	 $adjacents = `ls -l $adjacents_local`;\n"																			\
		"	 for($obj in $adjacents) {\n"																						\
		"		$adjacent[size($adjacent)] = $obj;\n"																			\
		"	 }\n"																												\
		"    string $connections[];\n"																							\
		"    $connections_local = `listConnections -s true -d false -type HelixBase ($base + $direction)`;\n"					\
		"	 if (size($connections_local) == 0) {\n"																			\
		"		$ends[size($ends)] = $base;\n"																					\
		"	 }\n"																												\
		"	 else {\n"																											\
		"    	$connections = `ls -l $connections_local`;\n"																	\
		"    	for ($connection in $connections) {\n"																			\
		"        print (\"rec call on \" + $connection + \", \" + $recursive + \"\\n\");\n"										\
		"        appendNeighbours_Helix($connection, $starting_base, 1, $direction, $neighbourBases, $ends, $adjacent);\n"		\
		"    	}\n"																											\
		"    }\n"																												\
		"}\n"																													\
		"proc generateSelectedAndNeighbourHelixLists() {\n"																		\
		"	string $neighbourBases[];\n"																						\
		"	string $fivePrimes[];\n"																							\
		"	string $threePrimes[];\n"																							\
		"	string $adjacent[];\n"																								\
		"	$selectedBases = `ls -selection -type HelixBase -l`;\n"																\
		"	for ($obj in $selectedBases) {\n"																					\
		"	    //string $forwardConnected;\n"																					\
		"	    //$forwardConnected = `connectionInfo -sourceFromDestination ($obj + \".forward\")`;\n"							\
		"	    //print $forwardConnected;\n"																					\
		"	    string $connections[];\n"																						\
		"	    $connections_local = `listConnections -s true -d false -type HelixBase ($obj + \".forward\")`;\n"				\
		"	    $connections = `ls -l $connections_local`;\n"																	\
		"	    for ($connection in $connections) {\n"																			\
		"	        print (\"first level\" + $connection + \"\\n\");\n"															\
		"	        appendNeighbours_Helix($connection, $connection, 0, \".forward\", $neighbourBases, $fivePrimes, $adjacent);\n"			\
		"	    }\n"																											\
		"	    $connections_local = `listConnections -s true -d false-type HelixBase ($obj + \".backward\")`;\n"				\
		"	    $connections = `ls -l $connections_local`;\n"																	\
		"	    for ($connection in $connections) {\n"																			\
		"	        appendNeighbours_Helix($connection, $connection, 0, \".backward\", $neighbourBases, $threePrimes, $adjacent);\n"		\
		"	    }\n"																											\
		"	}\n"																												\
		"	" MEL_ONSELECTIONCHANGED_CACHE_COMMAND " $selectedBases $neighbourBases $fivePrimes $threePrimes $adjacent;\n"		\
		"}\n"																													\
		"createNode HelixLocator;\n"																							\
		"scriptJob -e  \"SelectionChanged\" \"generateSelectedAndNeighbourHelixLists();\" -permanent"

// New native command, let this one replace the old one!

#define MEL_ONSELECTIONCHANGED_COMMAND_NATIVE	"GenerateHelixLocatorCache"

#define MEL_REGISTER_ONSELECTIONCHANGED_COMMAND_NATIVE																			\
		"$helixLocatorNode = `createNode HelixLocator`;\n"																		\
		"int $helixTracker_scriptJob = `scriptJob -e  \"SelectionChanged\" \"" MEL_ONSELECTIONCHANGED_COMMAND_NATIVE "\"`;"
#define MEL_DEREGISTER_ONSELECTIONCHANGED_COMMAND_NATIVE																		\
		"scriptJob -kill $helixTracker_scriptJob;\n"																			\
		"delete $helixLocatorNode;"

/*
 * HelixTracker command: takes two arguments, a string array of currently selected bases, and an array of currently selected neighbour bases
 * If the currently selected is not a base (cylinder or helix for example), the selected bases will be empty, BUT neighbours will contain
 * the bases belonging to the selected helix or the cylinders helix. Connected inter-helix-bases will also be added!
 *
 * You don't call this method directly, it is done in a MEL script that is triggered as an event
 *

namespace Helix {
	class HelixTracker : public MPxCommand {
	public:

		HelixTracker();
		virtual ~HelixTracker();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;
		
		static void *creator();
		
		// This is the "database" of currently activated helix bases. Will be cleared on every doIt, and filled with the given arguments

		static MDagPathArray s_selectedBases, s_selectedNeighbourBases, s_selectedAdjacentBases, s_selectedFivePrimeBases, s_selectedThreePrimeBases;

		// Not part of MPxCommand

		static MStatus registerOnSelectionChangedListener();
		static MStatus deregisterOnSelectionChangedListener();
		static MStatus recursiveSearchForNeighbourBases(MDagPath dagPath, MObject forward_attribute, MObject backward_attribute, MDagPathArray endBases, bool force = false);
	};
}*/

#endif /* HELIXTRACKER_H_ */
