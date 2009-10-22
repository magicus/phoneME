/*
 * @(#)directmem.h	1.57 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

/*
 * This file is the definition of the direct memory interface. All
 * Java object and array accesses must go through this interface.  
 */

#ifndef _INCLUDED_DIRECTMEM_H
#define _INCLUDED_DIRECTMEM_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/cstates.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/utils.h"
#include "javavm/include/globals.h"

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/memory.h"
#include "javavm/include/clib.h"

/*
 * Read/write barriers (applying to arrays as well as objects)
 *
 * 1) Reference-typed (for reference-typed object fields and array elements)
 * 2) Non-reference typed
 *    a) 64-bit wide  (for long and double fields and array elements)
 *    b) Int and Float
 *    c) Short and Char
 *    d) Byte and Bool
 *
 * There are a total of 8 read and 8 write barriers.
 *
 * The barriers are invoked right before the read or write takes
 * place.
 * 
 * If a given type of barrier is not supplied by a particular GC
 * implementation, we just provide an empty implementation.
 *
 * Read barriers look like:
 *
 *     CVMgcimplReadBarrier<T>(CVMObject*  directObj,
 *                             <slotT>*    slotAddr);
 *
 * where <T> is one of Int,Float,Short,Char,Bool,Byte,64 or Ref
 * and the corresponding <slotT> is one of
 * CVMJava{Int,Float,Short,Char,Bool,Byte,32} or CVMObject*.
 *
 * CVMgcimplReadBarrier<T> is called right before the system reads 
 * the slot with address 'slotAddr' from object 'directObj'.
 *
 * Write barriers are very similar:
 * 
 *     CVMgcimplWriteBarrier<T>(CVMObject*  directObj,
 *                              <slotT>*    slotAddr,
 *                              <slotT>     rhsValue);
 *
 * where <T> is one of Int,Float,Short,Char,Bool,Byte or Ref
 * and the corresponding <slotT> is one of
 * CVMJava{Int,Float,Short,Char,Bool,Byte} or CVMObject*.
 *
 * 64-bit value write barriers look very slightly different:
 * 
 *     CVMgcimplWriteBarrier64(CVMObject*  directObj,
 *                             CVMJavaVal32*  slotAddr,
 *                             CVMJavaVal32*  rhsValue);
 * 
 * in that rhsValue is a CVMJavaVal32* and not a 64-bit value
 *
 * CVMgcimplWriteBarrier<T> is called right before the system writes
 * the value 'rhsValue' to the slot with address 'slotAddr' in object
 * 'directObj'.
 */

/*
 * If we don't have any non-reference barriers, then we can optimize
 * CVMjniGetPrimitiveArrayCritical and CVMjniReleasePrimitiveArrayCritical.
 */
#if defined(CVMgcimplReadBarrierByte)     ||	\
    defined(CVMgcimplReadBarrierBoolean)  ||	\
    defined(CVMgcimplReadBarrierShort)    ||	\
    defined(CVMgcimplReadBarrierChar)     ||	\
    defined(CVMgcimplReadBarrierInt)      ||	\
    defined(CVMgcimplReadBarrierFloat)    ||	\
    defined(CVMgcimplReadBarrier64)       ||	\
    defined(CVMgcimplWriteBarrierByte)    ||	\
    defined(CVMgcimplWriteBarrierBoolean) ||	\
    defined(CVMgcimplWriteBarrierShort)   ||	\
    defined(CVMgcimplWriteBarrierChar)    ||	\
    defined(CVMgcimplWriteBarrierInt)     ||	\
    defined(CVMgcimplWriteBarrierFloat)   ||	\
    defined(CVMgcimplWriteBarrier64)
#define CVMGC_HAS_NONREF_BARRIERS
#endif

#undef CVMGC_HAS_NO_BYTE_READ_BARRIER
#undef CVMGC_HAS_NO_BYTE_WRITE_BARRIER
#undef CVMGC_HAS_NO_BOOLEAN_READ_BARRIER
#undef CVMGC_HAS_NO_BOOLEAN_WRITE_BARRIER
#undef CVMGC_HAS_NO_SHORT_READ_BARRIER
#undef CVMGC_HAS_NO_SHORT_WRITE_BARRIER
#undef CVMGC_HAS_NO_CHAR_READ_BARRIER
#undef CVMGC_HAS_NO_CHAR_WRITE_BARRIER
#undef CVMGC_HAS_NO_INT_READ_BARRIER
#undef CVMGC_HAS_NO_INT_WRITE_BARRIER
#undef CVMGC_HAS_NO_FLOAT_READ_BARRIER
#undef CVMGC_HAS_NO_FLOAT_WRITE_BARRIER
#undef CVMGC_HAS_NO_REF_READ_BARRIER
#undef CVMGC_HAS_NO_REF_WRITE_BARRIER
#undef CVMGC_HAS_NO_64BIT_READ_BARRIER
#undef CVMGC_HAS_NO_64BIT_WRITE_BARRIER


