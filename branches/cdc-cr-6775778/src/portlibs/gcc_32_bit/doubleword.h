/*
 * @(#)doubleword.h	1.38 06/10/10
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

#ifndef _GCC_32_BIT_DOUBLEWORD_H
#define _GCC_32_BIT_DOUBLEWORD_H

/*
 * See src/share/javavm/include/porting/doubleword.h for full documentation.
 */

/*
 * Two versions, one assuming a platform double-word instruction that
 * can allow unaligned access, and the other not allowing it.  
 */

#ifdef CAN_DO_UNALIGNED_INT64_ACCESS

#define CVMjvm2Long(location)				\
({							\
    typedef union {					\
	CVMJavaLong l;					\
    } fakeOutGCCStrictAliasing;				\
    (((fakeOutGCCStrictAliasing *)(location))->l);	\
})

#define CVMlong2Jvm(location, val)			\
{							\
    typedef union {					\
	CVMJavaLong l;					\
    } fakeOutGCCStrictAliasing;				\
    ((fakeOutGCCStrictAliasing *)(location))->l = val;  \
}

#else /* !CAN_DO_UNALIGNED_INT64_ACCESS */

/* 
 * The following jvm2long, long2jvm, jvm2double and double2jvm are
 * used in conjunction with the 'raw' field of struct JavaVal32.
 * Because the raw field is of type CVMAddr, the type of location has
 * to be changed accordingly.
 *
 * CVMAddr is 8 byte on 64 bit platforms and 4 byte on 32 bit
 * platforms.
 * 
 * That meens that in case of an 32 bit platform, the JavaLong 
 * value is build from 2 32 bit values.
 * In case of the 64 bit platform, The JavaLong value is a 
 * copy of (location)[0].  
 * (location)[1] is not needed.
 * This implies that a JavaLong fits in the first JavaVal32
 * slot. The second JavaVal32 is not required.  
 */
#define CVMjvm2Long(location)	\
({				\
    union {			\
	CVMJavaLong l;		\
	CVMAddr v[2];		\
    } res;			\
    res.v[0] = (location)[0];	\
    res.v[1] = (location)[1];	\
    res.l;			\
})

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

#ifdef CAN_DO_UNALIGNED_DOUBLE_ACCESS

#define CVMjvm2Double(location)				\
({							\
    typedef union {					\
	CVMJavaDouble d;				\
    } fakeOutGCCStrictAliasing;				\
    (((fakeOutGCCStrictAliasing *)(location))->d);	\
})

#define CVMdouble2Jvm(location, val)			\
{							\
    typedef union {					\
	CVMJavaDouble d;				\
    } fakeOutGCCStrictAliasing;				\
    ((fakeOutGCCStrictAliasing *)(location))->d = val;	\
}

#else /* !CAN_DO_UNALIGNED_DOUBLE_ACCESS */

#define CVMjvm2Double(location)		\
({					\
    union {				\
	CVMJavaDouble d;		\
	CVMAddr v[2];			\
    } res;				\
    res.v[0] = (location)[0];		\
    res.v[1] = (location)[1];		\
    res.d;				\
})

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
{								\
    typedef union {						\
	CVMJavaDouble d;					\
    } fakeOutGCCStrictAliasing;					\
    ((fakeOutGCCStrictAliasing *)(destination))->d = 		\
	((fakeOutGCCStrictAliasing *)(source))->d;		\
}

#elif defined(CAN_DO_UNALIGNED_INT64_ACCESS) && defined(COPY_64_AS_INT64)

/* CVMmemCopy64(CVMUint32* destination, CVMUint32* source) */
#define CVMmemCopy64(destination, source) \
{								\
    typedef union {						\
	CVMJavaLong l;						\
    } fakeOutGCCStrictAliasing;					\
    ((fakeOutGCCStrictAliasing *)(destination))->l = 		\
	((fakeOutGCCStrictAliasing *)(source))->l;		\
}

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

/* 
 * CVMlong2VoidPtr(val), CVMvoidPtr2Long(val)
 * casting val to an address requires CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMlong2VoidPtr(val)	((void *)(CVMAddr)(val))

#define CVMvoidPtr2Long(val)	((CVMJavaLong)(CVMAddr)(val))

#ifdef HAVE_DOUBLE_BITS_CONVERSION
# ifdef NORMAL_DOUBLE_BITS_CONVERSION
#  define CVMlongBits2Double(val)	(*((CVMJavaDouble *) &(val)))
#  define CVMdouble2LongBits(val)	(*((CVMJavaLong *) &(val)))
# endif
#else
# error Must port CVMlongBitsToDouble and CVMdoubleToLongBits to your platform
#endif

/*
 * Long Arithmetic:
 */

#define CVMlongAdd(op1, op2)	((op1) + (op2))
#define CVMlongSub(op1, op2)	((op1) - (op2))
#define CVMlongMul(op1, op2)	((op1) * (op2))
#define CVMlongDiv(op1, op2)	((op1) / (op2))
#define CVMlongRem(op1, op2)	((op1) % (op2))
#define CVMlongAnd(op1, op2)	((op1) & (op2))
#define CVMlongOr(op1, op2)	((op1) | (op2))
#define CVMlongXor(op1, op2)	((op1) ^ (op2))

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

#define CVMlongNeg(op)	(-(op))
#define CVMlongNot(op)	(~(op))

/*
 * Long Comparisons (returning an CVMBool: CVM_TRUE or CVM_FALSE)
 */

/* To 0: */

#define CVMlongLtz(op)	((op) < 0LL)
#define CVMlongGez(op)	((op) >= 0LL)
#define CVMlongEqz(op)	((op) == 0LL)

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

#endif /* _GCC_32_BIT_DOUBLEWORD_H */
