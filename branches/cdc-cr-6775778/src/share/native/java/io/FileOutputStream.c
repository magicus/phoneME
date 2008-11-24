/*
 * @(#)FileOutputStream.c	1.26 06/10/10
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
#include "jvm.h"

#include "io_util.h"
#include "java_io_FileOutputStream.h"
#include "javavm/include/porting/ansi/errno.h"

/*******************************************************************/
/*  BEGIN JNI ********* BEGIN JNI *********** BEGIN JNI ************/
/*******************************************************************/

#include "jni_statics.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"open", "(Ljava/lang/String;)V",
	(void *)&Java_java_io_FileOutputStream_open},
    {"write", "(I)V",
	(void *)&Java_java_io_FileOutputStream_write}
};

/**************************************************************
 * static methods to store field ID's in initializers
 */

JNIEXPORT void JNICALL 
Java_java_io_FileOutputStream_initIDs(JNIEnv *env, jclass fdClass)
{
    JNI_STATIC(java_io_FileOutputStream, fos_fd) = (*env)->GetFieldID(env, fdClass, "fd", "Ljava/io/FileDescriptor;");
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, fdClass, methods,
	sizeof methods / sizeof methods[0]);
}

/**************************************************************
 * Output stream
 */

JNIEXPORT void JNICALL
Java_java_io_FileOutputStream_open(JNIEnv *env, jobject thisObj, jstring path) {
    fileOpen(env, thisObj, path, JNI_STATIC(java_io_FileOutputStream, fos_fd), O_WRONLY | O_CREAT | O_TRUNC);
}

JNIEXPORT void JNICALL
Java_java_io_FileOutputStream_openAppend(JNIEnv *env, jobject thisObj, jstring path) {
    fileOpen(env, thisObj, path, JNI_STATIC(java_io_FileOutputStream, fos_fd), O_WRONLY | O_CREAT | O_APPEND);
}

JNIEXPORT void JNICALL
Java_java_io_FileOutputStream_write(JNIEnv *env, jobject thisObj, jint byte) {
    writeSingle(env, thisObj, byte, JNI_STATIC(java_io_FileOutputStream, fos_fd));
}

JNIEXPORT void JNICALL
Java_java_io_FileOutputStream_writeBytes(JNIEnv *env,
    jobject thisObj, jbyteArray bytes, jint off, jint len) {
    writeBytes(env, thisObj, bytes, off, len, JNI_STATIC(java_io_FileOutputStream, fos_fd));
#if 0
    /* %comment w001 */
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
#   define BUFSIZE 128
    char cbuf[BUFSIZE+1], *buf;

    if (len > BUFSIZE) {
	buf = (char *)malloc(len + 1);
    } else {
	buf = cbuf;
    }
    CVMD_gcUnsafeExec(ee, {
	int i = 0;
	CVMArrayOfByte* arrayobj =
	    (CVMArrayOfByte*)CVMID_icellDirect(ee, bytes);
	while (i < len) {
	    size_t l = (len - i) > BUFSIZE ? BUFSIZE : (len - i);
	    /* %comment w002 */
	    memcpy(&buf[i], &arrayobj->elems[off + i], l);
	    i += l;
	}
	buf[len] = '\0';
    });
    CVMconsolePrintf("%s", buf);
    if (buf != cbuf) {
	free(buf);
    }
#endif
}

JNIEXPORT void JNICALL
Java_java_io_FileOutputStream_close0(JNIEnv *env, jobject thisObj) {
    int fd = GET_FD(JNI_STATIC(java_io_FileOutputStream, fos_fd));
    SET_FD(-1, JNI_STATIC(java_io_FileOutputStream, fos_fd));    
    if ( (JVM_Close(fd) == -1) && (errno == ENOSPC)) {
        JNU_ThrowIOException(env, "No Space left on Device");
    }
}
