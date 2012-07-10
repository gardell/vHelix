#ifndef _VHELIX_OPENGL_H_
#define _VHELIX_OPENGL_H_

/*
 * Because we used a lot of OpenGL extensions, i had to put them all in one file
 */

#include <Definition.h>

#if defined(WIN32) || defined(WIN64)
#define GETPROCADDRESS(str)		wglGetProcAddress((LPCSTR) str)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#define GETPROCADDRESS(str)		glXGetProcAddressARB((const GLubyte *) str)
#endif /* No windows */


#ifdef MAC_PLUGIN

#include <OpenGL/glext.h>
#include <OpenGL/gl.h>

#else

#define GL_GLEXT_PROTOTYPES
#define GLAPI

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

/*
 * Must be called in main when using GLX, in an active GL context when using WGL. Installs the OpenGL extensions
 */

bool installGLExtensions();

#endif /* N MAC_PLUGIN */


#endif /* N _VHELIX_OPENGL_H_ */