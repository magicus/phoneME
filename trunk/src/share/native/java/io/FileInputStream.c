/*
 * @(#)FileInputStream.c	1.30 06/10/10
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

#include "jni.h"
#include "jni_util.h"
#include "jlong.h"
#include "io_util.h"
#include "jvm.h"

#include "java_io_FileInputStream.h"
#include "javavm/include/porting/ansi/errno.h"

/*******************************************************************/
/*  BEGIN JNI ********* BEGIN JNI *********** BEGIN JNI ************/
/*******************************************************************/

#include "jni_statics.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"read", "()I", (void *)&Java_java_io_FileInputStream_read}
};

/**************************************************************
 * static methods to store field ID's in initializers
 */

JNIEXPORT void JNICALL
Java_java_io_FileInputStream_initIDs(JNIEnv *env, jclass fdClass)
{
    JNI_STATIC(java_io_FileInputStream, fis_fd) = (*env)->GetFieldID(env, fdClass, "fd", "Ljava/io/FileDescriptor;");
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, fdClass, methods,
	sizeof methods / sizeof methods[0]);
}

/**************************************************************
 * Input stream
 */

JNIEXPORT void JNICALL
Java_java_io_FileInputStream_open(JNIEnv *env, jobject thisObj, jstring path) {
    fileOpen(env, thisObj, path, JNI_STATIC(java_io_FileInputStream, fis_fd), O_RDONLY);
}

JNIEXPORT void JNICALL
Java_java_io_FileInputStream_close0(JNIEnv *env, jobject thisObj) {
    int fd = GET_FD(JNI_STATIC(java_io_FileInputStream, fis_fd));
    SET_FD(-1, JNI_STATIC(java_io_FileInputStream, fis_fd));
    if ( (JVM_Close(fd) == -1) && (errno == ENOSPC)) {
        JNU_ThrowIOException(env, "No Space left on Device");
    }
}

JNIEXPORT jint JNICALL
Java_java_io_FileInputStream_read(JNIEnv *env, jobject thisObj) {
    return readSingle(env, thisObj, JNI_STATIC(java_io_FileInputStream, fis_fd));
}

JNIEXPORT jint JNICALL
Java_java_io_FileInputStream_readBytes(JNIEnv *env, jobject thisObj,
	jbyteArray bytes, jint off, jint len) {
    return readBytes(env, thisObj, bytes, off, len, JNI_STATIC(java_io_FileInputStream, fis_fd));
}

JNIEXPORT jlong JNICALL
Java_java_io_FileInputStream_skip(JNIEnv *env, jobject thisObj, jlong toSkip) {
    jlong cur = jlong_zero;
    jlong end = jlong_zero;
    int fd = GET_FD(JNI_STATIC(java_io_FileInputStream, fis_fd));

    cur = JVM_Lseek(fd, CVMlongConstZero(), SEEK_CUR);
    if (CVMlongEq(cur, CVMint2Long(-1))) {
        JNU_ThrowIOExceptionWithLastError(env, "Seek error");
    } else {
	end = JVM_Lseek(fd, toSkip, SEEK_CUR);
	if (CVMlongEq(end, CVMint2Long(-1))) {
	    JNU_ThrowIOExceptionWithLastError(env, "Seek error");
	}
    }
    return CVMlongSub(end, cur);
}

JNIEXPORT jint JNICALL
Java_java_io_FileInputStream_available(JNIEnv *env, jobject thisObj) {
    int fd;
    jlong ret;

    fd = GET_FD(JNI_STATIC(java_io_FileInputStream, fis_fd));

    if (JVM_Available(fd, &ret)) {
        if (CVMlongGt(ret, CVMint2Long(INT_MAX))) {
	    ret = CVMint2Long(INT_MAX);
	}
	return CVMlong2Int(ret);
    }

    JNU_ThrowIOExceptionWithLastError(env, NULL);
    return 0;
}
