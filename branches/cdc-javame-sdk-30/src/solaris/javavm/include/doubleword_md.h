/*
 * @(#)doubleword_md.h	1.20 06/10/10
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

#ifndef _SOLARIS_DOUBLEWORD_MD_H
#define _SOLARIS_DOUBLEWORD_MD_H

#include "javavm/include/doubleword_arch.h"

#include "portlibs/ansi_c/doubleword.h"

#if defined(__GNUC__)

#include "portlibs/gcc_32_bit/doubleword.h"

#else

/*
 * See src/share/javavm/include/portlibs/doubleword.h for full documentation.
 */

/*
 * Two versions, one assuming a platform double-word instruction that
 * can allow unaligned access, and the other not allowing it.  
 */

#if defined(CAN_DO_UNALIGNED_INT64_ACCESS) || defined (_LP64)

#define CVMjvm2Long(l)		(*(CVMJavaLong *)(l))
#define CVMlong2Jvm(l, v)	(*(CVMJavaLong *)(l) = (v))

#else /* !CAN_DO_UNALIGNED_INT64_ACCESS */

static CVMJavaLong SOLARISjvm2Long(const CVMAddr location[2]);

#pragma inline (SOLARISjvm2Long);

static CVMJavaLong SOLARISjvm2Long(const CVMAddr location[2])
{
    union {
	CVMJavaLong l;
	CVMAddr v[2];
    } res;
    res.v[0] = (location)[0];
    res.v[1] = (location)[1];
    return res.l;
}

#define CVMjvm2Long SOLARISjvm2Long

#define CVMlong2Jvm(location, val)	\
{					\
    union {				\
	CVMJavaLong l;			\
	CVMAddr v[2];			\
    } tmp;				\
    tmp.l = (val);			\
    (location)[0] = tmp.v[0];		\
    (location)[1] = tmp.v[1];		\
}

#endif /* CAN_DO_UNALIGNED_INT64_ACCESS */

#if defined (CAN_DO_UNALIGNED_DOUBLE_ACCESS) || defined (_LP64)

#define CVMjvm2Double(location)		\
    (*(CVMJavaDouble *)(location))

#define CVMdouble2Jvm(location, val)		\
    (*(CVMJavaDouble *)(location) = (val))

#else /* !CAN_DO_UNALIGNED_DOUBLE_ACCESS */

static CVMJavaDouble SOLARISjvm2Double(const CVMAddr location[2]);

#pragma inline (SOLARISjvm2Double)

static CVMJavaDouble SOLARISjvm2Double(const CVMAddr location[2])
{
    union {
	CVMJavaDouble d;
	CVMAddr v[2];
    } res;
    res.v[0] = location[0];
    res.v[1] = location[1];
    return res.d;
}

#define CVMjvm2Double SOLARISjvm2Double

#define CVMdouble2Jvm(location, val)	\
{					\
    union {				\
	CVMJavaDouble d;		\
	CVMAddr v[2];			\
    } tmp;				\
    tmp.d = (val);			\
    (location)[0] = tmp.v[0];		\
    (location)[1] = tmp.v[1];		\
}

#endif /* CAN_DO_UNALIGNED_DOUBLE_ACCESS */

#if defined(CAN_DO_UNALIGNED_DOUBLE_ACCESS) && defined(COPY_64_AS_DOUBLE)

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
    (*(CVMJavaDouble *)(destination) = *(CVMJavaDouble *)(source))

#elif defined(CAN_DO_UNALIGNED_INT64_ACCESS) && defined(COPY_64_AS_INT64)

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
    (*(CVMJavaLong *)(destination) = *(CVMJavaLong *)(source))

#elif defined (_LP64)

#define CVMmemCopy64(destination, source) \
    (*(CVMAddr *)(destination) = *(CVMAddr *)(source))

#else

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
    ((destination)[0] = (source)[0], (destination)[1] = (source)[1])

#endif

/*
 * Long Conversions:
 */

#define CVMint2Long(val) \
    ((CVMJavaLong)(val))

#define CVMlong2Int(val) \
    ((CVMJavaInt)(val))

#define CVMlong2Float(val) \
    ((CVMJavaFloat)(val))

#define CVMlong2Double(val) \
    ((CVMJavaDouble)(val))

#define CVMlong2VoidPtr(val) \
    ((void *)(CVMAddr)(val))

#define CVMvoidPtr2Long(val) \
    ((CVMJavaLong)(CVMAddr)(val))

#ifdef HAVE_DOUBLE_BITS_CONVERSION
# ifdef NORMAL_DOUBLE_BITS_CONVERSION
#  define CVMlongBits2Double(val) \
      (*((CVMJavaDouble *) &(val)))
