#ifndef _CONTROLLER_OXDNAEXPORTER_H_
#define _CONTROLLER_OXDNAEXPORTER_H_

#include <controller/Operation.h>
#include <model/Strand.h>

#include <DNA.h>

#include <list>
#include <vector>

#include <maya/MQuaternion.h>

#if defined(WIN32) || defined(WIN64)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif /* N Windows */

#define HELIX_OXDNA_CONF_FILE_TYPE "conf"
#define HELIX_OXDNA_TOP_FILE_TYPE "top"
#define HELIX_OXDNA_VHELIX_FILE_TYPE "vhelix"

/*
 * OxDnaExporter: Generates .top and .conf with the strands of the scene that can be used together with oxDNA for simulating
 * DNA models. See https://dna.physics.ox.ac.uk for more information.
 *
 * All strands must have been assigned sequences.
 */
namespace Helix {
	namespace Controller {
		class VHELIXAPI OxDnaExporter : public Operation<Model::Strand> {
		public:

			/*
			 * Writes what is currently stored in m_strands. Use the Operation interface to populate it.
			 */
			MStatus write(const char *topology_filename, const char *configuration_filename, const char *vhelix_filename, const MVector & minTranslation, const MVector & maxTranslation) const;

		protected:

			virtual MStatus doExecute(Model::Strand & element);
			MStatus doUndo(Model::Strand & element, Empty & undoData);
			MStatus doRedo(Model::Strand & element, Empty & redoData);

		private:
			/*
			 * For exporting a strand we need its sequence (the labels are the only identifiers used) and whether its a circular strand.
			 *
			 */
			struct Base {
				DNA::Name label;
				MVector translation, normal, tangent; // Tangent is the normalized vector between the helix axis and the base.
				MString name, helixName;
			};

			struct Strand {
				MString name;
				std::vector<Base> strand;
				bool circular;
			};

			std::list<Strand> m_strands;

			struct Helix {
				MVector translation, normal;
			};

			std::tr1::unordered_map<std::string, Helix> m_helices;
		};
	}
}

#endif /* _CONTROLLER_OXDNAEXPORTER_H_ */

