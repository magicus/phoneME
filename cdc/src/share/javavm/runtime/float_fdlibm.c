/*
 * @(#)float_fdlibm.c	1.7 06/10/10
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

/*
 * Default floating point routines required for implementing frem and drem.
 */

#include "javavm/include/defs.h"
#include "javavm/include/assert.h"
#include "fdlibm.h"

#if !defined(USE_ANSI_FMOD) && !defined(USE_NATIVE_FREM)

CVMJavaFloat 
CVMfloatRem(CVMJavaFloat op1, CVMJavaFloat op2)
{
    return (CVMJavaFloat)CVMfdlibmFmod((CVMJavaDouble)op1, (CVMJavaDouble)op2);
}

CVMJavaDouble 
CVMdoubleRem(CVMJavaDouble op1, CVMJavaDouble op2)
{
    return CVMfdlibmFmod(op1, op2);
}

#endif

#if !defined(USE_ANSI_FCOMPARE) && !defined(USE_NATIVE_FCOMPARE)

/* Purpose: Compared "op1" and "op2" according to the semantics of the "fcmpl"
            (direction is -1) or "fcmpg" (direction is 1) bytecodes. */
/* Returns: A CVMInt32 value: 0, 1, or -1). */
CVMInt32 CVMfloatCompare(CVMJavaFloat op1, CVMJavaFloat op2,
                         CVMInt32 direction)
{
    CVMassert((direction == -1) || (direction == 1));
    return ((CVMfdlibmIsnan((CVMJavaDouble)op1) ||
             CVMfdlibmIsnan((CVMJavaDouble)op2)) ?
            direction :
            (op1 < op2) ? -1 :
            (op1 > op2) ? 1 :
            (op1 == op2) ? 0 :
            0);
}

/* Purpose: Compared "op1" and "op2" according to the semantics of the "dcmpl"
            (direction is -1) or "dcmpg" (direction is 1) bytecodes. */
/* Returns: A CVMInt32 value: 0, 1, or -1). */
CVMInt32 CVMdoubleCompare(CVMJavaDouble op1, CVMJavaDouble op2,
                          CVMInt32 direction)
{
    CVMassert((direction == -1) || (direction == 1));
    return ((CVMfdlibmIsnan(op1) || CVMfdlibmIsnan(op2)) ? direction :
            (op1 < op2) ? -1 :
            (op1 > op2) ? 1 :
            (op1 == op2) ? 0 :
            0);
}

#endif

