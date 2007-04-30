/* 
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
 *
 */


#ifndef PISCES_DEFS_H
#define PISCES_DEFS_H

#include <jni.h>

#define XNI_TRUE JNI_TRUE
#define XNI_FALSE JNI_FALSE

#ifndef INTEGER_MIN_VALUE
#define INTEGER_MIN_VALUE 0x80000000
#endif

#ifndef INTEGER_MAX_VALUE
#define INTEGER_MAX_VALUE 0x7fffffff
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef INLINE
#define INLINE
#endif

#define MIN_X 0
#define MIN_Y 1
#define MAX_X 2
#define MAX_Y 3

// extern double CVMfdlibmFloor(double);
#define floor CVMfdlibmFloor

/*
extern double JFP_lib_floor(double x);
#define floor JFP_lib_floor
*/

#endif
