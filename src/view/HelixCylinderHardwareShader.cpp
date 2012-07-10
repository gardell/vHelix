#include <view/HelixCylinderHardwareShader.h>

//
// OpenGL 2.0 extensions
//

#if defined(WIN32) || defined(WIN64)
#define GETPROCADDRESS(str)		wglGetProcAddress((LPCSTR) str)
#else
#define GETPROCADDRESS(str)		glXGetProcAddressARB((const GLubyte *) str)
#endif /* No windows */

namespace Helix {
	namespace View {
		HelixCylinderHardwareShader::DrawData HelixCylinderHardwareShader::s_drawData;

#ifndef MAC_PLUGIN
		PFNGLCREATEPROGRAMPROC glCreateProgram;
		PFNGLCREATESHADERPROC glCreateShader;
		PFNGLATTACHSHADERPROC glAttachShader;
		PFNGLCOMPILESHADERPROC glCompileShader;
		PFNGLLINKPROGRAMPROC glLinkProgram;
		PFNGLSHADERSOURCEPROC glShaderSource;
		PFNGLDELETESHADERPROC glDeleteShader;
		PFNGLDELETEPROGRAMPROC glDeleteProgram;
		PFNGLUSEPROGRAMPROC glUseProgram;
		PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
		PFNGLGETSHADERIVPROC glGetShaderiv;
		PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
		PFNGLGETPROGRAMIVPROC glGetProgramiv;
		PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
		PFNGLUNIFORM2FPROC glUniform2f;
		PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
		PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
		PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
		PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

		PFNGLDRAWELEMENTSINSTANCEDARBPROC glDrawElementsInstancedARB;
		PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
		PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
		PFNGLUNIFORM3FVPROC glUniform3fv;
		PFNGLGENBUFFERSPROC glGenBuffers;
		PFNGLBINDBUFFERPROC glBindBuffer;
		PFNGLBUFFERDATAPROC glBufferData;
		PFNGLBUFFERSUBDATAPROC glBufferSubData;
		PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
		PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
		PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
		PFNGLUNIFORM4FPROC glUniform4f;

#endif /* N MAC_PLUGIN */

		HelixCylinderHardwareShader::HelixCylinderHardwareShader() {

		}

		HelixCylinderHardwareShader::~HelixCylinderHardwareShader() {

		}

		MStatus HelixCylinderHardwareShader::render (MGeometryList & iterator) {
			if (s_drawData.failure)
				return MStatus::kFailure;

			if (!s_drawData.initialized)
				initializeDraw();

			/*
			 * Extract base data and put them in the texture to be used for rendering
			 */

			MPlug 

			glPushAttrib(GL_TEXTURE_BIT);

			glBindTexture(GL_TEXTURE_2D, s_drawData.texture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, s_drawData.texture_height, 1, GL_RGB, GL_UNSIGNED_BYTE, NULL);

			glPopAttrib();

			return MStatus::kSuccess;
		}
			
		void *HelixCylinderHardwareShader::creator() {
			return new HelixCylinderHardwareShader();
		}

		MStatus HelixCylinderHardwareShader::initialize() {
			return MStatus::kSuccess;
		}

		void HelixCylinderHardwareShader::initializeDraw() {
#ifndef MAC_PLUGIN
			if ((glCreateProgram = (PFNGLCREATEPROGRAMPROC) GETPROCADDRESS("glCreateProgram")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glCreateShader = (PFNGLCREATESHADERPROC) GETPROCADDRESS("glCreateShader")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}
			if ((glAttachShader = (PFNGLATTACHSHADERPROC) GETPROCADDRESS("glAttachShader")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glCompileShader = (PFNGLCOMPILESHADERPROC) GETPROCADDRESS("glCompileShader")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glLinkProgram = (PFNGLLINKPROGRAMPROC) GETPROCADDRESS("glLinkProgram")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glShaderSource = (PFNGLSHADERSOURCEPROC) GETPROCADDRESS("glShaderSource")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glDeleteShader = (PFNGLDELETESHADERPROC) GETPROCADDRESS("glDeleteShader")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glDeleteProgram = (PFNGLDELETEPROGRAMPROC) GETPROCADDRESS("glDeleteProgram")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glUseProgram = (PFNGLUSEPROGRAMPROC) GETPROCADDRESS("glUseProgram")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) GETPROCADDRESS("glGetShaderInfoLog")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGetShaderiv = (PFNGLGETSHADERIVPROC) GETPROCADDRESS("glGetShaderiv")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) GETPROCADDRESS("glGetProgramInfoLog")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGetProgramiv = (PFNGLGETPROGRAMIVPROC) GETPROCADDRESS("glGetProgramiv")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) GETPROCADDRESS("glGetUniformLocation")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glUniform2f = (PFNGLUNIFORM2FPROC) GETPROCADDRESS("glUniform2f")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) GETPROCADDRESS("glGetAttribLocation")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) GETPROCADDRESS("glEnableVertexAttribArray")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) GETPROCADDRESS("glDisableVertexAttribArray")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) GETPROCADDRESS("glVertexAttribPointer")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			/*
			 * New functions
			 */

			if ((glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC) GETPROCADDRESS("glDrawElementsInstancedARB")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) GETPROCADDRESS("glUniformMatrix4fv")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glUniform3fv = (PFNGLUNIFORM3FVPROC) GETPROCADDRESS("glUniform3fv")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glBindBuffer = (PFNGLBINDBUFFERPROC) GETPROCADDRESS("glBindBuffer")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glBufferData = (PFNGLBUFFERDATAPROC) GETPROCADDRESS("glBufferData")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGenBuffers = (PFNGLGENBUFFERSPROC) GETPROCADDRESS("glGenBuffers")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glBufferSubData = (PFNGLBUFFERSUBDATAPROC) GETPROCADDRESS("glBufferSubData")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) GETPROCADDRESS("glUniformMatrix3fv")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) GETPROCADDRESS("glGenVertexArrays")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) GETPROCADDRESS("glDeleteVertexArrays")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) GETPROCADDRESS("glBindVertexArray")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

			if ((glUniform4f = (PFNGLUNIFORM4FPROC) GETPROCADDRESS("glUniform4f")) == NULL) {
				std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
				s_drawData.failure = true;
			}

