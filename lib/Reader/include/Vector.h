/*
 * Vector.h
 *
 *  Created on: 9 maj 2012
 *      Author: johan
 */

#ifndef _VHELIX_MA_PARSER_VECTOR_H_
#define _VHELIX_MA_PARSER_VECTOR_H_

#include <cmath>
#include <ostream>

namespace Helix {
	/*
	 * A simple 3 component vector class
	 */

	template<typename T>
	class VectorT {
	public:
		T x, y, z;

		inline VectorT(T _x = T(0), T _y = T(0), T _z = T(0)) : x(_x), y(_y), z(_z) {

		}

		inline VectorT(const VectorT<T> & copy) : x(copy.x), y(copy.y), z(copy.z) {

		}

		inline VectorT<T> & operator=(const VectorT<T> & copy) {
			x = copy.x;
			y = copy.y;
			z = copy.z;

			return *this;
		}

		inline T dot(const VectorT & v) {
			return x * v.x + y * v.y + z * v.z;
		}

		inline T length() const {
			return sqrt(dot(*this));
		}
	};

	typedef VectorT<double> Vector;
}

template<typename T>
std::ostream & operator<< (std::ostream & stream, const Helix::VectorT<T> & vector) {
	stream << vector.x << ", " << vector.y << ", " << vector.z;
	return stream;
}

#endif /* _VHELIX_MA_PARSER_VECTOR_H_ */
