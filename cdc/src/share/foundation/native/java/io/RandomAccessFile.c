/*
 * @(#)RandomAccessFile.c	1.39 06/10/10
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
#include "jvm.h"

#include "io_util.h"
#include "java_io_RandomAccessFile.h"
#include "javavm/include/porting/ansi/errno.h"

/*
 * static method to store field ID's in initializers
 */

#include "jni_statics.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"read", "()I",
	(void *)&Java_java_io_RandomAccessFile_read},
    {"write", "(I)V",
	(void *)&Java_java_io_RandomAccessFile_write}
};

JNIEXPORT void JNICALL 
Java_java_io_RandomAccessFile_initIDs(JNIEnv *env, jclass fdClass) {
    JNI_STATIC(java_io_RandomAccessFile, raf_fd) = (*env)->GetFieldID(env, fdClass, "fd", "Ljava/io/FileDescriptor;");
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, fdClass, methods,
	sizeof methods / sizeof methods[0]);
}


JNIEXPORT void JNICALL
Java_java_io_RandomAccessFile_open(JNIEnv *env,
				   jobject thisObj, jstring path, jint mode)
{
    int flags = 0;
    if (mode & java_io_RandomAccessFile_O_RDONLY)
	flags = O_RDONLY;
    else if (mode & java_io_RandomAccessFile_O_RDWR) {
	flags = O_RDWR | O_CREAT;
        /* NOTE: We don't need to support these modes
	   as we don't support nio in CDC: */
#ifdef JAVASE
	if (mode & java_io_RandomAccessFile_O_SYNC)
	    flags |= O_SYNC;
	else if (mode & java_io_RandomAccessFile_O_DSYNC)
	    flags |= O_DSYNC;
#endif
    }
    fileOpen(env, thisObj, path, JNI_STATIC(java_io_RandomAccessFile, raf_fd),
	     flags);
}

JNIEXPORT jint JNICALL
Java_java_io_RandomAccessFile_read(JNIEnv *env, jobject thisObj) {
    return readSingle(env, thisObj,
		      JNI_STATIC(java_io_RandomAccessFile, raf_fd));
}

JNIEXPORT jint JNICALL
Java_java_io_RandomAccessFile_readBytes(JNIEnv *env,
    jobject thisObj, jbyteArray bytes, jint off, jint len) {
    return readBytes(env, thisObj, bytes, off, len,
		     JNI_STATIC(java_io_RandomAccessFile, raf_fd));
}

JNIEXPORT void JNICALL
Java_java_io_RandomAccessFile_write(JNIEnv *env, jobject thisObj, jint byte) {
    writeSingle(env, thisObj, byte,
		JNI_STATIC(java_io_RandomAccessFile, raf_fd));
}

JNIEXPORT void JNICALL
Java_java_io_RandomAccessFile_writeBytes(JNIEnv *env,
    jobject thisObj, jbyteArray bytes, jint off, jint len) {
    writeBytes(env, thisObj, bytes, off, len,
	       JNI_STATIC(java_io_RandomAccessFile, raf_fd));
}

JNIEXPORT jlong JNICALL
Java_java_io_RandomAccessFile_getFilePointer(JNIEnv *env, jobject thisObj) {
    int fd;
    jlong ret;

    fd = GET_FD(JNI_STATIC(java_io_RandomAccessFile, raf_fd));
    ret = JVM_Lseek(fd, CVMlongConstZero(), SEEK_CUR);
    if (CVMlongEq(ret, CVMint2Long(-1))) {
      	JNU_ThrowIOExceptionWithLastError(env, "Seek failed");
    }
    return ret;
}

JNIEXPORT jlong JNICALL
Java_java_io_RandomAccessFile_length(JNIEnv *env, jobject thisObj) {
    int fd;
    jlong cur = jlong_zero;
    jlong end = jlong_zero;

    fd = GET_FD(JNI_STATIC(java_io_RandomAccessFile, raf_fd));

    cur = JVM_Lseek(fd, CVMlongConstZero(), SEEK_CUR);
    if (CVMlongEq(cur, CVMint2Long(-1))) {
      	JNU_ThrowIOExceptionWithLastError(env, "Seek failed");
    } else {
	end = JVM_Lseek(fd, CVMlongConstZero(), SEEK_END);
	if (CVMlongEq(end, CVMint2Long(-1))) {
	    JNU_ThrowIOExceptionWithLastError(env, "Seek failed");
	} else if (CVMlongEq(JVM_Lseek(fd, cur, SEEK_SET), CVMint2Long(-1))) {
	    JNU_ThrowIOExceptionWithLastError(env, "Seek failed");
	}
    }
    return end;
}

JNIEXPORT void JNICALL
Java_java_io_RandomAccessFile_seek(JNIEnv *env,
		    jobject thisObj, jlong pos) {

    int fd;

    fd = GET_FD(JNI_STATIC(java_io_RandomAccessFile, raf_fd));
    if (pos < jlong_zero) {
        JNU_ThrowIOException(env, "Negative seek offset");
    } else if (CVMlongEq(JVM_Lseek(fd, pos, SEEK_SET), CVMint2Long(-1))) {
      	JNU_ThrowIOExceptionWithLastError(env, "Seek failed");
    }
}

JNIEXPORT void JNICALL
Java_java_io_RandomAccessFile_setLength(JNIEnv *env, jobject thisObj,
					jlong newLength)
{
    int fd;
    jlong cur;

    fd = GET_FD(JNI_STATIC(java_io_RandomAccessFile, raf_fd));
    cur = JVM_Lseek(fd, CVMlongConstZero(), SEEK_CUR);
    if (CVMlongEq(cur, CVMint2Long(-1))) {
	goto fail;
    }
    if (JVM_SetLength(fd, newLength) == -1) {
	goto fail;
    }
    if (CVMlongGt(cur, newLength)) {
	if (CVMlongEq(JVM_Lseek(fd, CVMlongConstZero(), SEEK_END),
		      CVMint2Long(-1))) {
	    goto fail;
	}
    } else {
	if (CVMlongEq(JVM_Lseek(fd, cur, SEEK_SET), CVMint2Long(-1))) {
	    goto fail;
	}
    }
    return;

 fail:
    JNU_ThrowIOExceptionWithLastError(env, "setLength failed");
}


JNIEXPORT void JNICALL
Java_java_io_RandomAccessFile_close0(JNIEnv *env, jobject thisObj) {
    int fd = GET_FD(JNI_STATIC(java_io_RandomAccessFile, raf_fd));
    SET_FD(-1, JNI_STATIC(java_io_RandomAccessFile, raf_fd));
    if ( (JVM_Close(fd) == -1) && (errno == ENOSPC)) {
        JNU_ThrowIOException(env, "No Space left on Device");
    }
}
