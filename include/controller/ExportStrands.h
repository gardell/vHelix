#ifndef _CONTROLLER_EXPORTSTRANDS_H_
#define _CONTROLLER_EXPORTSTRANDS_H_

#include <controller/Operation.h>

#include <model/Strand.h>

namespace Helix {
	namespace Controller {
		class ExportStrands : public Operation<Model::Strand> {
		public:
			enum Mode {
				COMMA_SEPARATED,
				COLON_SEPARATED
			};

			MStatus write(const MString & filename, Mode mode = COMMA_SEPARATED);

			virtual void onProgressStep();

		protected:
			MStatus doExecute(Model::Strand & element);
			MStatus doUndo(Model::Strand & element, Empty & undoData);
			MStatus doRedo(Model::Strand & element, Empty & redoData);

		private:
			/*
			 * The data stored for export
			 */

			struct Data {
				/*inline Data(const MString & _strand_name, const MString & _sequence) : strand_name(_strand_name), sequence(_sequence) {

				}*/

				MString strand_name, // Will be the two end bases for a strand not in a loop, if it's a loop, then any base name
						sequence;
			};

			std::list<Data> m_export_data;

			friend std::ostream & operator<<(std::ostream & stream, const ExportStrands::Data & data);
		};
	}
}

#endif /* N  _CONTROLLER_EXPORTSTRANDS_H_ */