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

#ifndef _JAVA_TYPES_H_
#define _JAVA_TYPES_H_

/**
 * @defgroup types Parameter Type Declarations
 */

/**
 * @file
 * @ingroup types
 * @brief Basic types and constants for PCSL
 */ 

#ifndef KNI_PRIMITIVE_TYPES
#define KNI_PRIMITIVE_TYPES

#include <java_types_md.h>
#ifndef _JAVASOFT_JNI_H_

/** Boolean parameter type. */
typedef unsigned char   jboolean;

/** Short parameter type. */
typedef short           jshort;

/** Float parameter type. */
typedef float           jfloat;

/** Double parameter type. */
typedef double          jdouble;

/** Integer parameter type. */
typedef jint            jsize;
#endif /* _JAVASOFT_JNI_H_ */

#endif /* KNI_PRIMITIVE_TYPES */

/** Constant boolean FALSE value. */
#define PCSL_FALSE 0

/** Constant boolean TRUE value. */
#define PCSL_TRUE  1

#endif /* _JAVA_TYPES_H_ */