#ifndef CVMgcimplReadBarrierByte
#define CVMgcimplReadBarrierByte(objRef, fieldLoc_)
#define CVMGC_HAS_NO_BYTE_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierBoolean
#define CVMgcimplReadBarrierBoolean(objRef, fieldLoc_)
#define CVMGC_HAS_NO_BOOLEAN_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierShort
#define CVMgcimplReadBarrierShort(objRef, fieldLoc_)
#define CVMGC_HAS_NO_SHORT_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierChar
#define CVMgcimplReadBarrierChar(objRef, fieldLoc_)
#define CVMGC_HAS_NO_CHAR_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierInt
#define CVMgcimplReadBarrierInt(objRef, fieldLoc_)
#define CVMGC_HAS_NO_INT_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierFloat
#define CVMgcimplReadBarrierFloat(objRef, fieldLoc_)
#define CVMGC_HAS_NO_FLOAT_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierRef
#define CVMgcimplReadBarrierRef(objRef, fieldLoc_)
#define CVMGC_HAS_NO_REF_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrier64
#define CVMgcimplReadBarrier64(objRef, fieldLoc_)
#define CVMGC_HAS_NO_64BIT_READ_BARRIER
#endif

#ifndef CVMgcimplReadBarrierAddr
#define CVMgcimplReadBarrierAddr(objRef, fieldLoc_)
#define CVMGC_HAS_NO_ADDR_READ_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierByte
#define CVMgcimplWriteBarrierByte(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_BYTE_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierBoolean
#define CVMgcimplWriteBarrierBoolean(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_BOOLEAN_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierShort
#define CVMgcimplWriteBarrierShort(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_SHORT_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierChar
#define CVMgcimplWriteBarrierChar(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_CHAR_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierInt
#define CVMgcimplWriteBarrierInt(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_INT_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierFloat
#define CVMgcimplWriteBarrierFloat(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_FLOAT_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierRef
#define CVMgcimplWriteBarrierRef(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_REF_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrierAddr
#define CVMgcimplWriteBarrierAddr(objRef, fieldLoc_, rhs_)
#define CVMGC_HAS_NO_ADDR_WRITE_BARRIER
#endif

#ifndef CVMgcimplWriteBarrier64
#define CVMgcimplWriteBarrier64(objRef, fieldLoc_, location_)
#define CVMGC_HAS_NO_64BIT_WRITE_BARRIER
#endif

/*
 * Array block copy, read and write routines.
 *
 * CVMD_arrayReadBody<type>(buf, arr, start, len) will read "len"
 * elements of type <type> from the array "arr" starting from index
 * "start". The elements will be written into the C buffer "buf". The
 * right read barrier will be invoked on an element by element basis.
 *
 * If a particular GC implementation defines
 * CVMgcimpArrayReadBody<type>, CVMD_arrayReadBody<type> will go
 * through that definition instead of the default.  
 *
 * CVMD_arrayWriteBody<type>(buf, arr, start, len) will write "len"
 * elements of type <type> to the array "arr" starting from index
 * "start". The elements will be read from the C buffer "buf". The
 * right write barrier will be invoked on an element by element basis.
 *
 * If a particular GC implementation defines
 * CVMgcimpArrayWriteBody<type>, CVMD_arrayWriteBody<type> will go
 * through that definition instead of the default.  
 *
 * CVMD_arrayCopy<type>(srcArr, srcIdx, dstArr, dstIdx, len) will copy
 * 'len' elements of type <type> from srcArr[srcIdx, srcIdx+len) into
 * dstArr[dstIdx, dstIdx+len).  The right read and write barriers will
 * be invoked on an element by element basis.
 *
 * If a particular GC implementation defines
 * CVMgcimpArrayCopy<type>, CVMD_arrayCopy<type> will go
 * through that definition instead of the default.  
 */

/*
 * Default array block read, write and copy routines. 
 */

#define CVMDprivateDefaultArrayBodyRead(buf, arr, start, len,		\
					jType, accessor)		\
{									\
    CVMJavaInt i;							\
    jType* bufPtr = (jType*)(buf);					\
    CVMassert((start) + (len) <= (arr)->length);			\
    for (i = (start); i < (start) + (len); i++, bufPtr++) {		\
	CVMD_arrayRead##accessor(arr, i, *bufPtr);			\
    }									\
}

#define CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, 		\
					 jType, accessor)		\
{									\
    CVMJavaInt i;							\
    jType* bufPtr = (jType*)(buf);					\
    CVMassert((start) + (len) <= (arr)->length);			\
    for (i = (start); i < (start) + (len); i++, bufPtr++) {		\
	CVMD_arrayWrite##accessor(arr, i, *bufPtr);			\
    }									\
}

