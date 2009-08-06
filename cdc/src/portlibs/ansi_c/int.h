/*
 * @(#)int.h	1.23 06/10/10
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

#ifndef _ANSI_BIT_INT_H
#define _ANSI_BIT_INT_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/endianness.h"

/*
 * See src/share/javavm/include/porting/int.h for full documentation.
 *
 * Arithmetic:
 *
 * Some platforms don't treat 0x80000000 / -1 the same as Java does,
 * so it's necessary to special case the '/' and '%' operators. Also
 * some compilers aren't smart enough to notice that (#operator == '/')
 * is always true. For example, if CVMintBinaryOp was implemented as:
 *
 *    #define CVMintBinaryOp(op1, op2, operator)  			\
 *                (((#operator[0]=='/') || (#operator[0]=='%')) ?	\
 *                     ((((op1) == 0x80000000) && ((op2) == -1)) ?     	\
 *                            ((op1) op 1) :				\
 *                            ((op1) op (op2))) :			\
 *                      ((op1) op (op2)))
 *
 * Some compilers would force the operator argument to be compared to '/'
 * and '%' even if '+' was passed in. A smart compiler would simply optimized
 * away everything but ((op1) op (op2))) if '+' was passed in.
 */

#define intBinaryOp(op1, op2, operator) \
    ((op1) operator (op2))

#define intBinaryOp2(op1, op2, operator)	\
    (((op1) == 0x80000000 && (op2) == -1) ?	\
	((op1) operator 1) :		\
	((op1) operator (op2)))

#define CVMintAdd(op1, op2) \
    intBinaryOp(op1, op2, +)

#define CVMintAnd(op1, op2) \
    intBinaryOp(op1, op2, &)

#define CVMintMul(op1, op2) \
    intBinaryOp(op1, op2, *)

#define CVMintOr(op1, op2) \
    intBinaryOp(op1, op2, |)

#define CVMintSub(op1, op2) \
    intBinaryOp(op1, op2, -)

#define CVMintXor(op1, op2) \
    intBinaryOp(op1, op2, ^)

/*
 * Shift operations 
 */

#define CVMintUshr(op1, n) \
    (((unsigned int)(op1)) >> (n & 0x1F))

#define CVMintShl(op1, n) \
    ((op1) << (n & 0x1F))

#define CVMintShr(op1, n) \
    ((op1) >> (n & 0x1F))

/*
 * Unary operations
 */
#define CVMintUnaryOp(op, operator) \
    (operator ((op)))

#define CVMintNeg(op) \
    CVMintUnaryOp(op, -)


#ifdef JAVA_COMPLIANT_DIV_REM

#define CVMintDiv(op1, op2) \
    intBinaryOp(op1, op2, /)

#define CVMintRem(op1, op2) \
    intBinaryOp(op1, op2, %)

#else

#define CVMintDiv(op1, op2) \
    intBinaryOp2(op1, op2, /)

#define CVMintRem(op1, op2) \
    intBinaryOp2(op1, op2, %)

#endif

/*
 * Int Conversions:
 */

#define zeroExtendJavaChar (0xffff)

#define CVMint2Float(val) \
    ((CVMJavaFloat)((val)))

#define CVMint2Byte(val) \
    ((signed char)(val))

#define CVMint2Char(val) \
    ((val) & zeroExtendJavaChar)

#define CVMint2Short(val) \
    ((signed short)(val))

/*
 * Macros for accessing byte aligned big-endian integers.
 *
 * A big-endian platform with relaxed alignment requirements
 * might choose to perform a single 16- or 32-bit load.  A
 * little-endian platform like x86 might choose to follow a
 * single load by a byte-swap instruction.
 */

#define getUint8(ptr_, offset_) (*(CVMUint8*)((ptr_)+(offset_)))

#define CVMgetUint16(ptr_)		 \
    ((getUint8(ptr_, 0) << 8) |		\
    getUint8(ptr_, 1))

#define CVMgetUint32(ptr_) 		\
    ((getUint8(ptr_, 0) << 24) |	\
    (getUint8(ptr_, 1) << 16) |		\
    (getUint8(ptr_, 2) <<  8) |		\
    getUint8(ptr_, 3))

#define CVMgetInt16(ptr_) ((CVMInt16) CVMgetUint16(ptr_))
#define CVMgetInt32(ptr_) ((CVMInt32) CVMgetUint32(ptr_))

/*
 * Macro for accessing word aligned big-endian integers.
 *
 * A little-endian platform like x86 would need to make sure the
 * bytes get reversed.
 */
#if CVM_ENDIANNESS == CVM_BIG_ENDIAN
#define CVMgetAlignedInt32(ptr_) (*((CVMInt32*)(ptr_)))
#define CVMputAlignedInt32(ptr_,val_) (*((CVMInt32*)(ptr_))=(val_))
#else
/* %comment d001 */
#define CVMgetAlignedInt32(ptr_) (CVMInt32)((*((CVMInt32*)(ptr_)) << 24) \
				 | ((*((CVMInt32*)(ptr_)) & 0xff00) << 8) \
				 | ((*((CVMInt32*)(ptr_)) >> 8) & 0xff00) \
				 | (*((CVMUint32*)(ptr_)) >> 24))

#define CVMputAlignedInt32(ptr_,val_)  (*((CVMInt32*)(ptr_))= \
			(((val_)>>24) &0x000000ff) \
			| (((val_)>>8)&0x0000ff00) \
			| (((val_)<<8)&0x00ff0000) \
			| (((val_)<<24) ) )
	
#endif

#endif /* _ANSI_BIT_INT_H */
