/*
 * @(#)doubleword.h	1.25 06/10/10
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

#ifndef _INCLUDED_PORTING_DOUBLEWORD_H
#define _INCLUDED_PORTING_DOUBLEWORD_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * 64-bit value access
 *
 * Convert 64-bit Java longs between native representation and JVM
 * two-word representation:
 */

/*
 * Read a 64-bit Java long in two-word "JVM" representation from "location"
 * and return the result in native representation.  There is no restriction
 * on the JVM respresentation, except that a value written using
 * CVMlong2Jvm() must return the same value when CVMjvm2Long() is called
 * (The operation is reversible.)
 */

/*
 * The following jvm2long, long2jvm, jvm2double, double2jvm 
 * and CVMmemCopy64 are used in conjunction with the 'raw'
 * field of struct JavaVal32.
 *
 * Because the raw field is of type CVMAddr, the type of location has
 * to be changed accordingly.
 */
CVMJavaLong CVMjvm2Long(const CVMAddr location[2]);

/*
 * Read a 64-bit Java long in native representation from "val"
 * and write the result in two-word JVM representation to "location"
 */

void CVMlong2Jvm(CVMAddr location[2], CVMJavaLong val);

/*
 * Convert 64-bit Java doubles between native representation and JVM
 * two-word representation:
 */

/*
 * Read a 64-bit Java double in two-word representation from "location"
 * and return the result in native representation.  There is no restriction
 * on the JVM respresentation, except that a value written using
 * CVMdouble2Jvm() must return the same value when CVMjvm2Dobule()
 * is called (the operation is reversible.)
 */

CVMJavaDouble CVMjvm2Double(const CVMAddr location[2]);

/*
 * Read a 64-bit Java double in native representation from "val"
 * and write the result in two-word JVM representation to "location"
 */

void CVMdouble2Jvm(CVMAddr location[2], CVMJavaDouble val);

/*
 * Copy two typeless 32-bit words from one location to another.
 * This is semantically equivalent to:
 * 
 * to[0] = from[0];
 * to[1] = from[1];
 *
 * but this interface is provided for those platforms that could
 * optimize this into a single 64-bit transfer.
 */

void CVMmemCopy64(CVMUint32 *to, const CVMUint32 *from);

/*
 * Java long manipulation.
 *
 * The result argument is, once again, an lvalue.
 *
 * Conversions:
 */

/*
 * Convert long bits to double bits, according to Double.longBitsToDouble()
 */

extern CVMJavaDouble CVMlongBits2Double(CVMJavaLong val);

/*
 * Convert double bits to long bits, according to Double.doubleToRawLongBits()
 */

extern CVMJavaLong CVMdouble2LongBits(CVMJavaDouble val);

/*
 * Convert long to int, according to "l2i" bytecode semantics
 */
extern CVMJavaInt CVMlong2Int(CVMJavaLong val);

/*
 * Convert long to float, according to "l2f" bytecode semantics
 */
extern CVMJavaFloat CVMlong2Float(CVMJavaLong val);

/*
 * Convert long to double, according to "l2d" bytecode semantics
 */
extern CVMJavaDouble CVMlong2Double(CVMJavaLong val);

/*
 * Convert long to void *
 */
extern void * CVMlong2VoidPtr(CVMJavaLong val);

/*
 * Convert void * to long
 */
extern CVMJavaLong CVMvoidPtr2Long(void * val);

/*
 * Convert int to long, according to "i2l" bytecode semantics
 */
extern CVMJavaLong CVMint2Long(CVMJavaInt val);

/*
 * Convert double to long, according to "d2l" bytecode semantics
 */
extern CVMJavaLong CVMdouble2Long(CVMJavaDouble val);

/*
 * Arithmetic:
 *
 * The functions below follow the semantics of the
 * ladd, land, ldiv, lmul, lor, lxor, and lrem bytecodes,
 * respectively.
 */

