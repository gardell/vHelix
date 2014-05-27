/*
 * Strand.cpp
 *
 *  Created on: 17 feb 2012
 *      Author: johan
 */

#include <model/Strand.h>

#include <Utility.h>

namespace Helix {
	namespace Model {
		bool Strand::contains_base(Base & base, MStatus & status) {
			ForwardIterator it = forward_begin();
			if (find_itref_nonconst(it, forward_end(), base) != forward_end())
				return true;

			if (it.loop())
				return false;

			return find_nonconst(++reverse_begin(), reverse_end(), base) != reverse_end();
		}

		bool Strand::contains_base(const Base & base, MStatus & status) {
			ForwardIterator it = forward_begin();
			if (find_itref_nonconst(it, forward_end(), base) != forward_end())
				return true;

			if (it.loop())
				return false;

			return find_nonconst(++reverse_begin(), reverse_end(), base) != reverse_end();
		}

		bool Strand::operator==(Strand & strand) {
			MStatus status;
			bool retval;
			HMEVALUATE(retval = contains_base(strand.getDefiningBase(), status), status);
			return retval;
		}

		bool Strand::operator==(const Strand & strand) {
			MStatus status;
			bool retval;
			HMEVALUATE(retval = contains_base(strand.getDefiningBase(), status), status);
			return retval;
		}

		void Strand::rewind() {
			BackwardIterator it = reverse_begin();
			BackwardIterator last_it(it);
			for (; it != reverse_end(); ++it) {
				last_it = it;
			}
			m_base = *last_it;
		}
	}
}
