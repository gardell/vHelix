/*
 * Base.h
 *
 *  Created on: 9 feb 2012
 *      Author: johan
 */

#ifndef _MODEL_BASE_H_
#define _MODEL_BASE_H_

#include <model/Object.h>
#include <model/Material.h>

#include <DNA.h>

#include <maya/MString.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MEulerRotation.h>

namespace Helix {
	namespace Model {
		class Helix;
		/*
		 * Base: Defines the interface for manipulating helix bases
		 */

		class Base : public Object {
		public:
			enum Type {
				BASE = 0,
				FIVE_PRIME_END = 1,
				THREE_PRIME_END = 2,
				END = 3
			};

			static MStatus Create(Helix & helix, const MString & name, const MVector & translation, Base & base);

			DEFINE_DEFAULT_INHERITED_OBJECT_CONSTRUCTORS(Base)

			static MStatus AllSelected(MObjectArray & selectedBases);

			/*
			 * Handle materials (colors) of the base
			 */

			MStatus setMaterial(const Material & material);
			MStatus getMaterial(Material & material);

			/*
			 * Returns one of the enum types above
			 */

			Type type(MStatus & status);

			/*
			 * Connect/disconnect
			 */

			MStatus connect_forward(Base & target);
			MStatus disconnect_forward();
			MStatus disconnect_backward();
			MStatus connect_opposite(Base & target);
			MStatus disconnect_opposite();

			/*
			 * Labels
			 */

			MStatus setLabel(DNA::Names label);
			MStatus getLabel(DNA::Names & label);

			/*
			 * Find the next, prev or opposite bases.
			 * If you want to iterate strands, use the Strand class as it is iterator based and works with STL
			 */

			Base forward(MStatus & status);
			Base forward();

			Base backward(MStatus & status);
			Base backward();

			Base opposite(MStatus & status);
			Base opposite();

			bool opposite_isDestination(MStatus & status);

			/*
			 * Helix
			 */

			Helix getParent(MStatus & status);
		};
	}
}

#endif /* _MODEL_BASE_H_ */
