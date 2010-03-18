/*
 * @(#)doubleword_arch.h	1.7 06/10/10
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

#ifndef _WIN32_DOUBLEWORD_ARCH_H
#define _WIN32_DOUBLEWORD_ARCH_H

#undef CAN_DO_UNALIGNED_DOUBLE_ACCESS
#undef CAN_DO_UNALIGNED_INT64_ACCESS
#undef  JAVA_COMPLIANT_d2i
#undef  JAVA_COMPLIANT_d2l
#undef  NAN_CHECK_d2i
#undef  NAN_CHECK_d2l
#undef  BOUNDS_CHECK_d2l

#define USE_NATIVE_FREM
#undef USE_ANSI_FMOD
#define USE_NATIVE_FCOMPARE
#undef USE_ANSI_FCOMPARE

#define CVMdoubleNeg CVMdoubleNeg
__inline CVMJavaDouble CVMdoubleNeg(CVMJavaDouble op)
{
    _int64 *lp = (_int64 *)&op;   

    if (*lp ==  0x8000000000000001)
        *lp = 1;
    else if (*lp == 1)
        *lp = 0x8000000000000001;
    else
        op = -op;
    return op;
}

/* NOTE: In the implementation of CVMdouble2Float() below, we need to check
   for the case where the double value could be rounded up to Float.MIN_VALUE
   as well.  That's why we compare against Float.MIN_VALUE/2.0.
*/
#define CVM_DOUBLE_MIN_FLOAT_VALUE  (1.40129846432481707e-45)
#define CVMdouble2Float(val) \
    ((((val) > CVM_DOUBLE_MIN_FLOAT_VALUE) || \
      ((val) < -CVM_DOUBLE_MIN_FLOAT_VALUE)) ? \
        ((CVMJavaFloat)(val)) : \
        ((val) > (CVM_DOUBLE_MIN_FLOAT_VALUE/2.0)) ? \
            ((CVMJavaFloat)CVM_DOUBLE_MIN_FLOAT_VALUE) : \
            ((val) < (-CVM_DOUBLE_MIN_FLOAT_VALUE/2.0)) ? \
               ((CVMJavaFloat)-CVM_DOUBLE_MIN_FLOAT_VALUE) : \
                   ((val) >= 0.0) ? 0.0f : -0.0f)

extern double doubleRem(double, double);

#define CVMlongDiv(op1, op2)    \
        ((op1 == (_int64)(0x8000000000000000) && (op2) == -1) ? \
        (op1)  :                \
        (op1 / op2))

#define CVMlongRem(op1, op2)     \
         ((op1 == (_int64)(0x8000000000000000) && (op2) == -1) ?        \
         (op1 % 1) :            \
         (op1 % op2))

#endif /* _WIN32_DOUBLEWORD_ARCH_H */
