/*
 * @(#)defs.h	1.17 06/10/10
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
 * This file declares basic types. These are C types with the width
 * encoded in a one word name, and a mapping of Java basic types to C types.
 *
 * These types are hopefully the same on most 32-bit gcc targets.
 *
 */

#ifndef _GCC_32_BIT_DEFS_H
#define _GCC_32_BIT_DEFS_H

/*
 * Some basic C types to be used throughout the VM.
 *
 * See src/share/javavm/include/porting/defs.h for full documentation.
 *
 */

#if defined(CVM_64)
typedef long		CVMInt64;
#else
typedef long long	CVMInt64;
#endif
typedef int		CVMInt32;
typedef short		CVMInt16;
typedef signed char	CVMInt8;

/* 
 * Added a typedef for 64 bit unsigned long integer.
 */
#if defined(CVM_64)
typedef unsigned long      CVMUint64;
#else
typedef unsigned long long CVMUint64;
#endif
typedef unsigned int	CVMUint32;
typedef unsigned short	CVMUint16;
typedef unsigned char	CVMUint8;

typedef float		CVMfloat32;
typedef double		CVMfloat64;

/* 
 * make CVM ready to run on 64 bit platforms
 * To keep the source base common for 32 and 64 bit
 * platforms it is required to introduce a new type
 * CVMAddr, an integer big enough to hold a native pointer.
 * That meens 4 byte (int32) on 32 platforms and 8 byte
 * on 64 bit platforms. This type should be used at places
 * where pointer arithmetic and casting is required. Using
 * this type instead of the original CVMUint32 should
 * not change anything for the platforms linux32/win32 but
 * make the code better portable for 64 bit platforms.
 */
#if defined(CVM_64)
typedef unsigned long	CVMAddr;
#else
typedef unsigned int	CVMAddr;
#endif

typedef unsigned int    CVMSize;

#define CONST64(x) (x ## LL)
#define UCONST64(x) (x ## ULL)

#endif /* _GCC_32_BIT_DEFS_H */
