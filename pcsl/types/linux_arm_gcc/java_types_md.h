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

/**
 * @file
 * @ingroup types
 * @brief Basic types for linux_arm_gcc configuration
 */ 

#if !defined _JAVA_TYPES_H_
# error "Never include <java_types_md.h> directly; use <java_types.h> instead."
#endif

#ifndef _JAVA_TYPES_MD_H_
#define _JAVA_TYPES_MD_H_

#ifndef _JAVASOFT_JNI_H_
/** Byte parameter type. */
typedef signed char jbyte;

/** Char parameter type. */
typedef unsigned short jchar;

/** Integer parameter type. */
typedef int         jint;

/** Long parameter type. */
typedef long long   jlong;
#endif /* _JAVASOFT_JNI_H_ */

/** Platform-specific type specifier for 64-bit integer */
#define PCSL_LLD "%lld"

#endif /* !_JAVA_TYPES_MD_H_ */
