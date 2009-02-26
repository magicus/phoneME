/*
 * @(#)float.h	1.14 06/10/10
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

#ifndef _ANSI_FLOAT_H
#define _ANSI_FLOAT_H

#ifdef USE_ANSI_FMOD
#include <math.h>		 /* fmod */
#endif

/*
 * See src/share/javavm/include/porting/float.h for full documentation.
 */

#define CVMfloatAdd(op1, op2)	((op1) + (op2))
#define CVMfloatSub(op1, op2)	((op1) - (op2))
#define CVMfloatMul(op1, op2)	((op1) * (op2))
#define CVMfloatDiv(op1, op2)	((op1) / (op2))

#ifdef USE_ANSI_FMOD
#define CVMfloatRem(op1, op2)	fmod((op1), (op2))
#endif

/*
 * Unary operations
 */

#ifndef CVMfloatNeg
#define CVMfloatNeg(op)	(-(op))
#endif

/*
 * Float Comparisons (returning int value 0, 1, or -1) 
 */

#ifdef USE_ANSI_FCOMPARE
#define CVMfloatCompare(op1, op2, opcode_value) \
    (((op1) < (op2)) ? -1 : \
     ((op1) > (op2)) ? 1 : \
     ((op1) == (op2)) ? 0 : \
     ((opcode_value) == -1 || (opcode_value) == 1) ? (opcode_value) :\
     0)
#endif

/*
 * Float Conversions:
 */

#define CVMfloat2Double(val)	((CVMJavaDouble)(val)) 

#define CVMfloat2Int(val)	(float2Int(val))

/*
 * JAVA_COMPLIANT_f2i, NAN_CHECK_f2i, or neither (but not both) may be
 * defined.
 *
 */

#ifdef JAVA_COMPLIANT_f2i

#define float2Int(val)    ((CVMJavaInt)(val))

#elif (defined(NAN_CHECK_f2i))

#define float2Int(val)    ((val) != (val) ? 0 : (CVMJavaInt)(val))

#else

extern CVMJavaInt float2Int(CVMJavaFloat d);

#endif

#endif /* _ANSI_FLOAT_H */
