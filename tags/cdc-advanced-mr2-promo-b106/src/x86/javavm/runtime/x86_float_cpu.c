/*
 * @(#)x86_float_cpu.c	1.13 06/10/10
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

#include "javavm/include/porting/float.h"
#include "javavm/include/porting/ansi/string.h"

#include <math.h> /* fmod */
#include <strings.h> /* bcopy */

#if (!defined(JAVA_COMPLIANT_f2i) && !defined(NAN_CHECK_f2i))

#undef _isnan
#define _isnan(d)	CVMfloatCompare(d, d, 1)

CVMJavaInt
float2Int(CVMJavaFloat f)
{
    CVMJavaLong ltmp = (CVMJavaLong)f;
    CVMJavaInt itmp = (CVMJavaInt)ltmp;

    if (itmp == ltmp) {
	return itmp;
    } else {
	if (_isnan((double)f)) {
	    return 0;
	}
	if (f < 0.0) {
	    return 0x80000000;
	} else {
	    return 0x7fffffff;
	}
    }
}

#endif  /* (!defined(JAVA_COMPLIANT_f2i) && !defined(NAN_CHECK_f2i)) */


#if (!defined(JAVA_COMPLIANT_f2l) && !defined(NAN_CHECK_f2l))

CVMJavaLong
float2Long(CVMJavaFloat f)
{
    CVMJavaLong ltmp = (CVMJavaLong)f;
    if (ltmp != longMin) {
        return ltmp;
    } else {
        if (_isnan((double)f)) {
	    return 0;
	}
        if (f < 0.0) {
            return longMin;
        } else {
            return longMax;
        }
    }
}

#endif /* (!defined(JAVA_COMPLIANT_f2l) && !defined(NAN_CHECK_f2l)) */

#if (!defined(JAVA_COMPLIANT_d2i) && !defined(NAN_CHECK_d2i))

CVMJavaInt
double2Int(CVMJavaDouble d)
{
    CVMJavaLong ltmp = (CVMJavaLong)d;
    CVMJavaInt itmp = (CVMJavaInt)ltmp;

    if (itmp == ltmp) {
	return itmp;
    } else {
	if (_isnan(d)) {
	    return 0;
	}
	if (d < 0.0) {
	    return 0x80000000;
	} else {
	    return 0x7fffffff;
	}
    }
}

#endif /* (!defined(JAVA_COMPLIANT_d2i) && !defined(NAN_CHECK_d2i)) */

#if (!defined(JAVA_COMPLIANT_d2l) && !defined(NAN_CHECK_d2l))

CVMJavaLong
double2Long(CVMJavaDouble d)
{
    CVMJavaLong ltmp = (CVMJavaLong)d;
    if (ltmp != longMin) {
        return ltmp;
    } else {
        if (_isnan(d)) {
	    return 0;
	}
        if (d < 0.0) {
            return longMin;
        } else {
            return longMax;
        }
    }
}

#endif /* (!defined(JAVA_COMPLIANT_d2l) && !defined(NAN_CHECK_d2l)) */

double
doubleRem(double op1, double op2) {
#ifndef _MSC_VER
    if (op1 == (1.0 / 0.0) || op1 == (-1.0 / 0.0)	/* Inf or -Inf */
	|| op2 == 0.0 || op2 == -0.0)
#else
    double zero = 0.0;
    if (op1 == (1.0 / zero) || op1 == (-1.0 / zero)	/* Inf or -Inf */
	|| op2 == 0.0 || op2 == -0.0)
#endif
    {
#ifndef _MSC_VER
	return (0.0 / 0.0);	/* NaN */
#else
	double zero = 0.0;
	return zero / zero;	/* NaN */
#endif
    } else {
        CVMInt64 lres, lop1, rlli, olli;
        long rli, oli;

	double res = fmod(op1, op2);

	/* sign must match dividend */
        bcopy((void *)&res, (void *)&lres, 8);
        rlli = (CVMInt64)lres >> 63;
        rli = (long) rlli & 0x00000001;

        bcopy((void *)&op1, (void *)&lop1, 8);
        olli = (CVMInt64)lop1 >> 63;
        oli = (long) olli & 0x00000001;

        if (res == res && rli != oli) {
            res = -res;
        }
	return res;
    }
}

float
floatRem(float op1, float op2) {
#ifdef _MSC_VER
    double zero = 0.0;
#else
#define zero 0.0
#endif
    if (op1 == (1.0 / zero) || op1 == (-1.0 / zero)	/* Inf or -Inf */
	|| op2 == 0.0 || op2 == -0.0)
    {
	return (0.0 / zero);	/* NaN */
    } else {
        long lres, lop1, rli, oli;
	float res = fmod(op1, op2);

	/* sign must match dividend */
        bcopy((void *)&res, (void *)&lres, 4);
        rli = lres >> 31;
        rli = rli & 0x00000001;

        bcopy((void *)&op1, (void *)&lop1, 4);
        oli = lop1 >> 31;
        oli = oli & 0x00000001;

        if (res == res && rli != oli) {
            res = -res;
        }
	return res;
    }
}
