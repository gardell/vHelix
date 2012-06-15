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
	}
}
