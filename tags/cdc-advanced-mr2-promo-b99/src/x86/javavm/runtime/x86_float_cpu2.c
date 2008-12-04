/*
 * @(#)x86_float_cpu2.c	1.11 06/10/10
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
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/float.h"
#include "javavm/include/porting/ansi/string.h"
#include <math.h>

CVMJavaInt
CVMfloat2Int(CVMJavaFloat f)
{
    if (CVMisNan((double)f)) {
	return 0;
    }
    if (f <= -2147483648.0) {
	return 0x80000000;
    } else if (f >= 2147483647.0) {
	return 0x7fffffff;
    } else {
	return ((CVMJavaInt)f);
    }
}

CVMJavaLong
float2Long(CVMJavaFloat f)
{
    if (CVMisNan((double)f)) {
	return (CVMJavaLong)0;
    }
    return (  (f <= -9223372036854775808.0) ? longMin : 
                   (f >= 9223372036854775807.0) ? longMax : 
                   (CVMJavaLong)f );
}

CVMJavaInt
double2Int(CVMJavaDouble d)
{
    if (CVMisNan(d)) {
	return 0;
    }
    if (d <= -2147483648.0) {
	return 0x80000000;
    } else if (d >= 2147483647.0) {
	return 0x7fffffff;
    } else {
	return ((CVMJavaInt)d);
    }
}

CVMJavaLong
double2Long(CVMJavaDouble d)
{
    if (CVMisNan(d)) {
	return (CVMJavaLong)0;
    }
    return    (  (d <= -9223372036854775808.0) ? longMin : \
       (d >= 9223372036854775807.0) ? longMax : \
       (CVMJavaLong)(d) );
}

double
doubleRem(double dividend, double divisor) {
    double res;

    if (!CVMisFinite(divisor)) {
	if (CVMisNan(divisor))
	    return divisor;
	if (CVMisFinite(dividend))
	    return dividend;
    }
    res = fmod(dividend, divisor);
    /* With MSVC, "res == 0" succeeds when res is NaN. */
    if (!CVMisNan(res) && res == 0) {  /*MSFT fmod doesn't do -0.0 right*/
      return CVMcopySign(0.0,dividend);
    } else {
      return res;
    }
}

float
floatRem(float dividend0, float divisor0) {
    double res;
    double dividend = dividend0, divisor = divisor0;

	return (doubleRem(dividend, divisor));
}


CVMJavaDouble
CVMlongBits2Double(CVMJavaLong val)
{
    union {
	unsigned int x[2];
	CVMJavaLong l;
	CVMJavaDouble d;
    } bits;
    unsigned int tmp;

    bits.l = val;
    tmp = bits.x[0];
    bits.x[0] = bits.x[1];
    bits.x[1] = tmp;

    return bits.d;
}

CVMJavaLong
CVMdouble2LongBits(CVMJavaDouble val)
{
    union {
	unsigned int x[2];
	CVMJavaLong l;
	CVMJavaDouble d;
    } bits;
    unsigned int tmp;

    bits.d = val;
    tmp = bits.x[0];
    bits.x[0] = bits.x[1];
    bits.x[1] = tmp;

    return bits.l;
}
