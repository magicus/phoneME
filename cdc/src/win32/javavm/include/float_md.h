/*
 * @(#)float_md.h	1.6 06/10/10
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

#ifndef _WIN32_FLOAT_MD_H
#define _WIN32_FLOAT_MD_H

#include "javavm/include/float_arch.h"

#include "portlibs/ansi_c/float.h"

/*
 * Float Comparisons (returning int value 0, 1, or -1) 
 */
#define CVMfloatCompare(op1, op2, opcode_value) \
    ((_isnan((double)op1) || _isnan((double)op2)) ? (opcode_value) : \
     ((op1) < (op2)) ? -1 : \
     ((op1) > (op2)) ? 1 : \
     ((op1) == (op2)) ? 0 : \
     ((opcode_value) == -1 || (opcode_value) == 1) ? (opcode_value) :\
     0)


/*
 * Float Conversions:
 */

#define longMin      (_int64)(0x8000000000000000)
#define longMax      (_int64)(0x7fffffffffffffff)

#define CVMfloat2Long(val) \
    float2Long((val))

/*
 * JAVA_COMPLIANT_f2l, NAN_CHECK_f2l, or neither (but not both) may be
 * defined.
 *
 * If NAN_CHECK_f2l is defined, then BOUNDS_CHECK_f2l may be defined.
 */
/* #define float2Long(val)   (CVMJavaLong)((val)) */


/*
#ifdef JAVA_COMPLIANT_f2l

#define float2Long(val)   (CVMJavaLong)((val))

#elif (defined(NAN_CHECK_f2l))

#define float2Long(val)   ((val) != (val) ? 0LL : float2Long0(val))

#else
extern CVMJavaLong float2Long(CVMJavaFloat d);
*/
/*
#endif
*/
/*
#ifdef BOUNDS_CHECK_f2l

#define float2Long0(val) \
    (  ((val) <= -9223372036854775808.0) ? longMin : \
                   ((val) >= 9223372036854775807.0) ? longMax : \
                   (CVMJavaLong)(val) )

#else
*/

/* #define float2Long0(val)  ((CVMJavaLong)(val)) */
extern CVMJavaLong float2Long(CVMJavaFloat d);

/*
#endif
*/

#endif /* _WIN32_FLOAT_MD_H */
