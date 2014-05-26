#include <controller/StrandLengthCount.h>

namespace Helix {
	namespace Controller {
		unsigned int StrandLengthCount::length(Model::Strand & strand) {
			// First rewind if the strand is not a loop.
			Model::Strand::BackwardIterator it(strand.reverse_begin());
			Model::Base start_base;
			for (; it != strand.reverse_end(); ++it)
				start_base = *it;

			Model::Strand new_strand(start_base);
			unsigned int length(0);
			for (Model::Strand::ForwardIterator it(new_strand.forward_begin()); it != new_strand.forward_end(); ++it, ++length);

			HPRINT("Return length: %u", length);
			return length;
		}
	}
}
