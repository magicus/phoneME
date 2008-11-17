/*
 * @(#)doubleword.h	1.3 06/10/10
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

#ifndef _MSVC_DOUBLEWORD_H
#define _MSVC_DOUBLEWORD_H

#include "javavm/include/doubleword_arch.h"

#define CVMdoubleRem(op1, op2)   doubleRem((op1), (op2))

#include "portlibs/ansi_c/doubleword.h"

/*
 * Double Arithmetic:
 */

extern double doubleRem(double, double);

#ifdef USE_NATIVE_FCOMPARE
/*
 * Double Comparisons (returning int value 0, 1, or -1) 
 */

/*
 * Double Compare operation
 */
#define CVMdoubleCompare(op1, op2, opcode_value) \
    ((CVMisNan(op1) || CVMisNan(op2)) ? (opcode_value) : \
     ((op1) < (op2)) ? -1 : \
     ((op1) > (op2)) ? 1 : \
     ((op1) == (op2)) ? 0 : \
     ((opcode_value) == -1 || (opcode_value) == 1) ? (opcode_value) :\
     0)
#endif

/*
 * Two versions, one assuming a platform double-word instruction that
 * can allow unaligned access, and the other not allowing it.  
 */

#ifdef CAN_DO_UNALIGNED_DOUBLE_ACCESS

#define CVMjvm2Double(location)		(*(CVMJavaDouble *)(location))
#define CVMdouble2Jvm(location, val)	(*(CVMJavaDouble *)(location) = (val))

#else

__inline CVMJavaDouble CVMjvm2Double(const CVMUint32 location[2])
{
    union {
	CVMJavaDouble d;
	CVMUint32 v[2];
    } res;
    res.v[0] = (location)[0];
    res.v[1] = (location)[1];
    return res.d;
}

#define CVMdouble2Jvm(location, val)   	\
{					\
    union {				\
	CVMJavaDouble d;		\
	CVMUint32 v[2];			\
    } tmp;				\
    tmp.d = (val);			\
    (location)[0] = tmp.v[0];		\
    (location)[1] = tmp.v[1];		\
}

#endif /* !CAN_DO_UNALIGNED_DOUBLE_ACCESS */


/*
 * Two versions, one assuming a platform double-word instruction that
 * can allow unaligned access, and the other not allowing it.  
 */

#ifdef CAN_DO_UNALIGNED_INT64_ACCESS

#define CVMjvm2Long(l)			(*(CVMJavaLong *)(l))
#define CVMlong2Jvm(l, v)		(*(CVMJavaLong *)(l) = (v))

#else

__inline CVMJavaLong CVMjvm2Long(const CVMUint32 location[2])
{
    union {
	CVMJavaLong l;
	CVMUint32 v[2];
    } res;
    res.v[0] = (location)[0];
    res.v[1] = (location)[1];
    return res.l;
}

#define CVMlong2Jvm(location, val)	\
{					\
    union {				\
	CVMJavaLong l;			\
	CVMUint32 v[2];			\
    } tmp;				\
    tmp.l = (val);			\
    (location)[0] = tmp.v[0];		\
    (location)[1] = tmp.v[1];		\
}

#endif /* !CAN_DO_UNALIGNED_INT64_ACCESS */


#if defined(CAN_DO_UNALIGNED_DOUBLE_ACCESS) && defined(COPY_64_AS_DOUBLE)

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
    (*(CVMJavaDouble *)(destination) = *(CVMJavaDouble *)(source))

#elif defined(CAN_DO_UNALIGNED_INT64_ACCESS) && defined(COPY_64_AS_INT64)

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
    (*(CVMJavaLong *)(destination) = *(CVMJavaLong *)(source))

#else

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
    (((CVMUint32*)(destination))[0] = ((CVMUint32*)(source))[0], \
     ((CVMUint32*)(destination))[1] = ((CVMUint32*)(source))[1])

#endif

/*
 * Long Conversions:
 */

#define CVMint2Long(val)	((CVMJavaLong)(val))

#define CVMlong2Int(val)	((CVMJavaInt)(val))

#define CVMlong2Float(val)	((CVMJavaFloat)(val))

#define CVMlong2Double(val)	((CVMJavaDouble)(val))

#define CVMlong2VoidPtr(val)	((void *)(int)(val))

#define CVMvoidPtr2Long(val)	((CVMJavaLong)(int)(val))


#define CVMlongAdd(op1, op2)	((op1) + (op2))
#define CVMlongSub(op1, op2)	((op1) - (op2))
#define CVMlongMul(op1, op2)	((op1) * (op2))
#ifndef CVMlongDiv
#define CVMlongDiv(op1, op2)	((op1) / (op2))
#endif
#ifndef CVMlongRem
#define CVMlongRem(op1, op2)	((op1) % (op2))
#endif
#define CVMlongAnd(op1, op2)	((op1) & (op2))
#define CVMlongOr(op1, op2)	((op1) | (op2))
#define CVMlongXor(op1, op2)	((op1) ^ (op2))

/*
 * The shift operations are somewhat unlike the above. Special-case them
 */
#define CVMlongUshr(op1, n) \
    (((unsigned __int64)(op1)) >> ((n) & 0x3F))

#define CVMlongShl(op1, n) \
    ((op1) << ((n) & 0x3F))

#define CVMlongShr(op1, n) \
    ((op1) >> ((n) & 0x3F))

/*
 * Long Unary operations
 */

#define CVMlongNeg(op)	(-(op))
#define CVMlongNot(op)	(~(op))

/*
 * Long Comparisons (returning an CVMBool: CVM_TRUE or CVM_FALSE)
 */

/* To 0: */

#define CVMlongLtz(op)	((op) < (__int64)0)
#define CVMlongGez(op)	((op) >= (__int64)0)
#define CVMlongEqz(op)	((op) == (__int64)0)

/*
 * Long OperationBetween operands:
 */

#define CVMlongEq(op1, op2)	((op1) == (op2))
#define CVMlongNe(op1, op2)	((op1) != (op2))
#define CVMlongGe(op1, op2)	((op1) >= (op2))
#define CVMlongLe(op1, op2)	((op1) <= (op2))
#define CVMlongLt(op1, op2)	((op1) < (op2))
#define CVMlongGt(op1, op2)	((op1) > (op2))

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
#define CVMlongConstZero()    (__int64)(0)
#define CVMlongConstOne()     (__int64)(1)

/*
 * Double Conversions:
 */

/* #define CVMdouble2Long(val) (CVMJavaLong)((val)) */
extern CVMJavaLong double2Long(CVMJavaDouble);
#define CVMdouble2Long(val)      double2Long((val))

#endif /* _MSVC_DOUBLEWORD_H */