/*
 * The default no barrier version does what 'memmove' does.
 */
#define CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx,		\
                                             dstArr, dstIdx,		\
                                             len, jType, accessor)	\
{									\
    jType volatile * srcElements_;					\
    jType volatile * dstElements_;					\
    CVMassert((srcIdx) + (len) <= (srcArr)->length);			\
    CVMassert((dstIdx) + (len) <= (dstArr)->length);			\
    srcElements_ =							\
        (jType volatile*)CVMDprivate_arrayElemLoc((srcArr), (srcIdx));	\
    dstElements_ =							\
        (jType volatile*)CVMDprivate_arrayElemLoc((dstArr), (dstIdx));	\
    CVMmemmove## accessor ((jType*)dstElements_, (jType*)srcElements_,	\
                           (len) * sizeof(jType));			\
}

/*
 * The default does what 'memmove' does, but element-wise to get all the
 * barriers invoked
 */
#define CVMDprivateDefaultArrayCopy(srcArr, srcIdx,			\
				    dstArr, dstIdx, len,		\
				    jType, accessor)			\
{									\
    CVMJavaInt i, j;							\
    CVMassert((srcIdx) + (len) <= (srcArr)->length);			\
    CVMassert((dstIdx) + (len) <= (dstArr)->length);			\
    if (((dstArr) != (srcArr)) || ((dstIdx) < (srcIdx))) {		\
	for (i = (srcIdx), j = (dstIdx); i < (srcIdx) + (len); 		\
	     i++, j++) {						\
	    jType elem;							\
	    CVMD_arrayRead##accessor(srcArr, i, elem);			\
	    CVMD_arrayWrite##accessor(dstArr, j, elem);			\
	}								\
    } else {								\
	for (i = (srcIdx) + (len) - 1, j = (dstIdx) + (len) - 1;	\
	     i >= (srcIdx); 						\
	     i--, j--) {						\
	    jType elem;							\
	    CVMD_arrayRead##accessor(srcArr, i, elem);			\
	    CVMD_arrayWrite##accessor(dstArr, j, elem);			\
	}								\
    }									\
}

#ifndef CVMgcimplArrayReadBodyByte
#define CVMD_arrayReadBodyByte(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaByte, Byte)
#else
#define CVMD_arrayReadBodyByte CVMgcimplArrayReadBodyByte
#endif

#ifndef CVMgcimplArrayReadBodyBoolean
#define CVMD_arrayReadBodyBoolean(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaBoolean, Boolean)
#else
#define CVMD_arrayReadBodyBoolean CVMgcimplArrayReadBodyBoolean
#endif

#ifndef CVMgcimplArrayReadBodyShort
#define CVMD_arrayReadBodyShort(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaShort, Short)
#else
#define CVMD_arrayReadBodyShort CVMgcimplArrayReadBodyShort
#endif

#ifndef CVMgcimplArrayReadBodyChar
#define CVMD_arrayReadBodyChar(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaChar, Char)
#else
#define CVMD_arrayReadBodyChar CVMgcimplArrayReadBodyChar
#endif

#ifndef CVMgcimplArrayReadBodyInt
#define CVMD_arrayReadBodyInt(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaInt, Int)
#else
#define CVMD_arrayReadBodyInt CVMgcimplArrayReadBodyInt
#endif

#ifndef CVMgcimplArrayReadBodyFloat
#define CVMD_arrayReadBodyFloat(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaFloat, Float)
#else
#define CVMD_arrayReadBodyFloat CVMgcimplArrayReadBodyFloat
#endif

#ifndef CVMgcimplArrayReadBodyRef
#define CVMD_arrayReadBodyRef(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMObject*, Ref)
#else
#define CVMD_arrayReadBodyRef CVMgcimplArrayReadBodyRef
#endif

#ifndef CVMgcimplArrayReadBodyLong
#define CVMD_arrayReadBodyLong(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaLong, Long)
#else
#define CVMD_arrayReadBodyLong CVMgcimplArrayReadBodyLong
#endif

#ifndef CVMgcimplArrayReadBodyDouble
#define CVMD_arrayReadBodyDouble(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyRead(buf, arr, start, len, \
                                    CVMJavaDouble, Double)
#else
#define CVMD_arrayReadBodyDouble CVMgcimplArrayReadBodyDouble
#endif

#ifndef CVMgcimplArrayWriteBodyByte
#define CVMD_arrayWriteBodyByte(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaByte, Byte)
#else
#define CVMD_arrayWriteBodyByte CVMgcimplArrayWriteBodyByte
#endif

#ifndef CVMgcimplArrayWriteBodyBoolean
#define CVMD_arrayWriteBodyBoolean(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaBoolean, Boolean)
#else
#define CVMD_arrayWriteBodyBoolean CVMgcimplArrayWriteBodyBoolean
#endif