CVMJavaLong CVMlongAdd(CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongAnd(CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongDiv(CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongMul(CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongOr (CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongSub(CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongXor(CVMJavaLong op1, CVMJavaLong op2);
CVMJavaLong CVMlongRem(CVMJavaLong op1, CVMJavaLong op2);

/*
 * Shift:
 *
 * The functions below follow the semantics of the
 * lushr, lshl, and lshr bytecodes, respectively.
 */

CVMJavaLong CVMlongUshr(CVMJavaLong op1, CVMJavaInt op2);
CVMJavaLong CVMlongShl (CVMJavaLong op1, CVMJavaInt op2);
CVMJavaLong CVMlongShr (CVMJavaLong op1, CVMJavaInt op2);

/*
 * Unary:
 *
 * Return the negation of "op" (-op), according to
 * the semantics of the lneg bytecode.
 */

CVMJavaLong CVMlongNeg(CVMJavaLong op);

/*
 * Return the complement of "op" (~op)
 */

CVMJavaLong CVMlongNot(CVMJavaLong op);

/*
 * Comparisons (returning an CVMBool: CVM_TRUE or CVM_FALSE)
 *
 * To 0:
 */

CVMInt32 CVMlongLtz(CVMJavaLong op);     /* op <= 0 */
CVMInt32 CVMlongGez(CVMJavaLong op);     /* op >= 0 */
CVMInt32 CVMlongEqz(CVMJavaLong op);     /* op == 0 */

/*
 * Between operands:
 */

CVMInt32 CVMlongEq(CVMJavaLong op1, CVMJavaLong op2);    /* op1 == op2 */
CVMInt32 CVMlongNe(CVMJavaLong op1, CVMJavaLong op2);    /* op1 != op2 */
CVMInt32 CVMlongGe(CVMJavaLong op1, CVMJavaLong op2);    /* op1 >= op2 */
CVMInt32 CVMlongLe(CVMJavaLong op1, CVMJavaLong op2);    /* op1 <= op2 */
CVMInt32 CVMlongLt(CVMJavaLong op1, CVMJavaLong op2);    /* op1 <  op2 */
CVMInt32 CVMlongGt(CVMJavaLong op1, CVMJavaLong op2);    /* op1 >  op2 */

/*
 * Comparisons (returning an CVMJavaInt value: 0, 1, or -1)
 *
 * Between operands:
 *
 * Compare "op1" and "op2" according to the semantics of the
 * "lcmp" bytecode.
 */

CVMInt32 CVMlongCompare(CVMJavaLong op1, CVMJavaLong op2);

/*
 * Constants (each a CVMJavaLong that can be passed to any one of the
 * operations above)
 *
 * The functions below follow the semantics of the
 * lconst_0 and lconst_1 bytecodes, respectively.
 */

CVMJavaLong CVMlongConstZero();
CVMJavaLong CVMlongConstOne();

/*
 ******************************************
 * Java double floating-point manipulation.
 ******************************************
 *
 * The result argument is, once again, an lvalue.
 *
 * Conversions:
 */

/*
 * Convert double to int, according to "d2i" bytecode semantics
 */

CVMJavaInt CVMdouble2Int(CVMJavaDouble val);

/*
 * Convert double to float, according to "d2f" bytecode semantics
 */

CVMJavaFloat CVMdouble2Float(CVMJavaDouble val);

/*
 * Convert int to double, according to "i2d" bytecode semantics
 */

CVMJavaDouble CVMint2Double(CVMJavaInt val);

/*
 * Arithmetic:
 *
 * The functions below follow the semantics of the
 * dadd, dsub, ddiv, dmul, and drem bytecodes, respectively.
 */

CVMJavaDouble CVMdoubleAdd(CVMJavaDouble op1, CVMJavaDouble op2);
CVMJavaDouble CVMdoubleSub(CVMJavaDouble op1, CVMJavaDouble op2);
CVMJavaDouble CVMdoubleDiv(CVMJavaDouble op1, CVMJavaDouble op2);
CVMJavaDouble CVMdoubleMul(CVMJavaDouble op1, CVMJavaDouble op2);

/*
 * #define USE_NATIVE_FREM if the platform provides a non-ANSI frem
 * #define USE_ANSI_FMOD if the platform has an ANSI fmod for
 * implementing CVMdoubleRem()
 *
 * Otherwise a default implementation is provided 
 */
CVMJavaDouble CVMdoubleRem(CVMJavaDouble op1, CVMJavaDouble op2);

/*
 * Unary:
 *
 * Return the negation of "op" (-op), according to
 * the semantics of the dneg bytecode.
 */

CVMJavaDouble CVMdoubleNeg(CVMJavaDouble op);

/*
 * Comparisons (returning an CVMInt32 value: 0, 1, or -1)
 *
 * Between operands:
 *
 * Compare "op1" and "op2" according to the semantics of the
 * "dcmpl" (direction is -1) or "dcmpg" (direction is 1) bytecodes.
 *
 * #define USE_NATIVE_FCOMPARE if the platform provides a non-ANSI compare.
 * #define USE_ANSI_FCOMPARE if the platform has an ANSI compare for
 * implementing CVMdoubleCompare().
 *
 * Otherwise a default implementation is provided.
 */

CVMInt32 CVMdoubleCompare(CVMJavaDouble op1, CVMJavaDouble op2,
			  CVMInt32 direction);

/*
 * Constants (each a CVMJavaDouble that can be passed to any one of the
 * operations above)
 *
 * The functions below follow the semantics of the
 * dconst_0 and dconst_1 bytecodes, respectively.
 */

CVMJavaDouble CVMdoubleConstZero();
CVMJavaDouble CVMdoubleConstOne();

#include CVM_HDR_DOUBLEWORD_H

#endif /* _INCLUDED_PORTING_DOUBLEWORD_H */
