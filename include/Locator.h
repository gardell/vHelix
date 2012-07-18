/*
 * HelixLocator.h
 *
 *  Created on: 9 jun 2011
 *      Author: johan
 */

#ifndef HELIXLOCATOR_H_
#define HELIXLOCATOR_H_

#include <Definition.h>

#include <iostream>

#include <maya/MPxLocatorNode.h>
#include <maya/MTypes.h>
#include <maya/M3dView.h>

/*
 * Because Python wasn't very good at rendering MPxLocatorNodes, i'm doing this extension in C++
 * This extension is as simple as possible, it just figures out what bases are selected, optionally its neighbours and the connected end bases
 * it marks these visually by rendering "halos" around the molecule models with different coloring and radius.
 * Currently, the halos are rendered as point sprites which are very fast and easy to implement.
 * It renders a generated texture of a halo, but this should be changed into a GLSL program for better quality
 */

#define HELIX_LOCATOR_ID 0x02114121
#define HELIX_TRANSFORM_ID 0x02114114
#define PLUGIN_VENDOR "Karolinska Institutet"
#define PLUGIN_VERSION "0.1"

#define HALO_TEXTURE_WIDTH 32
#define HALO_INNER_DIAMETER 28

#define SELECTED_HALO_COLOR { 0xFF, 0, 0, 0x7F }
#define SELECTED_FIVEPRIME_HALO_COLOR { 0, 0, 0x7F, 0x5F }
#define SELECTED_THREEPRIME_HALO_COLOR { 0, 0xAF, 0, 0x5F }
#define SELECTED_NEIGHBOUR_HALO_COLOR { 0, 0xAA, 0xAA, 0x5F }
#define SELECTED_ADJACENT_HALO_COLOR { 0xAF, 0x7F, 0x7F, 0x5F }
#define SELECTED_ADJACENT_NEIGHBOUR_HALO_COLOR { 0x7F, 0x7F, 0x7F, 0x5F }
#define SELECTED_ADJACENT_FIVEPRIME_HALO_COLOR { 0x7F, 0x7F, 0xAF, 0x5F }
#define SELECTED_ADJACENT_THREEPRIME_HALO_COLOR { 0x7F, 0xAF, 0x7F, 0x5F }

#define HALO_POINT_SIZE "0.0007"
#define BASE_CONNECTIONS_LINE_WIDTH 2.0f

#define HALO_THREE_PRIME_BASE_DIAMETER_MULTIPLIER 1.05f
#define HALO_FIVE_PRIME_BASE_DIAMETER_MULTIPLIER 1.1f
#define HALO_SELECTED_BASE_DIAMETER_MULTIPLIER 1.15f

#define HALO_DIAMETER_MULTIPLIER_ATTRIB_NAME "diameter_multiplier"

#define GLSL_VERTEX_SHADER																									\
		"#version 120\n",																									\
		"const float pointSize = " HALO_POINT_SIZE ";\n",																	\
		"uniform vec2 screen;\n",																							\
		"varying vec4 color;\n",																							\
		"varying float radius_multiplier;\n",																				\
		"attribute float " HALO_DIAMETER_MULTIPLIER_ATTRIB_NAME ";\n",														\
		"void main() {\n",																									\
		"    gl_Position = ftransform();\n",																				\
		"    gl_PointSize = screen.x * screen.y * pointSize * " HALO_DIAMETER_MULTIPLIER_ATTRIB_NAME " / (2.0 * gl_Position.w);\n",	\
		"	 color = gl_Color;\n",																							\
		"	 radius_multiplier = " HALO_DIAMETER_MULTIPLIER_ATTRIB_NAME ";\n",												\
		"}\n"

#define GLSL_VERTEX_SHADER_COUNT	12 // Always change this one when modifying the shader source code!

#define GLSL_FRAGMENT_SHADER																								\
		"#version 120\n",																									\
		"const float smooth_step = 0.05, halo_radius = 0.15;\n",															\
		"varying vec4 color;\n",																							\
		"varying float radius_multiplier;\n",																				\
		"void main() {\n",																									\
		"	 float radius = length(gl_PointCoord * 2.0 - vec2(1.0));\n",													\
		"    gl_FragColor = color * smoothstep(1.0, 1.0 - smooth_step, radius) *\n",										\
		"                   smoothstep((1.0 - smooth_step * 2 - halo_radius) / radius_multiplier, (1.0 - smooth_step - halo_radius) / radius_multiplier, radius);\n",	\
		"}\n"

#define GLSL_FRAGMENT_SHADER_COUNT	9 // Always change this one when modifying the shader source code!

namespace Helix {
	class HelixLocator : public MPxLocatorNode {
	public:
		inline HelixLocator() {

		}

		virtual ~HelixLocator();

		virtual void draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);

		virtual bool isBounded() const;
		virtual bool isTransparent() const;

		static void * creator();
		static MStatus initialize();

		const static MTypeId id;
		
	protected:
		static bool s_gl_initialized, s_gl_failed;
		static GLuint s_program, s_vertex_shader, s_fragment_shader;
		static GLint s_screen_dimensions_uniform, s_halo_size_attrib_location;

		void initializeGL();
	};
}

#endif /* HELIXLOCATOR_H_ */