#ifndef CVMgcimplArrayWriteBodyShort
#define CVMD_arrayWriteBodyShort(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaShort, Short)
#else
#define CVMD_arrayWriteBodyShort CVMgcimplArrayWriteBodyShort
#endif

#ifndef CVMgcimplArrayWriteBodyChar
#define CVMD_arrayWriteBodyChar(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaChar, Char)
#else
#define CVMD_arrayWriteBodyChar CVMgcimplArrayWriteBodyChar
#endif

#ifndef CVMgcimplArrayWriteBodyInt
#define CVMD_arrayWriteBodyInt(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaInt, Int)
#else
#define CVMD_arrayWriteBodyInt CVMgcimplArrayWriteBodyInt
#endif

#ifndef CVMgcimplArrayWriteBodyFloat
#define CVMD_arrayWriteBodyFloat(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaFloat, Float)
#else
#define CVMD_arrayWriteBodyFloat CVMgcimplArrayWriteBodyFloat
#endif

#ifndef CVMgcimplArrayWriteBodyRef
#define CVMD_arrayWriteBodyRef(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMObject*, Ref)
#else
#define CVMD_arrayWriteBodyRef CVMgcimplArrayWriteBodyRef
#endif

#ifndef CVMgcimplArrayWriteBodyLong
#define CVMD_arrayWriteBodyLong(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaLong, Long)
#else
#define CVMD_arrayWriteBodyLong CVMgcimplArrayWriteBodyLong
#endif

#ifndef CVMgcimplArrayWriteBodyDouble
#define CVMD_arrayWriteBodyDouble(buf, arr, start, len) \
    CVMDprivateDefaultArrayBodyWrite(buf, arr, start, len, \
                                     CVMJavaDouble, Double)
#else
#define CVMD_arrayWriteBodyDouble CVMgcimplArrayWriteBodyDouble
#endif

/*
 * Array copy definitions
 */
#ifdef CVMgcimplArrayCopyByte
#define CVMD_arrayCopyByte CVMgcimplArrayCopyByte
#elif defined(CVMGC_HAS_NO_BYTE_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_BYTE_WRITE_BARRIER)
#define CVMD_arrayCopyByte(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaByte, Byte)
#else
#define CVMD_arrayCopyByte(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaByte, Byte)
#endif

#ifdef CVMgcimplArrayCopyBoolean
#define CVMD_arrayCopyBoolean CVMgcimplArrayCopyBoolean
#elif defined(CVMGC_HAS_NO_BOOLEAN_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_BOOLEAN_WRITE_BARRIER)
#define CVMD_arrayCopyBoolean(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaBoolean, Boolean)
#else
#define CVMD_arrayCopyBoolean(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaBoolean, Boolean)
#endif

#ifdef CVMgcimplArrayCopyShort
#define CVMD_arrayCopyShort CVMgcimplArrayCopyShort
#elif defined(CVMGC_HAS_NO_SHORT_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_SHORT_WRITE_BARRIER)
#define CVMD_arrayCopyShort(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaShort, Short)
#else
#define CVMD_arrayCopyShort(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaShort, Short)
#endif

#ifdef CVMgcimplArrayCopyChar
#define CVMD_arrayCopyChar CVMgcimplArrayCopyChar
#elif defined(CVMGC_HAS_NO_CHAR_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_CHAR_WRITE_BARRIER)
#define CVMD_arrayCopyChar(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaChar, Char)
#else
#define CVMD_arrayCopyChar(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaChar, Char)
#endif

#ifdef CVMgcimplArrayCopyInt
#define CVMD_arrayCopyInt CVMgcimplArrayCopyInt
#elif defined(CVMGC_HAS_NO_INT_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_INT_WRITE_BARRIER)
#define CVMD_arrayCopyInt(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaInt, Int)
#else
#define CVMD_arrayCopyInt(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaInt, Int)
#endif

#ifdef CVMgcimplArrayCopyFloat
#define CVMD_arrayCopyFloat CVMgcimplArrayCopyFloat
#elif defined(CVMGC_HAS_NO_FLOAT_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_FLOAT_WRITE_BARRIER)
#define CVMD_arrayCopyFloat(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaFloat, Float)
#else
#define CVMD_arrayCopyFloat(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaFloat, Float)
#endif

#ifdef CVMgcimplArrayCopyRef
#define CVMD_arrayCopyRef CVMgcimplArrayCopyRef
#elif defined(CVMGC_HAS_NO_REF_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_REF_WRITE_BARRIER)
#define CVMD_arrayCopyRef(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMObject*, Ref)
#else
#define CVMD_arrayCopyRef(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMObject*, Ref)
#endif

