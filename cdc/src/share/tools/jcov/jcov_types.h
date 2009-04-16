/*
 * @(#)jcov_types.h	1.12 06/10/10
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

#ifndef _JCOV_TYPES_H
#define _JCOV_TYPES_H

#ifdef WIN32
#include <basetsd.h>
#endif

#ifdef _LP64
typedef int            INT32;
typedef unsigned int   UINT32;
#else
#ifndef WIN32
typedef long           INT32;
typedef unsigned long  UINT32;
#endif
#endif

#ifdef SOLARIS2
typedef intptr_t       INTPTR_T;
typedef uintptr_t      UINTPTR_T;
typedef ssize_t        SSIZE_T;
typedef size_t         SIZE_T;
#else
typedef int            INTPTR_T;
typedef unsigned int   UINTPTR_T;
#ifndef WIN32
typedef int            SSIZE_T;
typedef unsigned int   SIZE_T;
#endif
#endif

typedef short          INT16;
typedef unsigned short UINT16;
typedef char           INT8;
typedef unsigned char  UINT8;
typedef char           Bool;

#define TRUE 1
#define FALSE 0

#endif
