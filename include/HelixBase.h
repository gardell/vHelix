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

		virtual void postConstructor();

		MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
		MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

		// Standard utility metods
		//

		static void *creator();
		static MStatus initialize();

		static	MTypeId	id;

		static MObject aForward, aBackward, aLabel, aColor;
	};

	/*
	 * Solves a bug regarding aimConstraints, see HelixBase.h/.cpp
	 */
	bool HelixBase_AllowedToRetargetBase(const MObject & base);
}


#endif /* HELIXBASE_H_ */
