/*
 * @(#)int.h	1.18 06/10/10
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

#ifndef _INCLUDED_PORTING_INT_H
#define _INCLUDED_PORTING_INT_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * Java arithmetic macros. The res argument is an lvalue.
 * This interface does not exist to allow arbitrary representations
 * of CVMInt32.  Its only purpose is to guarantee Java semantics
 * for operations on the Java "int" type.
 *
 * The functions below follow the semantics of the
 * iadd, isub, imul, idiv, irem, iand, ior, ixor,
 * and ineg bytecodes, respectively.
 */

CVMJavaInt CVMintAdd(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintSub(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintMul(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintDiv(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintRem(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintAnd(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintOr (CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintXor(CVMJavaInt op1, CVMJavaInt op2);
CVMJavaInt CVMintNeg(CVMJavaInt op);

/*
 * Shift Operation:
 *     The result argument is, once again, an lvalue.
 *
 * The functions below follow the semantics of the
 * iushr, ishl, and ishr bytecodes, respectively.
 */

CVMJavaInt CVMintUshr(CVMJavaInt op, CVMJavaInt num);
CVMJavaInt CVMintShl (CVMJavaInt op, CVMJavaInt num);
CVMJavaInt CVMintShr (CVMJavaInt op, CVMJavaInt num);

/*
 * Unary Operation:
 *
 * Return the negation of "op" (-op), according to
 * the semantics of the ineg bytecode.
 */

CVMJavaInt CVMintNeg(CVMJavaInt op);

/*
 * Int Conversions:
 */

/*
 * Convert int to float, according to "i2f" bytecode semantics
 */

CVMJavaFloat CVMint2Float(CVMJavaInt val);

/*
 * Convert int to byte, according to "i2b" bytecode semantics
 */

CVMJavaByte CVMint2Byte(CVMJavaInt val);

/*
 * Convert int to char, according to "i2c" bytecode semantics
 */

CVMJavaChar CVMint2Char(CVMJavaInt val);

/*
 * Convert int to short, according to "i2s" bytecode semantics
 */

CVMJavaShort CVMint2Short(CVMJavaInt val);

/*
 * Functions for accessing byte aligned big-endian integers.
 *
 * A big-endian platform with relaxed alignment requirements
 * might choose to perform a single 16- or 32-bit load.  A
 * little-endian platform like x86 might choose to follow a
 * single load by a byte-swap instruction.
 */

CVMUint16 CVMgetUint16(const CVMUint8 *ptr);
CVMUint32 CVMgetUint32(const CVMUint8 *ptr);
CVMInt16  CVMgetInt16(const CVMUint8 *ptr);
CVMInt32  CVMgetInt32(const CVMUint8 *ptr);

/*
 * Macro for accessing word aligned big-endian integers.
 *
 * A little-endian platform like x86 would need to make sure the
 * bytes get reversed.
 *
 * CVMInt32 CVMgetAlignedInt32(ptr_) (const CVMUint8 *ptr)
 */

#include CVM_HDR_INT_H

#endif /* _INCLUDED_PORTING_INT_H */
