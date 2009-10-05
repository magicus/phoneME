/*
 * @(#)jlong.h	1.21 06/10/10
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

#ifndef _JLONG_H_
#define _JLONG_H_

/*
 * JDK-compatability interface
 */

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/endianness.h"

#define jlong_zero		CVMlongConstZero()
#define jlong_to_jint		CVMlong2Int
#define jint_to_jlong		CVMint2Long
#define ptr_to_jlong		CVMvoidPtr2Long
#define jlong_to_ptr		CVMlong2VoidPtr
#define jlong_shl(l, b)		((l) = CVMlongShl((l), (b)))
#define jlong_add(l1, l2)       CVMlongAdd(l1, l2)
#define jlong_sub(l1, l2)       CVMlongSub(l1, l2)
#define jlong_mul(l1, l2)       CVMlongMul(l1, l2)
#define jlong_div(l1, l2)       CVMlongDiv(l1, l2)
#define jlong_low(l)            CVMlong2Int(l)
#define jlong_high(l)           CVMlong2Int(CVMlongShr(l, 32))

#ifdef JAVASE
#define long_to_jlong           CVMint2Long
#endif

/* 
 * Useful on machines where jlong and jdouble have different endianness.
 */

#if (CVM_DOUBLE_ENDIANNESS == CVM_ENDIANNESS)

#define jlong_to_jdouble_bits(longBitsPtr)
#define jdouble_to_jlong_bits(doubleBitsPtr)

#else

#define jlong_to_jdouble_bits(longBitsPtr) {			\
    union {							\
	jdouble doubleBits;					\
	jlong   longBits;					\
    } val64;							\
    val64.doubleBits = CVMlongBits2Double(*longBitsPtr);	\
    *longBitsPtr = val64.longBits;				\
}

#define jdouble_to_jlong_bits(doubleBitsPtr) {			\
    union {							\
	jdouble doubleBits;					\
	jlong   longBits;					\
    } val64;							\
    val64.longBits = CVMdouble2LongBits(*doubleBitsPtr);	\
    *doubleBitsPtr = val64.doubleBits;				\
}

#endif

#endif
