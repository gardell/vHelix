#include <controller/FillStrandGaps.h>

#include <model/Base.h>
#include <model/Helix.h>
#include <model/Strand.h>

#include <Helix.h>
#include <HelixBase.h>

#include <maya/MProgressWindow.h>

namespace Helix {
	namespace Controller {
		MStatus FillStrandGaps::redo() {
			unsigned int num_added_bases(0);
			MStatus status;

			if (!MProgressWindow::reserve())
				MGlobal::displayWarning("Failed to reserve the progress window");

			MProgressWindow::setTitle("Auto fill strand gaps");
			MProgressWindow::setProgressStatus("Filling strand gaps by linear interpolation...");
			MProgressWindow::setProgressRange(0, int(redoable.size()));
			MProgressWindow::startProgress();

			for (std::vector<Redoable>::iterator it(redoable.begin()); it != redoable.end(); ++it) {

				MString base_name(it->start.getDagPath(status).partialPathName());
				HMEVALUATE_RETURN_DESCRIPTION("Model::Base::getDagPath", status);

				MVector forward_translation, base_translation;
				HMEVALUATE_RETURN(status = it->start.getTranslation(base_translation, MSpace::kWorld), status);
				HMEVALUATE_RETURN(status = it->end.getTranslation(forward_translation, MSpace::kWorld), status);
				MVector direction(forward_translation - base_translation);
				const double length(direction.length());
				int num_additional_bases(int(length / DNA::SINGLE_STRAND_STEP));
				direction.normalize();
				direction *= length / num_additional_bases;
				--num_additional_bases;
				MVector position;
				Model::Base previous(it->start);
				Model::Material material;
				HMEVALUATE_RETURN(status = previous.getMaterial(material), status);

				if (num_additional_bases > 0) {
					previously_connected.push_back(std::make_pair(it->start, it->end));

					for (int i = 0; i < num_additional_bases;) {
						position = base_translation + direction * ++i;
						Model::Base current;
						HMEVALUATE_RETURN(status = Model::Base::Create(it->helix, base_name, position, current, MSpace::kWorld), status);
						undoable.push_back(current);
						HMEVALUATE(status = current.setMaterial(material), status);
						HMEVALUATE_RETURN(status = previous.connect_forward(current), status);
						previous = current;
						++num_added_bases;
					}

					HMEVALUATE_RETURN(status = previous.connect_forward(it->end), status);
				}

				MProgressWindow::advanceProgress(1);
			}

			MProgressWindow::endProgress();

			HPRINT("Added %u bases.", num_added_bases);

			return MStatus::kSuccess;
		}

		MStatus FillStrandGaps::fill_object(const MObject & object) {
			MStatus status;
			MTypeId typeId;
			HMEVALUATE_RETURN(typeId = MFnDagNode(object).typeId(&status), status);

			if (typeId == ::Helix::Helix::id) {
				// Iterate over bases of the helix and look for gaps between its child bases.
				Model::Helix helix(object);
				for (Model::Helix::BaseIterator it(helix.begin()); it != helix.end(); ++it)
					fill_base(*it);
			} else if (typeId == ::Helix::HelixBase::id) {
				// Iterate over bases in the strand and look for gaps between its child bases.
				Model::Strand start_strand(object);
				Model::Base first_base;
				for (Model::Strand::BackwardIterator it(start_strand.reverse_begin()); it != start_strand.reverse_end(); ++it)
					first_base = *it;
				Model::Strand strand(first_base);

				for (Model::Strand::ForwardIterator it(strand.forward_begin()); it != strand.forward_end(); ++it)
					fill_base(*it);
			}

			return MStatus::kSuccess;
		}

		MStatus FillStrandGaps::fill_base(Model::Base & base) {
			MStatus status;
			Model::Helix helix(base.getParent(status));
			HMEVALUATE_RETURN_DESCRIPTION("Model::Base::getParent", status);
			
			Model::Base forward(base.forward(status));

			if (status == MStatus::kNotFound)
				return MStatus::kSuccess;
			HMEVALUATE_RETURN_DESCRIPTION("Model::Base::forward", status);

			redoable.push_back(Redoable(base, forward, helix));

			return MStatus::kSuccess;
		}
	}
}