#ifdef CVMgcimplArrayCopyLong
#define CVMD_arrayCopyLong CVMgcimplArrayCopyLong
#elif defined(CVMGC_HAS_NO_64BIT_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_64BIT_WRITE_BARRIER)
#define CVMD_arrayCopyLong(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaLong, Long)
#else
#define CVMD_arrayCopyLong(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaLong, Long)
#endif

#ifdef CVMgcimplArrayCopyDouble
#define CVMD_arrayCopyDouble CVMgcimplArrayCopyDouble
#elif defined(CVMGC_HAS_NO_64BIT_READ_BARRIER) && \
      defined(CVMGC_HAS_NO_64BIT_WRITE_BARRIER)
#define CVMD_arrayCopyDouble(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultNoBarrierArrayCopy(srcArr, srcIdx, dstArr, dstIdx, \
                                         len, CVMJavaDouble, Double)
#else
#define CVMD_arrayCopyDouble(srcArr, srcIdx, dstArr, dstIdx, len) \
    CVMDprivateDefaultArrayCopy(srcArr, srcIdx, dstArr, dstIdx, len, \
                                     CVMJavaDouble, Double)
#endif


/*
 * The location of an object field, with "slot index" off_.
 */
#define CVMDprivate_fieldLoc32(o_, off_) \
    ((CVMJavaVal32 volatile *)(o_) + (off_))

/*
 * Typed read, write, 32-bit, with appropriate R/W barriers.
 */
#define CVMDprivate_typedRead(o_, off_, item_, barrKind_, type_)	\
    {									\
	type_ volatile *fieldLoc_ = 					\
           (type_ volatile *)CVMDprivate_fieldLoc32(o_, off_);		\
	CVMgcimplReadBarrier##barrKind_((o_), (fieldLoc_));		\
        (item_) = *fieldLoc_;						\
    }

#define CVMDprivate_typedWrite(o_, off_, item_, barrKind_, type_) 	 \
    {									 \
	type_ volatile *fieldLoc_ = 					 \
	    (type_ volatile *)CVMDprivate_fieldLoc32(o_, off_);	 	 \
	CVMgcimplWriteBarrier##barrKind_((o_), (fieldLoc_), (item_));	 \
        *fieldLoc_ = (item_);						 \
    }

#ifdef __cplusplus
#define ASSIGN_VAL32(lhs, rhs)	((lhs).raw = (rhs).raw)
#else
#define ASSIGN_VAL32(lhs, rhs)	((lhs) = (rhs))
#endif

/*
 * Weakly-typed (non-reference) read, write, 32-bit with R/W barriers.
 *
 * Make generic 32-bit barriers look like Int barriers.
 */
#define CVMD_fieldRead32(o_, off_, item_)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	CVMgcimplReadBarrierInt((o_), ((CVMJavaInt*)(fieldLoc_)));	     \
	ASSIGN_VAL32((item_), *fieldLoc_);				     \
    }

#define CVMD_fieldWrite32(o_, off_, item_)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	CVMgcimplWriteBarrierInt((o_), ((CVMJavaInt*)(fieldLoc_)),	     \
				 (item_).i);				     \
	ASSIGN_VAL32(*fieldLoc_, (item_));				     \
    }

/*
 * Weakly-typed read, write, 64-bit
 */
#define CVMD_fieldRead64(o_, off_, location_)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	union {CVMUint32 x;} tmp_[2];					     \
	CVMgcimplReadBarrier64((o_), (fieldLoc_));			     \
        tmp_[0].x = (&(fieldLoc_)->raw)[0];				     \
        tmp_[1].x = (&(fieldLoc_)->raw)[1];				     \
        CVMmemCopy64(&(location_)->raw, &tmp_[0].x);			     \
    }

#define CVMD_fieldWrite64(o_, off_, location_)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	union {CVMUint32 x;} tmp_[2];					     \
	CVMgcimplWriteBarrier64((o_), (fieldLoc_), (location_));	     \
        CVMmemCopy64(&tmp_[0].x, &(location_)->raw);			     \
        (&(fieldLoc_)->raw)[0] = tmp_[0].x;				     \
        (&(fieldLoc_)->raw)[1] = tmp_[1].x;				     \
    }

#define CVMD_fieldReadRef(o_, off_, item_)     \
    CVMDprivate_typedRead(o_, off_, item_, Ref, CVMObject*)
#ifndef CVM_JAVASE_CLASS_HAS_REF_FIELD
#define CVMD_fieldWriteRef(o_, off_, item_)    \
    CVMDprivate_typedWrite(o_, off_, item_, Ref, CVMObject*)
#else
/*
 * If the object is in ROM then we cannot mark the cardtable. For JAVA
 * SE 1.5 and 1.6, there are reference fields in java.lang.Class.
 */
