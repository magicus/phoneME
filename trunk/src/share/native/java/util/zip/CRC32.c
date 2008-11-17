/*
 * @(#)CRC32.c	1.19 06/10/10
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

/*
 * Native method support for java.util.zip.CRC32
 */

#include "jni.h"
#include "jni_util.h"
#include "zlib.h"

#include "java_util_zip_CRC32.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"update", "(II)I", (void *)&Java_java_util_zip_CRC32_update}
};

JNIEXPORT void JNICALL
Java_java_util_zip_CRC32_init(JNIEnv *env, jclass cls)
{
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, cls, methods,
	sizeof methods / sizeof methods[0]);
}

JNIEXPORT jint JNICALL
Java_java_util_zip_CRC32_update(JNIEnv *env, jclass cls, jint crc, jint b)
{
    Bytef buf[1];

    buf[0] = (Bytef)b;
    return crc32(crc, buf, 1);
}

JNIEXPORT jint JNICALL
Java_java_util_zip_CRC32_updateBytes(JNIEnv *env, jclass cls, jint crc,
				     jarray b, jint off, jint len)
{
    Bytef *buf = (Bytef *)(*env)->GetPrimitiveArrayCritical(env, b, 0);
    if (buf) {
        crc = crc32(crc, buf + off, len);
	(*env)->ReleasePrimitiveArrayCritical(env, b, buf, 0);
    }
    return crc;
}

JNIEXPORT jint ZIP_CRC32(jint crc, const jbyte *buf, jint len)
{
    return crc32(crc, (Bytef*)buf, len);
}
