#include <model/Color.h>

namespace Helix {
	namespace Model {
		Color::ContainerType Color::s_colors;
		static const float colors[] = { AVAILABLE_COLORS, -1.0f, -1.0f, -1.0f };

		Color::ContainerType::ContainerType() {
			/*
			 * Yes not really necessary, but don't want an overallocated array for no use
			 */

			size_t numColors = 0;
			for(int i = 0; colors[i] > -1.0f; i += 3)
				++numColors;
			s_colors.reserve(numColors);

			for(int i = 0; colors[i * 3] > -1.0f; ++i)
				s_colors.push_back(Model::Color(i));
		}

		Color::Iterator Color::AllColors_begin(MStatus & status, Container::size_type & numColors) {
			Iterator begin = AllColors_begin(status);
			numColors = s_colors.size();
			return begin;
		}

		Color::Iterator Color::AllColors_begin(MStatus & status) {
			return s_colors.begin();
		}

		const float *Color::getRGB() const {
			return &colors[index * 3];
		}
	}
}
