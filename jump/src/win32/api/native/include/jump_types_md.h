/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef __JUMP_TYPES_MD_H
#define __JUMP_TYPES_MD_H

/*
 * The platform defines its own
 *
 * uint32                     Unsigned 32-bit integer value
 * int32                      Signed 32-bit integer value
 * uint16                     Unsigned 16-bit integer value
 * int16                      Signed 16-bit integer value
 * uint8                      Unsigned 8-bit integer value
 * int8                       Signed 8-bit integer value
 *
 * JUMPPlatformCString        A handle to a platform C string
 */

typedef long int32;
typedef unsigned int uint32;

typedef short int16;
typedef unsigned short uint16;

typedef signed char int8;
typedef unsigned char uint8;

typedef unsigned char* JUMPPlatformCString;

#endif /* __JUMP_TYPES_MD_H */
