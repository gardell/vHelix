#ifndef _MODEL_COLOR_H_
#define _MODEL_COLOR_H_

/*
 * Color: Replaces the old Material objects for representing materials assigned to bases. The color interface is much simpler and only defines
 * an RGB-value used together with the shader
 */

#include <Definition.h>

#include <maya/MStatus.h>

#include <limits>
#include <vector>

#define AVAILABLE_COLORS					\
	0, 1, 0.45666122f,						\
	1, 0.18333334f, 0,						\
	1, 1, 1,								\
	0.75f, 0.75f, 0.75f,					\
	0, 0, 1,								\
	1, 1, 0,								\
	1, 0, 1,								\
	0.20863661f, 0.20863661f, 0.20863661f,	\
	0, 1, 1,								\
	1, 0.12300003f, 0.29839987f

namespace Helix {
	namespace Model {
		class Color {
		public:
			/*
			 * Aliases for fast access
			 */

			int index;

			/*float & r, & g, & b, color[3];

			inline Color(float r_ = 0.0f, float g_ = 0.0f, float b_ = 0.0f) : r(color[0]), g(color[1]), b(color[2]) {
				color[0] = r_;
				color[1] = g_;
				color[2] = b_;
			}

			inline Color(float color_[3]) : r(color[0]), g(color[1]), b(color[2]) {
				for(int i = 0; i < 3; ++i)
					color[i] = color_[i];
			}*/

			inline Color(int index = 0) : index(index) {

			}

			inline Color & operator=(const Color & c) {
				/*for(int i = 0; i < 3; ++i)
					color[i] = c.color[i];*/

				index = c.index;

				return *this;
			}

			inline bool operator==(const Color & c) const {
				/*for(int i = 0; i < 3; ++i) {
					if (fabs(color[i] - c.color[i]) > std::numeric_limits<float>().epsilon())
						return false;
				}*/

				return index == c.index;
			}

			inline bool operator!=(const Color & c) const {
				return !operator==(c);
			}

			const float *getRGB() const;

			typedef std::vector<Color> Container;
			typedef Container::const_iterator Iterator;

			static Iterator AllColors_begin(MStatus & status, Container::size_type & numColors);
			static Iterator AllColors_begin(MStatus & status);
			
			static inline Iterator AllColors_end() {
				return s_colors.end();
			}

		private:
			static class ContainerType : public Container {
			public:
				/* 
				 * So that we can fill the structure
				 */
				ContainerType();
			} s_colors;
		};
	}
}

#endif /* N _MODEL_COLOR_H_ */