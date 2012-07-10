#ifndef _JSONIMPORTER_H_
#define _JSONIMPORTER_H_

#include <Definition.h>

#include <model/Base.h>
#include <model/Helix.h>

#include <json/json.h>

#include <map>
#include <vector>
#include <string>
#include <list>

namespace Helix {
	namespace Controller {
		/*
		 * The caDNAno JSON importer rewritten to use the cleaner API and be less buggy
		 */

		class JSONImporter {
		public:
			/*
			 * Parse the json file
			 */

			MStatus parseFile(const char *filename);

		protected:
			/*
			 * Binary representation structures
			 */

			struct Base {
				int connections[4]; // to helix, to base, from helix, from base
				std::vector<Model::Base> bases; // only used by the create method to manage the created bases

				// if [-1, -1, -1, -1] it's no base
				bool isValid() const {
					return connections[0] + connections[1] + connections[2] + connections[3] != -4;
				}

				bool isEndBase() const {
					return connections[0] + connections[1] == -2 || connections[2] + connections[3] == -2;
				}

				bool hasPreviousConnection() const {
					return connections[0] + connections[1] != -2;
				}

				bool hasNextConnection() const {
					return connections[2] + connections[3] != -2;
				}
			};

			struct Helix {
				std::vector<Base> stap, scaf; // the strands
				std::vector<int> loop, skip;
				int col, row; // coordinates for the helix
				std::vector<int> stap_colors;

				Model::Helix helix;
			};

			struct file {
				std::map<int, Helix> helices; // Maps "num" in JSON file to a helix
				std::string name, filename;
			} m_file;
		};
	}
}

#endif /* N _JSONIMPORTER_H_ */
