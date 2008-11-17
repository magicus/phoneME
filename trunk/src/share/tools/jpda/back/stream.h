/*
 * @(#)stream.h	1.14 06/10/25
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

#ifndef JDWP_STREAM_H
#define JDWP_STREAM_H

#include <jni.h>
#include "javavm/include/porting/endianness.h"


#if CVM_ENDIANNESS == CVM_LITTLE_ENDIAN

#define HOST_TO_JAVA_CHAR(x) (((x & 0xff) << 8) | ((x >> 8) & (0xff)))
#define HOST_TO_JAVA_SHORT(x) (((x & 0xff) << 8) | ((x >> 8) & (0xff)))
#define HOST_TO_JAVA_INT(x)                                             \
                  ((x << 24) |                                          \
                   ((x & 0x0000ff00) << 8) |                            \
                   ((x & 0x00ff0000) >> 8) |                            \
                   (((UNSIGNED_JINT)(x & 0xff000000)) >> 24))
#define HOST_TO_JAVA_LONG(x)                                            \
                  ((x << 56) |                                          \
                   ((x & 0x0000ff00) << 40) |                           \
                   ((x & 0x00ff0000) << 24) |                           \
                   ((x & 0xff000000) << 8)  |                           \
                   ((x >> 8) & 0xff000000)  |                           \
                   ((x >> 24) & 0x00ff0000) |                           \
                   ((x >> 40) & 0x0000ff00) |                           \
                   ((((UNSIGNED_JLONG)x) >> 56) & 0x000000ff)); 

jfloat  stream_encodeFloat(jfloat theFloat);
#define HOST_TO_JAVA_FLOAT(x) stream_encodeFloat(x)

#else 

#define HOST_TO_JAVA_CHAR(x)   (x)
#define HOST_TO_JAVA_SHORT(x)  (x)
#define HOST_TO_JAVA_INT(x)    (x)
#define HOST_TO_JAVA_LONG(x)   (x)
#define HOST_TO_JAVA_FLOAT(x)  (x)

#endif
                             
/* If the double word endianness does not match the byte endianness,
   then we will also have to massage the bytes: */
#if (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN) || \
    (CVM_ENDIANNESS != CVM_DOUBLE_ENDIANNESS)

jdouble stream_encodeDouble(jdouble d);
#define HOST_TO_JAVA_DOUBLE(x) stream_encodeDouble(x)

#else

#define HOST_TO_JAVA_DOUBLE(x) (x)

#endif

#define JAVA_TO_HOST_CHAR(x)   HOST_TO_JAVA_CHAR(x)  
#define JAVA_TO_HOST_SHORT(x)  HOST_TO_JAVA_SHORT(x) 
#define JAVA_TO_HOST_INT(x)    HOST_TO_JAVA_INT(x)   
#define JAVA_TO_HOST_LONG(x)   HOST_TO_JAVA_LONG(x)  
#define JAVA_TO_HOST_FLOAT(x)  HOST_TO_JAVA_FLOAT(x) 
#define JAVA_TO_HOST_DOUBLE(x) HOST_TO_JAVA_DOUBLE(x)

#endif
