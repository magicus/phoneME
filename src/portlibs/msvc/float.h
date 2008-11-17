/*
 * @(#)float.h	1.4 06/10/10
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

#ifndef _MSVC_FLOAT_H
#define _MSVC_FLOAT_H

#include "javavm/include/float_arch.h"

#ifdef USE_NATIVE_FCOMPARE
/*
 * Float Comparisons (returning int value 0, 1, or -1) 
 */
#define CVMfloatCompare(op1, op2, opcode_value) \
    ((CVMisNan((double)op1) || CVMisNan((double)op2)) ? (opcode_value) : \
     ((op1) < (op2)) ? -1 : \
     ((op1) > (op2)) ? 1 : \
     ((op1) == (op2)) ? 0 : \
     ((opcode_value) == -1 || (opcode_value) == 1) ? (opcode_value) :\
     0)
#endif

extern CVMJavaLong float2Long(CVMJavaFloat);
#define CVMfloat2Long(val)	float2Long((val))

#endif /* _MSVC_FLOAT_H */
