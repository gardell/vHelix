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

#endif /* DEFINITION_H_ */