#define CVMD_fieldWriteRef(o_, off_, item_)    \
    {									 \
	CVMObject* volatile *fieldLoc_ = 				 \
	    (CVMObject* volatile *)CVMDprivate_fieldLoc32(o_, off_);	 \
        if (!CVMobjectIsInROM(o_)) {					 \
	    CVMgcimplWriteBarrierRef((o_), (fieldLoc_), (item_));	 \
        }                                                                \
        *fieldLoc_ = (item_);						 \
    }
#endif

#define CVMD_fieldReadInt(o_, off_, item_)     \
    CVMDprivate_typedRead(o_, off_, item_, Int, CVMJavaInt)
#define CVMD_fieldWriteInt(o_, off_, item_)    \
    CVMDprivate_typedWrite(o_, off_, item_, Int, CVMJavaInt)

#define CVMD_fieldReadFloat(o_, off_, item_)     \
    CVMDprivate_typedRead(o_, off_, item_, Float, CVMJavaFloat)
#define CVMD_fieldWriteFloat(o_, off_, item_)    \
    CVMDprivate_typedWrite(o_, off_, item_, Float, CVMJavaFloat)

#define CVMD_fieldReadLong(o_, off_, lres)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	CVMgcimplReadBarrier64((o_), (fieldLoc_));			     \
        *(CVMJavaLong volatile*)&(lres) =				     \
            CVMjvm2Long((CVMUint32*)&(fieldLoc_)->raw);			     \
    }

#define CVMD_fieldWriteLong(o_, off_, lval)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	CVMUint32 tmp_[2];						     \
	CVMgcimplWriteBarrier64((o_), (fieldLoc_),			     \
				((CVMJavaVal32*)(&(val64_))));		     \
        CVMlong2Jvm(tmp_, (lval));				     	     \
        (&(fieldLoc_)->raw)[0] = tmp_[0];				     \
        (&(fieldLoc_)->raw)[1] = tmp_[1];				     \
    }

#define CVMD_fieldReadDouble(o_, off_, dres)				     \
    {									     \
	CVMJavaVal32 volatile* fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	CVMgcimplReadBarrier64((o_), (fieldLoc_));			     \
        *(CVMJavaDouble volatile*)&(dres) =				     \
            CVMjvm2Double((CVMUint32*)&(fieldLoc_)->raw);		     \
    }

#define CVMD_fieldWriteDouble(o_, off_, dval)				     \
    {									     \
	CVMJavaVal32 volatile *fieldLoc_ = CVMDprivate_fieldLoc32(o_, off_); \
	CVMUint32 tmp_[2];						     \
	CVMgcimplWriteBarrier64((o_), (fieldLoc_),			     \
				((CVMJavaVal32*)(&(val64_))));		     \
        CVMdouble2Jvm(tmp_, (dval)); \
        (&(fieldLoc_)->raw)[0] = tmp_[0];				     \
        (&(fieldLoc_)->raw)[1] = tmp_[1];				     \
    }

#define CVMD_fieldReadAddr(o_, off_, item_)     \
    CVMDprivate_typedRead(o_, off_, item_, Addr, CVMAddr)
#define CVMD_fieldWriteAddr(o_, off_, item_)    \
    CVMDprivate_typedWrite(o_, off_, item_, Addr, CVMAddr)

/* Array operations */

/* These are a bit different in that the basic types that are less
   than 32-bit values do not all get coalesced into CVMJavaInt's. Let's
   just make all array operations typed. */

#define CVMDprivate_arrayElemLoc(arr_, idx_)    (&((arr_)->elems[(idx_)]))

/*
 * Strongly-typed read, write, width <= 32-bit
 *
 * These are "package-private" (in that they are used here and in 
 * indirectmem.h)
 */
#define CVMDprivate_arrayRead(arr_, idx_, item_, barrKind_, type_)	\
    {									\
	type_ volatile *elemLoc_ =					\
            (type_ volatile *)CVMDprivate_arrayElemLoc(arr_, idx_);	\
        CVMassert(idx_ < CVMD_arrayGetLength(arr_));			\
	CVMgcimplReadBarrier##barrKind_(((CVMObject*)arr_),		\
					(elemLoc_));			\
        (item_) = *elemLoc_;						\
    }

#define CVMDprivate_arrayWrite(arr_, idx_, item_, barrKind_, type_)	\
    {									\
	type_ volatile *elemLoc_ =					\
            (type_ volatile *)CVMDprivate_arrayElemLoc(arr_, idx_);	\
        CVMassert(idx_ < CVMD_arrayGetLength(arr_));			\
	CVMgcimplWriteBarrier##barrKind_(((CVMObject*)arr_),		\
					 (elemLoc_),			\
					 (item_));			\
        *elemLoc_ = (item_);						\
    }

#define CVMDprivate_arrayRW(rw_, arr_, idx_, item_, barrKind_, type_) \
    CVMDprivate_array##rw_(arr_, idx_, item_, barrKind_, type_)

#define CVMD_arrayReadRef(arr_, idx_, item_)   \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
			Ref, CVMObject*)

