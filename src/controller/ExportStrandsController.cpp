#include <controller/ExportStrands.h>

#include <fstream>
#include <iterator>

namespace Helix {
	namespace Controller {
		char g_separator;

		/*
		 * Makes the export method really clean
		 */

		std::ostream & operator<<(std::ostream & stream, const ExportStrands::Data & data) {
			stream << data.strand_name.asChar() << g_separator << data.sequence.asChar();

			return stream;
		}

		MStatus ExportStrands::write(const MString & filename, ExportStrands::Mode mode) {
			g_separator = mode == COMMA_SEPARATED ? ',' : ';';

			std::ofstream file(filename.asChar(), std::ios_base::out);

			if (!file.good())
				return MStatus::kFailure;

			std::copy(m_export_data.begin(), m_export_data.end(), std::ostream_iterator<ExportStrands::Data>(file, "\n"));

			file.close();

			return MStatus::kSuccess;
		}

		MStatus ExportStrands::doExecute(Model::Strand & element) {
			/*
			 * Figure out if this strand is a loop or not by iterating backwards to the end.
			 * If that is a loop just name it the name of the base, if not use the end bases
			 */
			
			MStatus status;
			Data data;

			std::cerr << "Working on Strand defined by base: " << element.getDefiningBase().getDagPath(status).fullPathName().asChar() << std::endl;

			Model::Strand::BackwardIterator it = element.reverse_begin(), last_it = it;
			for(; it != element.reverse_end(); ++it) {
				last_it = it;
			}

			if (it.loop()) {
				std::cerr << "This strand has a loop" << std::endl;

				MDagPath base_dagPath = element.getDefiningBase().getDagPath(status);

				if (!status) {
					status.perror("Base::getDagPath");
					return status;
				}

				data.strand_name = base_dagPath.fullPathName();
			}

			std::cerr << "After reverse iterating, the first base is: " << last_it->getDagPath(status).fullPathName().asChar() << std::endl;

			/*
			 * Iterate forward and collect all the base labels
			 */

			Model::Strand::ForwardIterator f_it = Model::Strand(*last_it).forward_begin(), last_f_it = f_it;
			for(; f_it != element.forward_end(); ++f_it) {
				DNA::Name label;

				if (!(status = f_it->getLabel(label))) {
					status.perror("Base::getLabel 1");
					return status;
				}

				char char_label = label.toChar();
				data.sequence += MString(&char_label, 1);

				last_f_it = f_it;
			}

			std::cerr << "After forward iteration, f_it references: " << last_f_it->getDagPath(status).fullPathName().asChar() << std::endl;

			if (!f_it.loop()) {
				/*
				 * Since it wasn't a loop, it points to the first element in the strand while f_it should point to the last
				 */

				std::cerr << "Forward isn't a loop either" << std::endl;

				MDagPath first_base_dagPath = last_it->getDagPath(status);

				if (!status) {
					status.perror("Base::getDagPath 1");
					return status;
				}

				MDagPath last_base_dagPath = last_f_it->getDagPath(status);

				if (!status) {
					status.perror("Base::getDagPath 2");
					return status;
				}

				data.strand_name = first_base_dagPath.fullPathName() + " -> " + last_base_dagPath.fullPathName();
			}

			m_export_data.push_back(data);

			onProgressStep();

			return MStatus::kSuccess;
		}

		MStatus ExportStrands::doUndo(Model::Strand & element, Empty & undoData) {
			return MStatus::kSuccess;
		}

		MStatus ExportStrands::doRedo(Model::Strand & element, Empty & redoData) {
			return MStatus::kSuccess;
		}

		void ExportStrands::onProgressStep() {

		}
	}
}
