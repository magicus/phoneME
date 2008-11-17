/*
 * @(#)float.h	1.19 06/10/10
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

#ifndef _INCLUDED_PORTING_FLOAT_H
#define _INCLUDED_PORTING_FLOAT_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * Java floating-point float value manipulation.
 *
 * The result argument is, once again, an lvalue.
 *
 * Arithmetic:
 *
 * The functions below follow the semantics of the
 * fadd, fsub, fmul, fdiv, and frem bytecodes,
 * respectively.
 */

CVMJavaFloat CVMfloatAdd(CVMJavaFloat op1, CVMJavaFloat op2);
CVMJavaFloat CVMfloatSub(CVMJavaFloat op1, CVMJavaFloat op2);
CVMJavaFloat CVMfloatMul(CVMJavaFloat op1, CVMJavaFloat op2);
CVMJavaFloat CVMfloatDiv(CVMJavaFloat op1, CVMJavaFloat op2);

/*
 * #define USE_NATIVE_FREM if the platform provides a non-ANSI frem
 * #define USE_ANSI_FMOD if the platform has an ANSI fmod for
 * implementing CVMfloatRem()
 *
 * Otherwise a default implementation is provided 
 */
CVMJavaFloat CVMfloatRem(CVMJavaFloat op1, CVMJavaFloat op2);

/*
 * Unary:
 *
 * Return the negation of "op" (-op), according to
 * the semantics of the fneg bytecode.
 */

CVMJavaFloat CVMfloatNeg(CVMJavaFloat op);

/*
 * Comparisons (returning an int value: 0, 1, or -1)
 *
 * Between operands:
 *
 * Compare "op1" and "op2" according to the semantics of the
 * "fcmpl" (direction is -1) or "fcmpg" (direction is 1) bytecodes.
 *
 * #define USE_NATIVE_FCOMPARE if the platform provides a non-ANSI compare.
 * #define USE_ANSI_FCOMPARE if the platform has an ANSI compare for
 * implementing CVMfloatCompare().
 *
 * Otherwise a default implementation is provided.
 */

CVMInt32 CVMfloatCompare(CVMJavaFloat op1, CVMJavaFloat op2,
			 CVMInt32 direction);

/*
 * Conversion:
 */

/* 
 * Convert float to int, according to "f2i" bytecode semantics
 */

CVMJavaInt CVMfloat2Int(CVMJavaFloat op);

/*
 * Convert float to double, according to "f2d" bytecode semantics
 */

CVMJavaDouble CVMfloat2Double(CVMJavaFloat op);

/*
 * Convert float to long, according to "f2l" bytecode semantics
 */
extern CVMJavaLong CVMfloat2Long(CVMJavaFloat op);

#include CVM_HDR_FLOAT_H

#endif /* _INCLUDED_PORTING_FLOAT_H */
