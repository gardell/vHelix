#ifndef _VHELIX_OPENGL_H_
#define _VHELIX_OPENGL_H_

/*
 * Because we used a lot of OpenGL extensions, i had to put them all in one file
 */

#include <Definition.h>

#if defined(WIN32) || defined(WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* Windows */

#ifdef MAC_PLUGIN

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#else

#define GL_GLEXT_PROTOTYPES
#define GLAPI

#include <GL/gl.h>
#include <GL/glext.h>

/*
 * INFO: Remember to disable this when running in production mode as it will decrease performance
 */
#if 0
#ifndef GLCALL

#ifndef MAC_PLUGIN
#include <GL/glu.h>
#else
#include <OpenGL/glu.h>
#endif /* MAC_PLUGIN */
#define GLCALL(call)	{														\
							GLenum glcall_error;								\
							call;												\
							if ((glcall_error = glGetError()) != GL_NO_ERROR)	\
								std::cerr << "ERROR executing \"" << #call << "\": code: " << glcall_error << ", message: \"" << gluErrorString(glcall_error) << "\"" << std::endl;	\
						}
#endif /* N GLCALL */
#else
#ifndef GLCALL
#define GLCALL(call)	call
#endif /* N GLCALL */
#endif

/*
 * Must be called in main when using GLX, in an active GL context when using WGL. Installs the OpenGL extensions
 */

bool installGLExtensions();

#endif /* N MAC_PLUGIN */

#if defined(WIN32) || defined(WIN64)
#define GETPROCADDRESS(str)		wglGetProcAddress((LPCSTR) str)
#else
#define GETPROCADDRESS(str)		glXGetProcAddressARB((const GLubyte *) str)
#include <GL/glx.h>
#include <GL/glxext.h>
#endif /* No windows */


#endif /* N _VHELIX_OPENGL_H_ */
