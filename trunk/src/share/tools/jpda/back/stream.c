/*
 * @(#)stream.c	1.12 06/10/25
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

#include "util.h" /* Needed for linux, do not remove */
#include "stream.h"

#if CVM_ENDIANNESS == CVM_LITTLE_ENDIAN

jfloat 
stream_encodeFloat(jfloat theFloat) 
{
    union {
        jfloat f;
        jint i;
    } sF;

    sF.f = theFloat;

    sF.i = HOST_TO_JAVA_INT(sF.i);

    return sF.f;
}

#endif /* CVM_ENDIANNESS == CVM_LITTLE_ENDIAN */

#if (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN) || \
    (CVM_ENDIANNESS != CVM_DOUBLE_ENDIANNESS)

jdouble 
stream_encodeDouble(jdouble d)
{
    union {
        jdouble d;
        jlong l;
#if CVM_ENDIANNESS != CVM_DOUBLE_ENDIANNESS
        jint i[2];
#endif
    } sD;

    sD.d = d;

#if CVM_ENDIANNESS != CVM_DOUBLE_ENDIANNESS
    /* The desired word endianness is big endian.

       If the platform has little endian byte order and little endian word
       order, then the call to HOST_TO_JAVA_LONG() below will convert the word
       endianness as well.

       If the platform has little endian byte order and big endian word order,
       then we need to swap the high and low words first to cancel out the
       word swapping effect of the HOST_TO_JAVA_LONG() macro below.

       If the platform has big endian byte order and little endian word order,
       then we only need to swap the word order.  No need to call the
       HOST_TO_JAVA_LONG() macro below.

       If the platform has big endian byte order and big endian word order,
       then we have nothing to do and won't be calling this function anyway.

       Hence, we only need to do the word order swap here if the byte order
       endianness does not equals the word order endianness for doubles.
    */
    {
        jint i = sD.i[0];
        sD.i[0] = sD.i[1];
        sD.i[1] = i;
    }
#endif

#if CVM_ENDIANNESS == CVM_LITTLE_ENDIAN
    /* The desired byte endianness is big endian.  If the platform is little
       endian, then we will need to reverse the byte order.  We can do that
       using the HOST_TO_JAVA_LONG() macro which does the same thing for
       64-bit long ints. */
    sD.l = HOST_TO_JAVA_LONG(sD.l);
#endif

    return sD.d;
}

#endif /* (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN) || \
          (CVM_ENDIANNESS != CVM_DOUBLE_ENDIANNESS) */

