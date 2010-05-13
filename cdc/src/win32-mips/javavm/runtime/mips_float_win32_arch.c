/*
 * @(#)mips_float_win32_arch.c	1.7 06/10/10
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
	return (_int64)0; /* nan */
    }
    if (d <= -9223372036854775808.0) {
        return (_int64)0x8000000000000000;
    } else if (d >= 9223372036854775807.0) {
        return (_int64)0x7fffffffffffffff;
    }

    return (CVMJavaLong)d;	
} 


double
doubleRem(double dividend, double divisor) {
    double res;

    if (!_finite(divisor)) {
	if (_isnan(divisor))
	    return divisor;
	if (_finite(dividend))
	    return dividend;
    }
    res = fmod(dividend, divisor);
    /* With MSVC, "res == 0" succeeds when res is NaN. */
    if (!_isnan(res) && res == 0) {  /*MSFT fmod doesn't do -0.0 right*/
      return _copysign(0.0,dividend);
    } else {
      return res;
    }
}

float
floatRem(float dividend0, float divisor0) {
    double res;
    double dividend = dividend0, divisor = divisor0;

    return (float)(doubleRem(dividend, divisor));
}
