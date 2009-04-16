/*
 * @(#)doubleword.h	1.19 06/10/10
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

#ifndef _ANSI_DOUBLEWORD_H
#define _ANSI_DOUBLEWORD_H

#ifdef USE_ANSI_FMOD
#include <math.h>		 /* fmod */
#endif

/*
 * See src/share/javavm/include/porting/doubleword.h for full documentation.
 */

/*
 * Double Conversions:
 */

#ifndef CVMdouble2Float
#define CVMdouble2Float(val) \
    ((CVMJavaFloat)(val))
#endif

#ifndef CVMdouble2Int
#define CVMdouble2Int(val) \
    (double2Int(val))
#endif

#ifndef CVMint2Double
#define CVMint2Double(val) \
    ((CVMJavaDouble)(val))
#endif

/*
 * JAVA_COMPLIANT_d2i, NAN_CHECK_d2i, or neither (but not both) may be
 * defined.
 */

#ifdef JAVA_COMPLIANT_d2i

#define double2Int(val)    (CVMJavaInt)((val))

#elif (defined(NAN_CHECK_d2i))

#define double2Int(val)    ((val) != (val) ? 0 : (CVMJavaInt)(val))

#else

extern CVMJavaInt double2Int(CVMJavaDouble d);

#endif

/*
 * Double Arithmetic:
 */

#ifndef CVMdoubleAdd
#define CVMdoubleAdd(op1, op2)	((op1) + (op2))
#endif
#ifndef CVMdoubleSub
#define CVMdoubleSub(op1, op2)	((op1) - (op2))
#endif
#ifndef CVMdoubleMul
#define CVMdoubleMul(op1, op2)	((op1) * (op2))
#endif
/* Divide-by-zero exceptions should be masked */
#ifndef CVMdoubleDiv
#define CVMdoubleDiv(op1, op2)	((op1) / (op2))
#endif
#ifndef CVMdoubleRem
#ifdef USE_ANSI_FMOD
#define CVMdoubleRem(op1, op2)	(fmod((op1), (op2)))
#endif
#endif

/*
 * Double Unary operations
 */

#ifndef CVMdoubleNeg
#define CVMdoubleNeg(op)	(-(op))
#endif

/*
 * Double Comparisons (returning int value 0, 1, or -1) 
 */

/*
 * Double Compare operation
 */
#ifdef USE_ANSI_FCOMPARE
#define CVMdoubleCompare(op1, op2, opcode_value) \
    (((op1) < (op2)) ? -1 : \
     ((op1) > (op2)) ? 1 : \
     ((op1) == (op2)) ? 0 : \
     ((opcode_value) == -1 || (opcode_value) == 1) ? (opcode_value) :\
     0)
#endif

/*
 * Double Constants (CVMJavaDouble's each)
 */
#define CVMdoubleConstZero()    (0.0)
#define CVMdoubleConstOne()     (1.0)

#endif /* _ANSI_DOUBLEWORD_H */
