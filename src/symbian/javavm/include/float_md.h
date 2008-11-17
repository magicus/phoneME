/*
 * @(#)float_md.h	1.7 06/10/10
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

#ifndef _SYMBIAN_FLOAT_MD_H
#define _SYMBIAN_FLOAT_MD_H

extern void setFPMode(void);

#include "javavm/include/float_arch.h"

#include "portlibs/ansi_c/float.h"

#if defined(__GCC32__) || defined(__ARMCC__)

#include "portlibs/gcc_32_bit/float.h"

#elif defined(__VC32__)

#define longMin      (0x8000000000000000ui64)
#define longMax      (0x7fffffffffffffffui64)

#elif defined(__CW32__)

#define longMin      (0x8000000000000000LL)
#define longMax      (0x7fffffffffffffffLL)

#endif

#include <math.h>
#define CVMisNan(x)		isnan((x))
#define CVMisFinite(x)		finite((x))
#define CVMcopySign(x, y)	copysign((x), (y))

#endif /* _SYMBIAN_FLOAT_MD_H */
