/*
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
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


#ifndef PISCES_UTIL_H
#define PISCES_UTIL_H

#include <PiscesDefs.h>

#include <PiscesSysutils.h>

#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/* Always zero allocated memory */
#define my_malloc(type, len) (type *)PISCEScalloc(len, sizeof(type))

#define my_free(x) do { if (x) PISCESfree(x); } while(0)

/*
 * If 'array' is null or smaller than 'thresh', allocate with 
 * length max(thresh, len).  Discard old contents.
 */
#define ALLOC(array, type, thresh, len) do { \
  if (array == NULL || array##_length < (thresh)) { \
    jint nlen = MAX(thresh, len); \
    PISCESfree(array); \
    array = my_malloc(type, nlen); \
    array##_length = nlen; \
  } \
} while (0)

/*
 * If 'array' is null or smaller than 'len', allocate with 
 * length len.  Discard old contents.
 */
#define ALLOC3(array, type, len) ALLOC(array, type, len, len)

/*
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

/*
 * If 'array' is null or larger than 'maxLen', allocate with 
 * length maxLen.  Discard old contents.
 */
#define SHRINK(array, type, maxLen) do { \
  if (array##_length > (maxLen)) { \
    PISCESfree(array); \
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