#define CVMD_arrayWriteRef(arr_, idx_, item_)   \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Ref, CVMObject*)

#define CVMD_arrayReadInt(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
                        Int, CVMJavaInt)

#define CVMD_arrayWriteInt(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Int, CVMJavaInt)

#define CVMD_arrayReadFloat(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
			Float, CVMJavaFloat)

#define CVMD_arrayWriteFloat(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Float, CVMJavaFloat)

#define CVMD_arrayReadBoolean(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
			Boolean, CVMJavaBoolean)

#define CVMD_arrayWriteBoolean(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Boolean, CVMJavaBoolean)

#define CVMD_arrayReadByte(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
			Byte, CVMJavaByte)

#define CVMD_arrayWriteByte(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Byte, CVMJavaByte)

#define CVMD_arrayReadShort(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
			Short,CVMJavaShort)

#define CVMD_arrayWriteShort(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Short, CVMJavaShort)

#define CVMD_arrayReadChar(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Read, arr_, idx_, item_, \
			Char, CVMJavaChar)

#define CVMD_arrayWriteChar(arr_, idx_, item_) \
    CVMDprivate_arrayRW(Write, arr_, idx_, item_, \
			Char, CVMJavaChar)

#define CVMD_arrayReadLong(arr_, idx_, lres)				   \
    {									   \
	CVMJavaVal32 volatile *elemLoc_ =				   \
            (CVMJavaVal32 volatile *)CVMDprivate_arrayElemLoc(arr_, idx_); \
	CVMgcimplReadBarrier64(((CVMObject*)(arr_)), (elemLoc_));	   \
        *(CVMJavaLong  volatile *)&(lres) =				   \
            CVMjvm2Long((CVMUint32*)&(elemLoc_)->raw);			   \
    }

#define CVMD_arrayWriteLong(arr_, idx_, lval)				  \
    {									  \
	CVMJavaVal32 volatile *elemLoc_ =				  \
            (CVMJavaVal32 volatile*)CVMDprivate_arrayElemLoc(arr_, idx_); \
	CVMUint32 tmp_[2];						  \
	CVMgcimplWriteBarrier64(((CVMObject*)(arr_)), (elemLoc_),	  \
				((CVMJavaVal32*)(&(val64_))));		  \
        CVMlong2Jvm(tmp_, (lval));					  \
        (&(elemLoc_)->raw)[0] = tmp_[0];				  \
        (&(elemLoc_)->raw)[1] = tmp_[1];				  \
    }

#define CVMD_arrayReadDouble(arr_, idx_, dres)				   \
    {									   \
	CVMJavaVal32 volatile *elemLoc_ =				   \
            (CVMJavaVal32 volatile *)CVMDprivate_arrayElemLoc(arr_, idx_); \
	CVMgcimplReadBarrier64(((CVMObject*)(arr_)), (elemLoc_));	   \
        *(CVMJavaDouble volatile*)&(dres) =				   \
            CVMjvm2Double((CVMUint32*)&(elemLoc_)->raw);		   \
    }

#define CVMD_arrayWriteDouble(arr_, idx_, dval)				   \
    {									   \
	CVMJavaVal32 volatile *elemLoc_ =				   \
            (CVMJavaVal32 volatile *)CVMDprivate_arrayElemLoc(arr_, idx_); \
	CVMUint32 tmp_[2];						   \
	CVMgcimplWriteBarrier64(((CVMObject*)(arr_)), (elemLoc_),	   \
				((CVMJavaVal32*)(&(val64_))));		   \
        CVMdouble2Jvm(tmp_, (dval));					   \
        (&(elemLoc_)->raw)[0] = tmp_[0];				   \
        (&(elemLoc_)->raw)[1] = tmp_[1];				   \
    }

#define CVMD_arrayGetLength(arr_)     ((arr_)->length)

/*
 * Request a GC-safe point.
 * Called from GC-unsafe code.
 * Block on safe-->unsafe transition.  Don't suspend safe threads.
 *
 * saveAction_      save state for GC
 */
#define CVMD_gcSafeCheckPoint(ee_, saveAction_, restoreAction_)	\
    {                                                           \
        CVMthreadSchedHook(CVMexecEnv2threadID(ee_));           \
        if (CVMD_gcSafeCheckRequest(ee_)) {                     \
            saveAction_;                                        \
            CVMD_gcRendezvous(ee_, CVM_TRUE);                   \
            restoreAction_;                                     \
        }                                                       \
    }

/*
 * Execute an action that may end up blocking or taking a long time.
 * Called from GC-unsafe code.
 * Block on safe-->unsafe transition.  Don't suspend safe threads.
 *
 * safeAction_     gcsafe action that may end up blocking or
 *                 taking a long time.
 */

