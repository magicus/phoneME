/*
 * @(#)thumb_glue.c	1.3 06/10/10
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
 */

 /*
 * These are wrappers for transitioning from Thumb code
 * to ARM code, when the ARM code is compiled for Hard FP
 * and the GLib doesn't have the necessary ingredients.
 * This is all VERY GCC SPECIFIC!!
 * IT MUST BE COMPILED AS ARM CODE WITH THUMB-INTERWORKING
 * else the whole scheme will fail.
 */

/* these are for the locking routine at the end */
#include "javavm/include/sync.h"

/* Note to self: We may have to use unions and lie about the
 * param and return type to get past the obvious calling-convention issues
 */

float
__floatsisf( int i ){
    /* single int to single float */
    return (float)i;
}

double
__floatsidf( int i ){
    /* single int to double float */
    return (double)i;
}

double
__extendsfdf2( float f ){
    /* single float to double float */
    return (double)f;
}

float
__addsf3( float a, float b ){
    /* single precision addition */
    return a+b;
}

float
__subsf3( float a, float b ){
    /* single precision subtraction */
    return a-b;
}

float
__mulsf3( float a, float b ){
    /* single precision multiplication */
    return a+b;
}

float
__divsf3( float a, float b ){
    /* single precision division */
    return a/b;
}

int
__fixsfsi( float a ){
    /* single float to single int */
    return (int)a;
}

double
__adddf3( double a, double b ){
    /* double precision addition */
    return a+b;
}

double
__muldf3( double a, double b ){
    /* double precision multiplication */
    return a*b;
}

float
__truncdfsf2( double d ){
    /* double float to single float */
    return (float)d;
}

double
__ltdf2( long l ){
    /* long (int) to double float */
    return (double)l;
}

CVMAddr
CVMatomicSwapRoutine(CVMAddr newvalue, volatile CVMAddr *addr){
    return CVMatomicSwapImpl(newvalue, addr);
}
