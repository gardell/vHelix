/*
 * HelixBase.h
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#ifndef HELIXBASE_H_
#define HELIXBASE_H_

#include <Definition.h>

#include <iostream>
#include <maya/MPxTransform.h>
#include <maya/MNodeMessage.h>

#define HELIX_HELIXBASE_NAME "HelixBase"
#define HELIX_HELIXBASE_ID 0x02114111

namespace Helix {
	class HelixBase : public MPxTransform {
		friend void MNodeMessage_preRemovalCallbackFunc(MObject & node, void *clientData);
		friend void MModelMessage_removedFromModelCallbackFunc(MObject & node, void *clientData);
		friend void MModelMessage_addedToModelCallbackFunc(MObject & node, void *clientData);
	public:
		HelixBase();
		virtual ~HelixBase();

		//virtual void postConstructor();

		/*
		 * Here we listen to connect/disconnect events on the forward and backward connections.
		 * Because there's a lot of issues with what we can do when these are executed, 
		 * they only trigger MUserEventMessages so that further on
		 */

		//MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
		//MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

		static void *creator();
		static MStatus initialize();

		static	MTypeId	id;

		/*
		 * Our bases attributes: forward: connection to next base, backward: connection to previous, label: connection to opposite and also storage for DNA nucleotide (A,T,G,C)
		 */

		static MObject aForward, aBackward, aLabel;

	private:
		

		/*
		 * None of the below should be modified. It is event callback handlers for solving some of the issues with connectionMade/connectionBroken
		 */
	private:
		/*
		 * There's a bug in that we need to handle special events with care, such as deletion of the node, New Scene/Open events with care
		 * these are called by event listeners attached to this model to enable us to distinguish between
		 * deletion, undo deletion etc
		 */
		
		/*
		 * The event is triggered before all connectionBroken events will be triggered when the node is up for deletion
		 */

		//void onRemovePreConnectionBroken();

		/*
		 * The event is triggered after all connectionBroken events are triggered when the node is up for deletion
		 */

		//void onRemovePostConnectionBroken();

		/*
		 * The event is triggered before all connectionMade, both during creation but also after a delete + undo
		 * revived is true if the method is being called after an undo has been made
		 */

		//void onPreConnectionMade(bool revived);

		/*
		 * This is just special cases of the connectionMade/connectionBroken when the source is aForward and target is aBackward
		 */

		//void onForwardConnectionMade(MObject & target);
		//void onForwardConnectionBroken(MObject & target);

		/*
		 * An event triggered when the node connected as forward node to this one is being deleted
		 */

		//void onForwardConnectedNodePreRemoval(MObject & target);

		/*
		 * m_nodeRevived tracks whether the onPreConnectionMade was due to an undo or if it's a first time creation. Do NOT modify or read!
		 */

		//bool m_nodeRevived;

		/*
		 * Can be used to figure out whether our node is set for deletion when the onForward* are executed
		 */

		/*bool m_nodeIsBeingRemoved;

		inline bool isNodeBeingRemoved() const {
			return m_nodeIsBeingRemoved;
			}*/
	};
}


#endif /* HELIXBASE_H_ */
