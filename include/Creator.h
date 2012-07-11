/*
 * Creator.h
 *
 *  Created on: 9 jul 2011
 *      Author: johan
 */

#ifndef CREATOR_H_
#define CREATOR_H_

/*
 * Command for creating new helices
 */

#include <Definition.h>

#include <model/Helix.h>

#include <list>

#include <maya/MPxCommand.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>

#define MEL_CREATEHELIX_COMMAND "createHelix"

namespace Helix {
	class Creator : public MPxCommand {
	public:
		Creator();
		virtual ~Creator();

		virtual MStatus doIt(const MArgList & args);
		virtual MStatus undoIt ();
		virtual MStatus redoIt ();
		virtual bool isUndoable () const;
		virtual bool hasSyntax () const;

		static MSyntax newSyntax ();
		static void *creator();

		static int default_bases; // If no argument is given, uses this. It is initialized to DNA::CREATE_DEFAULT_NUM_BASES on startup

	private:
		/*
		 * The createHelix along CV curve puts requirements on the generated bases
		 * by implementing this class, the caller can accept or reject creation of certain bases
		 */

		class CreateBaseControl {
		public:
			/*
			 * Return true to generate 'shared' coordinates for 'translation' in the operator below
			 * By 'shared' it's a coordinate system above the helices. It is not necessary 'world' coordinates because this would
			 * be inefficient (and there's no Maya method to acquire them), but also since the helices will *always* be in the same coordinate space
			 * but not the bases on each helix, world coordinates are not required
			 * return false is faster
			 */

			virtual bool generateSharedCoordinates() const;

			/*
			 * Return true to accept the creation of the base positioned at 'translation' and at index 'index'. if forward then forward strand, else backward strand
			 */

			virtual bool operator() (const MVector & translation, int index, bool forward) const;
		};

		/*
		 * Before calling, the m_materials must be set
		 */

		//MStatus createHelix(int bases, double rotation, const MVector & origo, Model::Helix & helix, const CreateBaseControl & control = CreateBaseControl());

		MStatus createHelix(int bases, Model::Helix & helix, const MMatrix & transform = MMatrix::identity, const CreateBaseControl & control = CreateBaseControl());

		inline MStatus createHelix(int bases, const MMatrix & transform = MMatrix::identity, const CreateBaseControl & control = CreateBaseControl()) {
			Model::Helix helix;

			return createHelix(bases, helix, transform, control);
		}

		/*
		 * Create a helix along the line between the two given points
		 * either calculate the number of bases to generate and round to the nearest integer
		 * or give them explicitly.
		 * If no rotation is given, the helix will be oriented with the first base along the Y-axis.
		 * Notice that the rotation is given in degrees
		 */

		inline MStatus createHelix(const MVector & origo, const MVector & end, double rotation = 0.0, const CreateBaseControl & control = CreateBaseControl()) {
			Model::Helix helix;
			return createHelix(origo, end, rotation, helix, control);
		}

		MStatus createHelix(const MVector & origo, const MVector & end, double rotation, Model::Helix & helix, const CreateBaseControl & control = CreateBaseControl());

		inline MStatus createHelix(const MVector & origo, const MVector & direction, int bases, double rotation = 0.0, const CreateBaseControl & control = CreateBaseControl()) {
			Model::Helix helix;
			return createHelix(origo, direction, bases, rotation, helix, control);
		}

		MStatus createHelix(const MVector & origo, const MVector & direction, int bases, double rotation, Model::Helix & helix, const CreateBaseControl & control = CreateBaseControl());

		/*
		 * Create a number of helices along a CV curve. Notice that the curve's bending properties (bezier etc) are not taken into account
		 * The rotation is applied to the first helix, the following helices will be rotated to match the end rotation of the previous one
		 */

		MStatus createHelix(const MPointArray & points, double rotation = 0.0);

		/*
		 * These are for the redo/undo functionality
		 */

		enum CreateMode {
			CREATE_NONE = 0,
			CREATE_NORMAL = 1,
			CREATE_BETWEEN = 2,
			CREATE_ORIENTED = 3,
			CREATE_ALONG_CURVE = 4
		} m_redoMode;

		struct {
			int bases;
			MVector origo, end, direction;
			double rotation;
			std::vector<MPointArray> points;
		} m_redoData;

		std::list<Model::Helix> m_helices;
		Model::Material m_materials[2];

		MStatus randomizeMaterials();
	};
}


#endif /* CREATOR_H_ */
