/*
 * OxDnaImporter.h
 *
 *  Created on: Jan 29, 2014
 *      Author: johan
 */

#ifndef CONTROLLER_OXDNAIMPORTER_H_
#define CONTROLLER_OXDNAIMPORTER_H_

#include <Definition.h>
#include <DNA.h>

#include <model/Helix.h>
#include <model/Base.h>


#include <maya/MString.h>
#include <maya/MVector.h>

namespace Helix {
	namespace Controller {
		class VHELIXAPI OxDnaImporter {
		public:
			MStatus read(const char *topology_filename, const char *configuration_filename, const char *vhelix_filename);

		private:
			struct Base {
				unsigned int strand;
				int forward, backward;

				DNA::Name label;
				MVector translation;
				MString name, helixName;
				Model::Base base;
			};

			struct Helix {
				MVector translation, normal;
				Model::Helix helix;
			};
		};
	}
}


#endif /* CONTROLLER_OXDNAIMPORTER_H_ */
