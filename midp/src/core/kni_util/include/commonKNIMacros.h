/*
 *   
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
#ifndef _COMMONKNIMACROS_H
#define _COMMONKNIMACROS_H

/**
 * @defgroup core_kni KNI Utilities External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_kni
 *
 * @brief Generic macros to interact with KNI and its data.
 */

#include <kni.h>

#include <ROMStructs.h>

/**
 * Converts the given <code>jobject</code> to a pointer of the given
 * type.
 */
#define unhand(__type, __ptr)  (*((__type**)(void*)(__ptr)))

/**
 * Converts the given pointer to a <code>jobject</code>.
 */
#define hand(__ptr)  (((jobject)(void*)&(__ptr)))

/* Macros to handle primitive array types */
/**
 * Converts the given <code>jobject</code> to array of <tt>jbyte</tt>.
 * Such a spophisticated furmula is used to prevent erros issuing by 
 * some smart compilers that can otherwise track number of elements 
 * in <tt>jbyte_array</tt>, which is fake. 
 * Usage:
 * <code> 
 * jbyte *arr = JavaByteArray(handle);
 * arr = &JavaByteArray(handle)[2];
 * jbyte b = JavaByteArray(handle)[3];
 * arr[4] = b;
 * </code>
 */
#define JavaByteArray(__handle)  \
    ( (jbyte*) &(unhand(jbyte_array, (__handle))->elements[0]) )

/**
 * Converts the given <code>jobject</code> to array of <tt>jchar</tt>.
 * Such a spophisticated furmula is used to prevent erros issuing by 
 * some smart compilers that can otherwise track number of elements 
 * in <tt>jchar_array</tt>, which is fake. 
 */
#define JavaCharArray(__handle)  \
    ( (jchar*) &(unhand(jchar_array, (__handle))->elements[0]) )

/**
 * Converts the given <code>jobject</code> to array of <tt>jshort</tt>.
 * Such a spophisticated furmula is used to prevent erros issuing by 
 * some smart compilers that can otherwise track number of elements 
 * in <tt>jshort_array</tt>, which is fake. 
 */
#define JavaShortArray(__handle) \
    ( (jshort*) &(unhand(jshort_array, (__handle))->elements[0]) )

/**
 * Converts the given <code>jobject</code> to array of <tt>jint</tt>.
 * Such a spophisticated furmula is used to prevent erros issuing by 
 * some smart compilers that can otherwise track number of elements 
 * in <tt>jint_array</tt>, which is fake. 
 */
#define JavaIntArray(__handle) \
    ( (jint*) &(unhand(jint_array, (__handle))->elements[0]) )

#endif /*_COMMONKNIMACROS_H*/
