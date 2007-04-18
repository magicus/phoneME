/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved. 
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
 */


#ifndef PISCES_MATH_H
#define PISCES_MATH_H

#include <PiscesDefs.h>

#define PI_DOUBLE 3.141592653589793L

#define PISCES_PI ((jint)(PI_DOUBLE*65536.0))
#define PISCES_TWO_PI ((jint)(2.0*PI_DOUBLE*65536.0))
#define PISCES_PI_OVER_TWO ((jint)((PI_DOUBLE/2.0)*65536.0))
#define PISCES_SQRT_TWO ((jint)(1.414213562373095*65536.0))

jboolean piscesmath_moduleInitialize();
void piscesmath_moduleFinalize();

jint piscesmath_sin(jint theta);
jint piscesmath_cos(jint theta);
jint piscesmath_isqrt(jint x);
jlong piscesmath_lsqrt(jlong x);
jdouble piscesmath_dhypot(jdouble x, jdouble y);
jint piscesmath_ihypot(jint x, jint y);
jlong piscesmath_lhypot(jlong x, jlong y);

#endif
