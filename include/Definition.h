/*
 * Definition.h
 *
 *  Created on: 27 jan 2012
 *      Author: johan
 */

#ifndef DEFINITION_H_
#define DEFINITION_H_

#if !defined(LINUX) && !defined(WIN32) && !defined(WIN64) && !defined(MAC_PLUGIN)
#define LINUX /* Maya needs this */
#endif

#ifndef _BOOL
#define _BOOL /* Maya needs this */
#endif

#define _CRT_SECURE_NO_WARNINGS /* Visual Studios annoying warnings */
#define _SCL_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#define NOMINMAX /* Windows defines min, max that messes up std::min, std::max */

#ifdef _MSC_VER
/*
 * It seems the compiler even complains about private objects of classes
 */
#pragma warning( disable : 4251 )
#endif /* _MSC_VER */

#include <iostream>

/*
 * Color defines for selections, from apiMeshShapeUI.cpp in the Maya SDK
 */

#define LEAD_COLOR						18	// green
#define ACTIVE_COLOR					15	// white
#define ACTIVE_AFFECTED_COLOR			8	// purple
#define DORMANT_COLOR					4	// blue
#define HILITE_COLOR					17	// pale blue
#define DORMANT_VERTEX_COLOR			8	// purple
#define ACTIVE_VERTEX_COLOR				16	// yellow

/*
 * Other libraries that want to use vHelix on Windows need this
 */

#ifdef VHELIX_EXPORTS
#define VHELIXAPI	__declspec( dllexport )
#elif defined(WIN32) || defined(WIN64)
#define VHELIXAPI __declspec( dllimport )
#else
#define VHELIXAPI
#endif /* N VHELIX_EXPORTS N WIN32 N WIN64 */

#endif /* DEFINITION_H_ */
