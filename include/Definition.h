/*
 * Definition.h
 *
 *  Created on: 27 jan 2012
 *      Author: johan
 */

#ifndef DEFINITION_H_
#define DEFINITION_H_

#if !defined(LINUX) && !defined(WIN32) && !defined(MAC_PLUGIN)
#define LINUX /* Maya needs this */
#endif

#ifndef _BOOL
#define _BOOL /* Maya needs this */
#endif

#define _CRT_SECURE_NO_WARNINGS /* Visual Studios annoying warnings */
#define _SCL_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <iostream>

#ifndef MIN
#define MIN(a, b)		((a) < (b) ? (a) : (b))
#endif /* N MIN */

#ifndef MAX
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#endif /* N MAX */

/*
 * INFO: Remember to disable this when running in production mode as it will decrease performance
 */
#if 0
#ifndef GLCALL
#define GLCALL(call)	{														\
							GLenum glcall_error;								\
							call;												\
							if ((glcall_error = glGetError()) != GL_NO_ERROR)	\
								std::cerr << "ERROR executing \"" << #call << "\": code: " << glcall_error << ", message: \"" << gluErrorString(glcall_error) << "\"" << std::endl;	\
						}
#endif /* N GLCALL */
#else
#define GLCALL(call)	call
#endif

/*
 * Color defines for selections, from apiMeshShapeUI.cpp in the Maya SDK
 */

#define LEAD_COLOR                              18      // green
#define ACTIVE_COLOR                    15      // white
#define ACTIVE_AFFECTED_COLOR   8       // purple
#define DORMANT_COLOR                   4       // blue
#define HILITE_COLOR                    17      // pale blue
#define DORMANT_VERTEX_COLOR    8       // purple
#define ACTIVE_VERTEX_COLOR             16      // yellow


#endif /* DEFINITION_H_ */
