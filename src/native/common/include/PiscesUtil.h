/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 *  
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License version 
 * 2 only, as published by the Free Software Foundation. 
 *  
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License version 2 for more details (a copy is 
 * included at /legal/license.txt). 
 *  
 * You should have received a copy of the GNU General Public License 
 * version 2 along with this work; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 
 * 02110-1301 USA 
 *  
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa 
 * Clara, CA 95054 or visit www.sun.com if you need additional 
 * information or have any questions.
 */


/**
 * @file PiscesUtil.h 
 * PISCES memory management and other macro definitions.
 */ 

#ifndef PISCES_UTIL_H
#define PISCES_UTIL_H

#include <PiscesDefs.h>

#include <PiscesSysutils.h>

#ifndef ABS
/**
 * @def ABS(x)
 * Absolute value of x.
 */  
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef MIN
/**
 * @def MIN(a,b)
 * This macro gives minimum of a,b.
 */  
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
/**
 * @def MAX(a,b)
 * This macro gives maximum of a,b.
 */
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/**
 * @def my_malloc(type, len)
 * Allocates and cleares memory to zeros. Returns pointer to this memory 
 * type-casted to (type *). Size of allocated buffer is len*sizeof(type) bytes
 * long. 
 */
#define my_malloc(type, len) (type *)PISCEScalloc(len, sizeof(type))

/**
 * @def my_free(x)
 * Deallocates memory pointed to by pointer x. If x is NULL, does nothing.
 */  
#define my_free(x) do { if (x) PISCESfree(x); } while(0)

/* Clears count of bytes of memory pointed to by buffer to zero */
#define my_clear_mem(buffer,count) PISCESclear_mem(buffer,count)
 
/*
 * If 'array' is null or smaller than 'thresh', allocate with
 * length MAX(thresh, len).  Discard old contents.
 */
#define ALLOC(array, type, thresh, len) do { \
  if (array == NULL || array##_length < (thresh)) { \
    jint nlen = MAX(thresh, len); \
    PISCESfree(array); \
    array = my_malloc(type, nlen); \
    array##_length = nlen; \
  } \
} while (0)

/**
 * @def ALLOC3(array, type, len)
 * If 'array' is null or smaller than 'len', allocate with
 * length len.  Discard old contents.
 */
#define ALLOC3(array, type, len) ALLOC(array, type, len, len)

/**
 * @def REALLOC(array, type, thresh, len)
 * If 'array' is null or smaller than 'thresh', allocate with
 * length max(thresh, len).  Copy old contents into new storage.
 */
#define REALLOC(array, type, thresh, len) do { \
  if (array == NULL || array##_length < (thresh)) { \
    jint nlen; \
    nlen = MAX(thresh, len); \
    array = (type *)PISCESrealloc((array), nlen*sizeof(type)); \
    array##_length = nlen; \
  } \
} while (0)

/**
 * @def SHRINK(array, type, maxLen)
 * If 'array' is null or larger than 'maxLen', allocate with
 * length maxLen.  Discard old contents.
 */
#define SHRINK(array, type, maxLen) do { \
  if (array == NULL || array##_length > (maxLen)) { \
    if(array != NULL && array##_length > (maxLen)) { \
        PISCESfree(array); \
    } \
    array = my_malloc(type, (maxLen)); \
  } \
} while (0)

extern jint PISCES_STROKE_X_BIAS;
extern jint PISCES_STROKE_Y_BIAS;

extern jint *_Pisces_convert8To5;
extern jint *_Pisces_convert8To6;

jboolean piscesutil_moduleInitialize();
void piscesutil_moduleFinalize();
void piscesutil_setStrokeBias(jint xbias, jint ybias);

jlong PointerToJLong(void* ptr);
void* JLongToPointer(jlong ptr);

#endif
