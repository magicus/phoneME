/*
 * @(#)endianness.h	1.12 06/10/10
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

#ifndef _INCLUDED_PORTING_ENDIANNESS_H
#define _INCLUDED_PORTING_ENDIANNESS_H

#include "javavm/include/porting/defs.h"
/*
 * The platform defines the CVM_ENDIANNESS and CVM_DOUBLE_ENDIANNESS
 * macros to be either CVM_LITTLE_ENDIAN or CVM_BIG_ENDIAN.
 */

#define CVM_LITTLE_ENDIAN	1234
#define CVM_BIG_ENDIAN		4321

#include CVM_HDR_ENDIANNESS_H

/*
 * Sanity check the setting. 
 */
#if (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN)
#elif (CVM_ENDIANNESS == CVM_BIG_ENDIAN)
#else
#error CVM_ENDIANNESS must be defined as CVM_LITTLE_ENDIAN or CVM_BIG_ENDIAN
#endif

#if (CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN)
#elif (CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN)
#else
#error CVM_DOUBLE_ENDIANNESS must be defined as CVM_LITTLE_ENDIAN or CVM_BIG_ENDIAN
#endif

#endif /* _INCLUDED_PORTING_ENDIANNESS_H */
