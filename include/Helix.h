/*
 * Helix.h
 *
 *  Created on: 5 jul 2011
 *      Author: johan
 */

#ifndef HELIX_H_
#define HELIX_H_

#include <Definition.h>

#include <iostream>
#include <maya/MPxTransform.h>

#define HELIX_HELIX_NAME "vHelix"
#define HELIX_HELIX_ID 0x02114114

namespace Helix {
	class Helix : public MPxTransform {
	public:
		Helix();
		virtual ~Helix();

		virtual	void postConstructor();

		// Standard utility metods
		//

		static void *creator();
		static MStatus initialize();

		static	MTypeId	id;

		static MObject aLeftStrand, aRightStrand;
	};
}


#endif /* HELIX_H_ */
