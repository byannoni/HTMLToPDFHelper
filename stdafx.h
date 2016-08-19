// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <stdarg.h>
#include <stdlib.h>
#include <WinInet.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <Psapi.h>
#include <strsafe.h>
#include <locale.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>

/*
 * Determine the number of elements in an array. NOTE: this is not valid
 * for any pointers; the variable arr must be an array.
 */
#define LENGTHOF(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef _DEBUG
#define RT_NOT_NULL_S "%s encountered an unexpected null pointer (%s) at %s:%d"
#else
#define RT_NOT_NULL_S "%s encountered an unexpected null pointer"
#endif

#define RT_NOT_NULL(ptr) \
	do { \
		if((ptr) == NULL) \
			errorout(E_NULLPTR, _T(RT_NOT_NULL_S), _T(__FUNCTION__), \
					_T(#ptr), _T(__FILE__), __LINE__); \
	} while(0)

#ifndef va_copy
/*
 * This technically invokes undefined behavior, but Microsoft doesn't
 * provide an implementation for va_copy() until VS 2013, so it's the
 * best we can do.
 */
#define va_copy(dest, src) ((dest) = (src))
#endif
// TODO: reference additional headers your program requires here
