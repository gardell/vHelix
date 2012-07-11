#ifndef _VIEW_BASESHAPEUI_H_
#define _VIEW_BASESHAPEUI_H_

#include <Definition.h>

#include <maya/MPxSurfaceShapeUI.h>

#ifdef MAC_PLUGIN

#include <OpenGL/glext.h>
#include <OpenGL/gl.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>

#endif /* N MAC_PLUGIN */

/*
 * BaseShapeUI
 */

#define BASESHAPE_GLSL_VERTEX_SHADER															\
	"varying vec3 Normal, EyeVec;\n",															\
																								\
	"void main() {\n",																			\
	"	vec4 wVertex = gl_ModelViewMatrix * gl_Vertex;\n",										\
	"	gl_Position = gl_ProjectionMatrix * wVertex;\n",										\
	"	EyeVec = -vec3(wVertex);\n",															\
	"	Normal = gl_NormalMatrix * gl_Normal;\n",												\
	"}\n"

#define BASESHAPE_GLSL_VERTEX_SHADER_COUNT 7

#define BASESHAPE_GLSL_FRAGMENT_SHADER															\
	"const float Shininess = 6.0;\n",															\
																								\
	"uniform vec3 color, borderColor;\n",														\
	"uniform float border;\n",																	\
																								\
	"varying vec3 Normal, EyeVec;\n",															\
																								\
	"void main() {\n",																			\
	"	vec3 N = normalize(Normal), L = normalize(EyeVec), E = normalize(EyeVec), R = reflect(-L, N);\n",		\
	"	float lambertTerm = dot(N, L);\n",														\
	"	float borderFlag = smoothstep(0.3 * (border * 2.0 - 1.0), 0.7 * border, abs(lambertTerm));\n",								\
	"	gl_FragColor = vec4(borderColor * (1.0 - borderFlag) + borderFlag * color * max(0.0, lambertTerm), 1.0);\n",		\
	"}\n"

#define BASESHAPE_GLSL_FRAGMENT_SHADER_COUNT 10

namespace Helix {
	namespace View {
		class BaseShapeUI : public MPxSurfaceShapeUI {
		public:
			virtual void    getDrawRequests( const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & requests );

			// Main draw routine. Gets called by maya with draw requests.
			//
			virtual void    draw( const MDrawRequest & request, M3dView & view ) const;

			// Main selection routine
			//
			virtual bool    select( MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts ) const;

			static  void *creator();

			// Helper methods
			//

			/*
			 * Initialize OpenGL, done statically as the program and models are shared between all helices
			 */

			static void initializeDraw();

		private:
			/*
			 * OpenGL data for managing the arrows. VBO's, shaders, uniforms etc
			 * Notice: For generating the model into a C header, i used a command line tool called obj2opengl,
			 * that saved a few hours of work. Unfortenately, it uses glDrawArrays and not glDrawElements
			 */

			struct DrawData {
				GLuint program, vertex_shader, fragment_shader, display_list;
				GLint color_uniform, borderColor_uniform, border_uniform;
				bool initialized, failure;

				DrawData() : initialized(false), failure(false) { }
			} static s_drawData;
		};
	}
}

#endif /* N _VIEW_BASESHAPEUI_H_ */