#define CVMD_gcSafeExec(ee_, safeAction_)                               \
    {                                                                   \
        CVMthreadSchedHook(CVMexecEnv2threadID(ee_));                   \
        CVMDprivate_gcSafe(ee_);                                        \
        if (CVMD_gcSafeCheckRequest(ee_)) {                             \
            CVMD_gcRendezvous(ee_, CVM_FALSE);                          \
        }                                                               \
        safeAction_;                                                    \
        CVMDprivate_gcUnsafe(ee_);                                      \
        if (CVMD_gcSafeCheckRequest(ee_)) {                             \
            CVMD_gcRendezvous(ee_, CVM_TRUE);                           \
        }                                                               \
    }

/*
 * Execute a GC-unsafe action.
 * Called from GC-safe code.
 * Block on safe-->unsafe transition.  Don't suspend safe threads.
 *
 * unsafeAction_   gc-unsafe action.
 */

#define CVMD_gcUnsafeExec(ee_, unsafeAction_)                           \
    {                                                                   \
        CVMthreadSchedHook(CVMexecEnv2threadID(ee_));                   \
        CVMDprivate_gcUnsafe(ee_);                                      \
        if (CVMD_gcSafeCheckRequest(ee_)) {                             \
            CVMD_gcRendezvous(ee_, CVM_TRUE);                           \
        }                                                               \
        unsafeAction_;                                                  \
        CVMDprivate_gcSafe(ee_);                                        \
        if (CVMD_gcSafeCheckRequest(ee_)) {                             \
            CVMD_gcRendezvous(ee_, CVM_FALSE);                          \
        }                                                               \
    }

#define CVMDprivate_gcSafe(ee_)						\
        CVMtraceGcSafety(("gc-safe\n"));				\
	CVMcsBecomeConsistent(CVM_GCSAFE, CVM_TGCSAFE(ee_));

#define CVMDprivate_gcUnsafe(ee_)					\
        CVMtraceGcSafety(("gc-unsafe\n"));	       		        \
	CVMcsBecomeInconsistent(CVM_GCSAFE, CVM_TGCSAFE(ee_));

/*
 * These macros are used by CVMjniGetPrimitiveArrayCritical and
 * CVMjniReleasePrimitiveArrayCritical. They allow us to avoid
 * having to copy arrays by keeping gc disabled while the JNI method
 * maintains a direct pointer to the array.
 */
#define CVMD_gcEnterCriticalRegion(ee_)			\
	if ((ee_)->criticalCount == 0) {		\
	    CVMDprivate_gcUnsafe(ee_);			\
	    if (CVMD_gcSafeCheckRequest(ee_)) {		\
	        CVMD_gcRendezvous(ee_, CVM_TRUE);	\
	    }						\
	}						\
	(ee_)->criticalCount++;

#define CVMD_gcExitCriticalRegion(ee_)			\
	(ee_)->criticalCount--;				\
	if ((ee_)->criticalCount == 0) {		\
	    CVMDprivate_gcSafe(ee_);			\
	    if (CVMD_gcSafeCheckRequest(ee_)) {		\
	        CVMD_gcRendezvous(ee_, CVM_FALSE);	\
	    }						\
	}

#ifdef CVM_TRACE
#define CVMD_gcSafeCheckRequest(ee_)					\
	(CVMtraceGcSafety(("gc-safe?\n")),	       		        \
         CVMeeMarkHasRun(ee_),                                          \
	 (CVMcsCheckRequest(CVM_GCSAFE)))
#else
#define CVMD_gcSafeCheckRequest(ee_)    				\
        (CVMeeMarkHasRun(ee_),                                          \
         CVMcsCheckRequest(CVM_GCSAFE))
#endif

#define CVMD_gcRendezvous(ee_, entry)					\
	CVMcsRendezvous((ee_), CVM_GCSAFE, CVM_TGCSAFE(ee_), (entry));

#define CVMD_gcBecomeSafeAll(ee_)					\
	    CVMcsReachConsistentState((ee_), CVM_GC_SAFE);

#define CVMD_gcAllowUnsafeAll(ee_)					\
	    CVMcsResumeConsistentState((ee_), CVM_GC_SAFE);

#define CVMD_gcAllThreadsAreSafe()                                      \
        CVMcsAllThreadsAreConsistent(CVM_GCSAFE)

/*
 * Query current gc safety status
 */

#define CVMD_isgcSafe(ee_)   CVMcsIsConsistent(CVM_TGCSAFE(ee_))
#define CVMD_isgcUnsafe(ee_) CVMcsIsInconsistent(CVM_TGCSAFE(ee_))

/*
 * These are private macros for convenience
 */

#define CVM_GCSAFE		CVM_CSTATE(CVM_GC_SAFE)
#define CVM_TGCSAFE(ee)		CVM_TCSTATE((ee), CVM_GC_SAFE)

#endif /* _INCLUDED_DIRECTMEM_H */