#endif /* N MAC_PLUGIN */

			/*
			 * Now, setup shaders, program, VBO's, uniforms and download the models to the VBO's
			 */

			// Setup the shaders and programs

			if ((s_drawData.program = glCreateProgram()) == 0) {
				std::cerr << "Fatal, failed to create an OpenGL program object" << std::endl;
				s_drawData.failure = true;
			}

			if ((s_drawData.vertex_shader = glCreateShader(GL_VERTEX_SHADER)) == 0) {
				std::cerr << "Fatal, failed to create an OpenGL shader object" << std::endl;
				s_drawData.failure = true;
			}

			if ((s_drawData.fragment_shader = glCreateShader(GL_FRAGMENT_SHADER)) == 0) {
				std::cerr << "Fatal, failed to create an OpenGL shader object" << std::endl;
				s_drawData.failure = true;
			}

			static const char *vertex_shader[] = { HELIXCYLINDER_HARDWARE_GLSL_VERTEX_SHADER, NULL }, *fragment_shader[] = { HELIXCYLINDER_HARDWARE_GLSL_FRAGMENT_SHADER, NULL };
			
			glShaderSource(s_drawData.vertex_shader, HELIXCYLINDER_HARDWARE_GLSL_VERTEX_SHADER_COUNT, vertex_shader, NULL);
			glShaderSource(s_drawData.fragment_shader, HELIXCYLINDER_HARDWARE_GLSL_FRAGMENT_SHADER_COUNT, fragment_shader, NULL);

			glCompileShader(s_drawData.vertex_shader);
			glCompileShader(s_drawData.fragment_shader);

			GLint status;

			{
				GLint shaders[] = { s_drawData.vertex_shader, s_drawData.fragment_shader };
				
				for(int i = 0; i < 2; ++i) {
					

					glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);

					if (status != GL_TRUE) {
						GLint logLength;
						glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &logLength);
						GLchar *log = new GLchar[logLength];
						glGetShaderInfoLog(shaders[i], logLength, NULL, log);

						std::cerr << "OpenGL shader info log (" << (i + 1) << "): " << std::endl << log << std::endl << std::endl;

						s_drawData.failure = true;
					}

					glAttachShader(s_drawData.program, shaders[i]);
				}
			}

			glLinkProgram(s_drawData.program);

			glGetProgramiv(s_drawData.program, GL_LINK_STATUS, &status);

			if (status != GL_TRUE) {
				GLint logLength;
				glGetProgramiv(s_drawData.program, GL_INFO_LOG_LENGTH, &logLength);
				GLchar *log = new GLchar[logLength];
				glGetProgramInfoLog(s_drawData.program, logLength, NULL, log);

				std::cerr << "OpenGL program info log: " << std::endl << log << std::endl << std::endl;

				s_drawData.failure = true;
			}

			/*
			 * Setup uniforms
			 */

			if ((s_drawData.texture_uniform = glGetUniformLocation(s_drawData.program, "texture")) == -1) {
				std::cerr << "Couldn't find uniform texture" << std::endl;
				//s_drawData.failure = true;
			}

			/*
			 * Setup texture
			 */

			glGenTextures(1, &s_drawData.texture);

			s_drawData.initialized = true;
		}
	}
}