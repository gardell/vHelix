#ifndef _HELIXSHAPEUI_H_
#define _HELIXSHAPEUI_H_

#include <Definition.h>

#include <iostream>

#include <maya/MPxSurfaceShapeUI.h>

#ifdef MAC_PLUGIN

#include <OpenGL/glext.h>
#include <OpenGL/gl.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#endif /* N MAC_PLUGIN */

#define HELIXSHAPE_CYLINDER_SLICES	32

#define HELIXSHAPE_GLSL_VERTEX_SHADER															\
	"uniform vec2 range;\n",																	\
																								\
	"varying vec2 TexCoord;\n",																	\
	"varying vec3 Normal, EyeVec;\n",															\
																								\
	"const float PI = 3.14159265358979323846264;\n",											\
																								\
	"void main() {\n",																			\
	"	vec4 wVertex = gl_ModelViewMatrix * vec4(gl_Vertex.xy, range.x + (gl_Vertex.z - 0.5) * range.y, 1.0);\n",	\
	"	gl_Position = gl_ProjectionMatrix * wVertex;\n",										\
	"	EyeVec = -vec3(wVertex);\n",															\
	"	Normal = gl_NormalMatrix * vec3(gl_Vertex.xy, 0.0);\n",									\
	"	TexCoord = vec2(atan(gl_Vertex.y, gl_Vertex.x) / (2.0 * PI) + 0.5, gl_Vertex.z);\n",	\
	"}\n"

#define HELIXSHAPE_GLSL_VERTEX_SHADER_COUNT 11

#define HELIXSHAPE_GLSL_FRAGMENT_SHADER															\
	"uniform vec2 range;\n",																	\
	"uniform vec3 borderColor;\n",																\
	"uniform sampler2D texture;\n",																\
																								\
	"const float PI = 3.14159265358979323846264;\n",											\
	"const float Shininess = 6.0;\n",															\
	"const float Step = " STEP_STR ";\n",														\
	"const float PitchOverStep = " PITCH_STR " / Step / 180.0 * 2.0 * PI;\n",					\
																								\
	"varying vec2 TexCoord;\n",																	\
	"varying vec3 Normal, EyeVec;\n",															\
																								\
	"void main() {\n",																			\
	"	float y = TexCoord.y * range.y, xcoord = TexCoord.x - y / PitchOverStep;\n",			\
	"	vec4 texColor = texture2D(texture, vec2((sign(fract(xcoord) * 2.0 - 1.0) + 1.0) / 4.0 + 0.25, TexCoord.y));\n",	\
	"	if (texColor.a < 0.5)\n",																\
	"		discard;\n",																		\
	"	vec3 N = normalize(Normal), L = normalize(EyeVec), E = normalize(EyeVec), R = reflect(-L, N);\n",				\
	"	float lambertTerm = dot(N, L), x = fract(xcoord * 2.0);\n",														\
	"	float border = step(0.05, x) * (1.0 - step(0.95, x)) * step(0.05, y) * (1.0 - step(range.y - 0.05, y));\n",		\
	"	gl_FragColor = vec4(texColor.rgb * lambertTerm * border + borderColor * (1.0 - border), 1.0);\n",					\
	"}\n"

// vec4(TexCoord, 0.0, 1.0) + 0.01 * 
// For phong shading
//	"	float lambertTerm = dot(N, L), specular = pow( max(dot(R, E), 0.0), Shininess);\n",										\
//	"	gl_FragColor = sign(max(0.0, lambertTerm)) * (Color * lambertTerm + vec4(1.0, 1.0, 1.0, 1.0) * specular);\n",			\

#define HELIXSHAPE_GLSL_FRAGMENT_SHADER_COUNT 19

namespace Helix {
	namespace View {
		class HelixShapeUI : public MPxSurfaceShapeUI {
		public:
			virtual void getDrawRequests (const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & requests);
			virtual void draw (const MDrawRequest & request, M3dView & view) const;
			virtual bool select (MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts) const;

			static void *creator();

			/*
			 * Initialize OpenGL, done statically as the program and models are shared between all helices
			 */

			static void initializeDraw();

		protected:

			/*
			 * OpenGL data
			 */

			struct DrawData {
				GLuint program, vertex_shader, fragment_shader, draw_display_list, select_display_list, texture;
				GLint texture_uniform, range_uniform, borderColor_uniform;
				GLsizei texture_height; // If the number of bases change, we have to resize the texture

				bool initialized, failure;

				DrawData() : texture_height(0), initialized(false), failure(false) { }
			} static s_drawData;
		};
	}
}

#endif /* N _HELIXSHAPEUI_H_ */