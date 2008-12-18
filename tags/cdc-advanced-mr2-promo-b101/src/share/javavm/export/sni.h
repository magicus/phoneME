/*
 * @(#)sni.h	1.2 06/10/10 10:03:30
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
 */

#ifndef _JAVASOFT_SNI_H_
#define _JAVASOFT_SNI_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _JAVASOFT_KNI_H_
#include "javavm/export/kni.h"
#endif

#define SNI_BEGIN_RAW_POINTERS
#define SNI_END_RAW_POINTERS

#define SNI_BOOLEAN_ARRAY   CVM_T_BOOLEAN /* 0x04  0b0100 */
#define SNI_CHAR_ARRAY      CVM_T_CHAR    /* 0x05  0b0101 */
#define SNI_FLOAT_ARRAY     CVM_T_FLOAT   /* 0x06  0b0110 */
#define SNI_DOUBLE_ARRAY    CVM_T_DOUBLE  /* 0x07  0b0111 */
#define SNI_BYTE_ARRAY      CVM_T_BYTE    /* 0x08  0b1000 */
#define SNI_SHORT_ARRAY     CVM_T_SHORT   /* 0x09  0b1001 */
#define SNI_INT_ARRAY       CVM_T_INT     /* 0x0a  0b1010 */
#define SNI_LONG_ARRAY      CVM_T_LONG    /* 0x0b  0b1011 */
#define SNI_OBJECT_ARRAY    CVM_T_VOID + 100
#define SNI_STRING_ARRAY    CVM_T_VOID + 101

/**
 * Create an array of the given size. The elements contained in this
 * array is specified by the type parameter, which must be one of
 * the SNI_XXX_ARRAY type specified above. For example, the calls:
 *
 *     SNI_NewArray(SNI_BOOLEAN_ARRAY, 10, handle1);
 *     SNI_NewArray(SNI_OBJECT_ARRAY,  20, handle2);
 *     SNI_NewArray(SNI_STRING_ARRAY,  30, handle3);
 *
 * are equivalent to the Java code:
 *
 *     array1 = new boolean[10];
 *     array2 = new Object[10];
 *     array3 = new String[10];
 *
 * Note that SNI_OBJECT_ARRAY and SNI_STRING_ARRAY are provided for
 * the creation these two most comply used object arrays. For other
 * object array types, please use KNI_NewObjectArray().
 */

KNIEXPORT void
SNI_NewArrayImpl(CVMExecEnv* ee, jint type, jint size, jarray arrayHandle);

#define SNI_NewArray(type, size, arrayHandle)	\
    SNI_NewArrayImpl(_ee, type, size, arrayHandle)


/**
 * Create an object array of the given size. The elements contained in this
 * array is by the elementType variable. For example, the following code:
 *
 *     KNI_StartHandles(2);
 *     KNI_DeclareHandle(fooClass);
 *     KNI_DeclareHandle(fooArray);
 *
 *     KNI_FindClass("Foo", fooClass);
 *     SNI_NewObjectArray(fooClass, 10, fooArray);
 *     KNI_EndHandlesAndReturnObject(fooArray);
 *
 * Is equivalent to the Java code:
 *
 *     fooArray = new Foo[10];
 */
KNIEXPORT void
SNI_NewObjectArrayImpl(CVMExecEnv* ee,
                       jclass elementType, jint size,
                       jarray arrayHandle);

#define SNI_NewObjectArray(elementType, size, arrayHandle) \
    SNI_NewObjectArrayImpl(_ee, elementType, size, arrayHandle)

#ifdef __cplusplus
}
#endif

#endif /* _JAVASOFT_SNI_H_ */
