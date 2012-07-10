#ifndef _VIEW_HELIXCYLINDERHARDWARESHADER_H_
#define _VIEW_HELIXCYLINDERHARDWARESHADER_H_

#include <Definition.h>

#include <maya/MPxHardwareShader.h>

#ifdef MAC_PLUGIN

#include <OpenGL/glext.h>
#include <OpenGL/gl.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>

#endif /* N MAC_PLUGIN */

namespace Helix {
	namespace View {
		/*
		 * Implements a custom material that will be assigned to the Helix cylinder.
		 * The idea is to render a barberpole texture using a GLSL program, that has the colors of the materials of the bases
		 *
		 * The tricky part is the generation of colors to send to the GPU:
		 * - Create a texture that is length of cylinder / STEP (or number of bases length helix) and width 2 px
		 *   iterate over all base childs and take their color and put it in the texture
		 *
		 * Rendering is simple: The shader creates a barberpole pattern and every stripe fades between the colors, while the border between stripes
		 * is kept
		 *
		 * Note that the texture must have be in border mode, and the border color must be black, if not the lookup will wrap around
		 */

		class HelixCylinderHardwareShader : public MPxHardwareShader {
		public:
			HelixCylinderHardwareShader();
			virtual ~HelixCylinderHardwareShader();

			virtual MStatus render (MGeometryList & iterator);
			
			static void *creator();
			static MStatus initialize();

			static void initializeDraw();

		private:
			struct DrawData {
				GLuint program, vertex_shader, fragment_shader, texture;
				GLint texture_uniform;
				GLsizei texture_height; // The height of the image, if our number of bases change, we have to resize it
				bool initialized, failure;

				DrawData() : initialized(false), failure(false) { }
			} static s_drawData;
		};
	}
}

#endif /* N _VIEW_HELIXCYLINDERHARDWARESHADER_H_ */