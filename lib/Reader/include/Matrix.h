/*
 * Matrix.h
 *
 *  Created on: 18 maj 2012
 *      Author: johan
 */

#ifndef _VHELIX_MA_PARSER_MATRIX_H_
#define _VHELIX_MA_PARSER_MATRIX_H_

#define _USE_MATH_DEFINES

#include <cmath>
#include <ostream>

#ifndef DEG2RAD
#define	DEG2RAD(deg) ((deg) / 180 * M_PI)
#endif /* DEG2RAD */

namespace Helix {
	template<typename T>
	class VectorT;
	/*
	 * A simple 4x4 matrix used to represent the translation and rotation (and possibly scaling too) of nodes,
	 * because helices and groups can be parented, we need a recursive way to obtain the global coordinates,
	 * thus using matrices seemed to be the simplest way
	 * I chose to implement it in column major order
	 */

	template<typename T>
	class Matrix4x4T {
	public:
		static Matrix4x4T<T> Identity() {
			return Matrix4x4T(T(1), T(0), T(0), T(0),
							  T(0), T(1), T(0), T(0),
							  T(0), T(0), T(1), T(0),
							  T(0), T(0), T(0), T(1));
		}
		static Matrix4x4T<T> Zero() {
			return Matrix4x4T(T(0), T(0), T(0), T(0),
							  T(0), T(0), T(0), T(0),
							  T(0), T(0), T(0), T(0),
							  T(0), T(0), T(0), T(0));
		}
		static Matrix4x4T<T> Translate(VectorT<T> t) {
			return Matrix4x4T(T(1), T(0), T(0), t.x,
							  T(0), T(1), T(0), t.y,
							  T(0), T(0), T(1), t.z,
							  T(0), T(0), T(0), T(1));
		}

		/*
		 * Just to keep it simple and avoid any typing errors, i'm implementing the Rotate function
		 * using 3 simpler matrices, could be optimized
		 */

		static Matrix4x4T<T> RotateX(T degree) {
			T sin_theta = T(sin(DEG2RAD(degree))), cos_theta = T(cos(DEG2RAD(degree)));

			return Matrix4x4T<T>(T(1.0), T(0.0), T(0.0), T(0.0),
								 T(0.0), cos_theta, -sin_theta, T(0.0),
								 T(0.0), sin_theta, cos_theta, T(0.0),
								 T(0.0), T(0.0), T(0.0), T(1.0));
		}

		static Matrix4x4T<T> RotateY(T degree) {
			T sin_theta = T(sin(DEG2RAD(degree))), cos_theta = T(cos(DEG2RAD(degree)));

			return Matrix4x4T<T>(cos_theta, T(0.0), sin_theta, T(0.0),
								 T(0.0), T(1.0), T(0.0), T(0.0),
								 -sin_theta, T(0.0), cos_theta, T(0.0),
								 T(0.0), T(0.0), T(0.0), T(1.0)
								 );
		}

		static Matrix4x4T<T> RotateZ(T degree) {
			T sin_theta = T(sin(DEG2RAD(degree))), cos_theta = T(cos(DEG2RAD(degree)));

			return Matrix4x4T<T>(cos_theta, -sin_theta, T(0.0), T(0.0),
								 sin_theta, cos_theta, T(0.0), T(0.0),
								 T(0.0), T(0.0), T(1.0), T(0.0),
								 T(0.0), T(0.0), T(0.0), T(1.0)
								 );
		}

		static Matrix4x4T<T> Rotate(VectorT<T>  r) {
			return RotateZ(r.z) * RotateY(r.y) * RotateX(r.x);
		}

		Matrix4x4T() {

		}

		Matrix4x4T(T _00, T _10, T _20, T _30, T _01, T _11, T _21, T _31, T _02, T _12, T _22, T _32, T _03, T _13, T _23, T _33) {
			matrix[0][0] = _00;
			matrix[1][0] = _10;
			matrix[2][0] = _20;
			matrix[3][0] = _30;

			matrix[0][1] = _01;
			matrix[1][1] = _11;
			matrix[2][1] = _21;
			matrix[3][1] = _31;

			matrix[0][2] = _02;
			matrix[1][2] = _12;
			matrix[2][2] = _22;
			matrix[3][2] = _32;

			matrix[0][3] = _03;
			matrix[1][3] = _13;
			matrix[2][3] = _23;
			matrix[3][3] = _33;
		}

		Matrix4x4T(const Matrix4x4T<T> & copy) {
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x)
					matrix[x][y] = copy.matrix[x][y];
			}
		}

		Matrix4x4T<T> operator*(const Matrix4x4T<T> & m) const {
			Matrix4x4T<T> r = Zero();

			for(int y = 0; y < 4; ++y) {
				for(int x = 0; x < 4; ++x) {
					for(int i = 0; i < 4; ++i)
						r.matrix[y][x] += matrix[i][x] * m.matrix[y][i];
				}
			}

			return r;
		}

		VectorT<T> operator*(const VectorT<T> & v) const {
			return VectorT<T>(matrix[0][0] * v.x + matrix[1][0] * v.y + matrix[2][0] * v.z + matrix[3][0],
							  matrix[0][1] * v.x + matrix[1][1] * v.y + matrix[2][1] * v.z + matrix[3][1],
							  matrix[0][2] * v.x + matrix[1][2] * v.y + matrix[2][2] * v.z + matrix[3][2]);
		}

		/*Matrix4x4T<T> & operator*=(const Matrix4x4T<T> & m) {

		}*/

		Matrix4x4T<T> & operator=(const Matrix4x4T<T> & copy) {
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x)
					matrix[x][y] = copy.matrix[x][y];
			}

			return *this;
		}

		T *operator[](int index) {
			return matrix[index];
		}

		const T *operator[](int index) const {
			return matrix[index];
		}

	private:
		T matrix[4][4];
	};

	typedef Matrix4x4T<double> Matrix4x4;
}

template<typename T>
std::ostream & operator<< (std::ostream & stream, const Helix::Matrix4x4T<T> & matrix) {
	for(int y = 0; y < 4; ++y) {
		for(int x = 0; x < 4; ++x)
			stream << matrix[x][y] << " ";
		stream << std::endl;
	}

	return stream;
}

#endif /* _VHELIX_MA_PARSER_MATRIX_H_ */
