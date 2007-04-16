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


#include <PiscesMath.h>

#include <PiscesSysutils.h>

#define PISCES_SINTAB_LG_ENTRIES 10
#define PISCES_SINTAB_ENTRIES (1 << PISCES_SINTAB_LG_ENTRIES)

static jint* sintab = NULL;

jboolean
piscesmath_moduleInitialize() {
    if (sintab == NULL) {
        jint i;
        sintab = (jint*)PISCESmalloc((PISCES_SINTAB_ENTRIES + 1) 
                 * sizeof(jint));
        if (sintab == NULL) {
            return XNI_FALSE;
        }
        for (i = 0; i < PISCES_SINTAB_ENTRIES + 1; i++) {
            double theta = i*(PI_DOUBLE/2.0)/PISCES_SINTAB_ENTRIES;
            sintab[i] = (jint)(PISCESsin(theta)*65536.0);
        }
    }

    return XNI_TRUE;
}

void
piscesmath_moduleFinalize() {
    PISCESfree(sintab);
    sintab = NULL;
}

jint
piscesmath_sin(jint theta) {
    jint sign = 1;
    jint itheta;
    if (theta < 0) {
        theta = -theta;
        sign = -1;
    }
    // 0 <= theta
    while (theta >= PISCES_TWO_PI) {
        theta -= PISCES_TWO_PI;
    }
    // 0 <= theta < 2*PI
    if (theta >= PISCES_PI) {
        theta = PISCES_TWO_PI - theta;
        sign = -sign;
    }
    // 0 <= theta < PI
    if (theta > PISCES_PI_OVER_TWO) {
        theta = PISCES_PI - theta;
    }
    // 0 <= theta <= PI/2
    itheta = (jint)((jlong)theta*PISCES_SINTAB_ENTRIES/(PISCES_PI_OVER_TWO));
    return sign*sintab[itheta];
}

jint
piscesmath_cos(jint theta) {
    return piscesmath_sin(PISCES_PI_OVER_TWO - theta);
}

// From Ken Turkowski, _Fixed-Point Square Root_, In Graphics Gems V
jint
piscesmath_isqrt(jint x) {
    jint fracbits = 16;

    jint root = 0;
    jint remHi = 0;
    jint remLo = x;
    jint count = 15 + fracbits/2;

    do {
        jint testdiv;
        // N.B. - unsigned shift R
        remHi = (remHi << 2) | ((unsigned int)remLo >> 30); 
        remLo <<= 2;
        root <<= 1;
        testdiv = (root << 1) + 1;
        if (remHi >= testdiv) {
            remHi -= testdiv;
            root++;
        }
    } while (count-- != 0);

    return root;
}

jlong
piscesmath_lsqrt(jlong x) {
    jint fracbits = 16;

    jlong root = 0;
    jlong remHi = 0;
    jlong remLo = x;
    jint count = 31 + fracbits/2;

    do {
        jlong testdiv;
        // N.B. - unsigned shift R
        remHi = (remHi << 2) | ((ulong64)remLo >> 62); 
        remLo <<= 2;
        root <<= 1;
        testdiv = (root << 1) + 1;
        if (remHi >= testdiv) {
            remHi -= testdiv;
            root++;
        }
    } while (count-- != 0);

    return root;
}

jdouble
piscesmath_dhypot(jdouble x, jdouble y) {
    return PISCESsqrt(x*x + y*y);
}

jint
piscesmath_ihypot(jint x, jint y) {
    return (jint)((piscesmath_lsqrt((jlong)x*x + (jlong)y*y) + 128) >> 8);
}

jlong
piscesmath_lhypot(jlong x, jlong y) {
    return (piscesmath_lsqrt(x*x + y*y) + 128) >> 8;
}
