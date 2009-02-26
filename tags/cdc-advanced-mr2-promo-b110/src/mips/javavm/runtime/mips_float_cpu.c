/*
 * @(#)mips_float_cpu.c	1.5 06/10/10
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
#include "javavm/include/porting/vm-defs.h"
#include "javavm/include/porting/float.h"
#include "javavm/include/porting/ansi/string.h"
#include <math.h>
#include <float.h>

CVMJavaInt
float2Int(CVMJavaFloat f)
{
    if (f != f) {
        return 0; /* it's a nan */
    }

    if (f <= -2147483648.0) {
        return 0x80000000;
    } else if (f >= 2147483647.0) {
        return 0x7fffffff;
    }

    return (CVMJavaInt)f;

}

CVMJavaLong
float2Long(CVMJavaFloat f)
{ 
    if (f != f) {
	return 0; /* nan */
    }

    if (f <= -9223372036854775808.0) {
        return longMin;
    } else if (f >= 9223372036854775807.0) {
        return longMax;
    }

    return (CVMJavaLong)f;    
}

CVMJavaInt
double2Int(CVMJavaDouble d)
{
    if (d != d) {
        return 0; /* nan */
    }

    if (d <= -2147483648.0) {
        return 0x80000000;
    } else if (d >= 2147483647.0) {
        return 0x7fffffff;
    }
    
    return (CVMJavaInt)d;
}

CVMJavaLong
double2Long(CVMJavaDouble d)
{
    if (d != d) {
	return 0LL; /* nan */
    }
    if (d <= -9223372036854775808.0) {
        return 0x8000000000000000LL;
    } else if (d >= 9223372036854775807.0) {
        return 0x7fffffffffffffffLL;
    }

    return (CVMJavaLong)d;
} 

double
doubleRem(double op1, double op2) {
    if (op1 == (1.0 / 0.0) || op1 == (-1.0 / 0.0)	/* Inf or -Inf */
	|| op2 == 0.0 || op2 == -0.0)
    {
	return (0.0 / 0.0);	/* NaN */
    } else {
        long long lres, lop1, rlli, olli;
        long rli, oli;

	double res = fmod(op1, op2);

	/* sign must match dividend */
        bcopy((void *)&res, (void *)&lres, 8);
        rlli = (long long)lres >> 63;
        rli = (long) rlli & 0x00000001;

        bcopy((void *)&op1, (void *)&lop1, 8);
        olli = (long long)lop1 >> 63;
        oli = (long) olli & 0x00000001;

        if (res == res && rli != oli) {
            res = -res;
        }
	return res;
    }
}

float
floatRem(float op1, float op2) {
    if (op1 == (1.0 / 0.0) || op1 == (-1.0 / 0.0)	/* Inf or -Inf */
	|| op2 == 0.0 || op2 == -0.0)
    {
	return (0.0 / 0.0);	/* NaN */
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