#  define CVMdouble2LongBits(val) \
      (*((CVMJavaLong *) &(val)))
# endif
#else
# error Must port CVMlongBitsToDouble and CVMdoubleToLongBits to your platform
#endif

/*
 * Long Arithmetic:
 */
#define CVMlongBinaryOp(op1, op2, operator) \
    ((op1) operator (op2))

#define CVMlongAdd(op1, op2) \
    CVMlongBinaryOp(op1, op2, +)

#define CVMlongAnd(op1, op2) \
    CVMlongBinaryOp(op1, op2, &)

#define CVMlongDiv(op1, op2) \
    CVMlongBinaryOp(op1, op2, /)

#define CVMlongMul(op1, op2) \
    CVMlongBinaryOp(op1, op2, *)

#define CVMlongOr(op1, op2) \
    CVMlongBinaryOp(op1, op2, |)

#define CVMlongSub(op1, op2) \
    CVMlongBinaryOp(op1, op2, -)

#define CVMlongXor(op1, op2) \
    CVMlongBinaryOp(op1, op2, ^)

#define CVMlongRem(op1, op2) \
    CVMlongBinaryOp(op1, op2, %)

/*
 * The shift operations are somewhat unlike the above. Special-case them
 */
#define CVMlongUshr(op1, n) \
    (((unsigned long long)(op1)) >> ((n) & 0x3F))

#define CVMlongShl(op1, n) \
    ((op1) << ((n) & 0x3F))

#define CVMlongShr(op1, n) \
    ((op1) >> ((n) & 0x3F))

/*
 * Long Unary operations
 */
#define CVMlongUnaryOp(op, operator) \
    (operator ((op)))

#define CVMlongNeg(op) \
    CVMlongUnaryOp(op, -)

#define CVMlongNot(op) \
    CVMlongUnaryOp(op, ~)

/*
 * Long Comparisons (returning an CVMBool: CVM_TRUE or CVM_FALSE)
 */

#define CVMlongComparisonOp(op1, op2, boolop) \
    ((op1) boolop ((op2)))

/* To 0: */

#define CVMlongComparisonOpToZero(op, boolop) \
    ((op) boolop CVMlongConstZero())

#define CVMlongLtz(op) \
    CVMlongComparisonOpToZero((op), <)

#define CVMlongGez(op) \
    CVMlongComparisonOpToZero((op), >=)

#define CVMlongEqz(op) \
    CVMlongComparisonOpToZero((op), ==)

/*
 * Long OperationBetween operands:
 */
#define CVMlongEq(op1, op2) \
    CVMlongComparisonOp((op1), (op2), ==)

#define CVMlongNe(op1, op2) \
    CVMlongComparisonOp((op1), (op2), !=)

#define CVMlongGe(op1, op2) \
    CVMlongComparisonOp((op1), (op2), >=)

#define CVMlongLe(op1, op2) \
    CVMlongComparisonOp((op1), (op2), <=)

#define CVMlongLt(op1, op2) \
    CVMlongComparisonOp((op1), (op2), < )

#define CVMlongGt(op1, op2) \
    CVMlongComparisonOp((op1), (op2), > )

/*
 * Long Compare operation 
 */
#define CVMlongCompare(op1, op2) \
    (CVMlongLt((op1), (op2)) ? -1 : \
     CVMlongGt((op1), (op2)) ? 1 : \
     0)

/*
 * Long Constants (CVMJavaLong's each)
 */
#define CVMlongConstZero()    (0LL)
#define CVMlongConstOne()     (1LL)

/*
 * Double Conversions:
 */

#define CVMdouble2Long(val) \
    double2Long((val))

/*
 * JAVA_COMPLIANT_d2l, NAN_CHECK_d2l, or neither (but not both) may be
 * defined.
 *
 * If NAN_CHECK_d2l is defined, then BOUNDS_CHECK_d2l may be defined.
 */

#ifdef JAVA_COMPLIANT_d2l

#define double2Long(val)   (CVMJavaLong)((val))

#elif (defined(NAN_CHECK_d2l))

#define double2Long(val)   ((val) != (val) ? 0LL : double2Long0(val))

#else

extern CVMJavaLong double2Long(CVMJavaDouble d);

#endif

#ifdef BOUNDS_CHECK_d2l

#define double2Long0(val) \
    (  ((val) <= -9223372036854775808.0) ? longMin : \
                   ((val) >= 9223372036854775807.0) ? longMax : \
                   (CVMJavaLong)(val) )

#else

#define double2Long0(val) ((CVMJavaLong)(val))

#endif

#endif /* !GCC */

#endif /* _SOLARIS_DOUBLEWORD_MD_H */
