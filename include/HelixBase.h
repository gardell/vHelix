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

#define HELIX_HELIXBASE_NAME "HelixBase"
#define HELIX_HELIXBASE_ID 0x02114111

namespace Helix {
	class HelixBase : public MPxTransform {
		friend void MNode_NodePreRemovalCallbackFunc(MObject & node, void *clientData);
	public:
		HelixBase();
		virtual ~HelixBase();

		virtual	void postConstructor();

		MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
		MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

		// Standard utility metods
		//

		static void *creator();
		static MStatus initialize();

		static	MTypeId	id;

		static MObject aForward, aBackward, aLabel;
	};
}


#endif /* HELIXBASE_H_ */
