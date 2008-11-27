/*
 * @(#)io_util.c	1.95 06/10/10
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

#include "javavm/include/ansi2cvm.h"
#include "javavm/include/interpreter.h"

/* IO helper functions */


jint
readSingle(JNIEnv *env, jobject thisObj, jfieldID fid) {
    jint fd, nread;
    char ret;

    fd = GET_FD(fid);

    nread = JVM_Read(fd, &ret, 1);
    if (nread == 0) { /* EOF */
	return -1;
    } else if (nread == JVM_IO_ERR) { /* error */
	JNU_ThrowIOExceptionWithLastError(env, "Read error");
    } else if (nread == JVM_IO_INTR) {
        JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
    }
    return ret & 0xFF;
}


jint
readBytes(JNIEnv *env, jobject thisObj, jbyteArray bytes,
	  jint off, jint len, jfieldID fid) {
    jint fd, nread, datalen;
    char *buf = NULL;
    char *tmp_buf = NULL;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    if (IS_NULL(bytes)) {
	JNU_ThrowNullPointerException(env, 0);
	return -1;
    }
    datalen = (*env)->GetArrayLength(env, bytes);

    if ((off < 0) || (off > datalen) ||
        (len < 0) || ((off + len) > datalen) || ((off + len) < 0)) {
        JNU_ThrowByName(env, "java/lang/IndexOutOfBoundsException", 0);
	return -1;
    }

    if (len == 0) {
	return 0;
    } else if (len > CVM_CSTACK_BUF_SIZE) {
        buf = tmp_buf = malloc(len);
        if (buf == NULL) {
            JNU_ThrowOutOfMemoryError(env, 0);
            return 0;
        }
    } else {
        CVMCstackGetBuffer(ee, buf);	/* Lock EE-buffer */
        if (buf == NULL) {
            JNU_ThrowOutOfMemoryError(env, 0);
            return 0;
        }
    }

    fd = GET_FD(fid);
    nread = JVM_Read(fd, buf, len);
    if (nread > 0) {
        (*env)->SetByteArrayRegion(env, bytes, off, nread, (jbyte *)buf);
    } else if (nread == JVM_IO_ERR) {
	JNU_ThrowIOExceptionWithLastError(env, "Read error");
    } else if (nread == JVM_IO_INTR) {
        JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
    } else { /* EOF */
	nread = -1;
    }

    if (tmp_buf != NULL)
	free(tmp_buf);	/* malloc buffer was in-use */
    else
	CVMCstackReleaseBuffer(ee); /* Unlock EE-buffer */
    return nread;
}


void
writeSingle(JNIEnv *env, jobject thisObj, jint byte, jfieldID fid) {
    char c = byte;
    jint fd = GET_FD(fid);
    jint n = JVM_Write(fd, &c, 1);
    if (n == JVM_IO_ERR) {
	JNU_ThrowIOExceptionWithLastError(env, "Write error");
    } else if (n == JVM_IO_INTR) {
        JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
    }
}


void
writeBytes(JNIEnv *env, jobject thisObj, jbyteArray bytes,
	  jint off, jint len, jfieldID fid) {

    jint fd, n, datalen;
    char *buf = NULL;
    char *tmp_buf = NULL;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    if (IS_NULL(bytes)) {
	JNU_ThrowNullPointerException(env, 0);
	return;
    }
    datalen = (*env)->GetArrayLength(env, bytes);

    if ((off < 0) || (off > datalen) ||
        (len < 0) || ((off + len) > datalen) || ((off + len) < 0)) {
        JNU_ThrowByName(env, "java/lang/IndexOutOfBoundsException", 0);
	return;
    }

    if (len == 0) {
        return;
    } else if (len > CVM_CSTACK_BUF_SIZE) {
        buf = tmp_buf = malloc(len);
        if (buf == NULL) {
            JNU_ThrowOutOfMemoryError(env, 0);
            return;
        }
    } else {
        CVMCstackGetBuffer(ee, buf);	/* Lock EE-buffer */
        if (buf == NULL) {
            JNU_ThrowOutOfMemoryError(env, 0);
            return;
        }
    }

    fd = GET_FD(fid);
    (*env)->GetByteArrayRegion(env, bytes, off, len, (jbyte *)buf);

    if (!(*env)->ExceptionOccurred(env)) {
        off = 0;
	while (len > 0) {
	    n = JVM_Write(fd, buf+off, len);
	    if (n == JVM_IO_ERR) {
	        JNU_ThrowIOExceptionWithLastError(env, "Write error");
		break;
	    } else if (n == JVM_IO_INTR) {
	        JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
		break;
	    }
	    off += n;
	    len -= n;
	}
    }
    if (tmp_buf != NULL)
	free (tmp_buf);
    else
	CVMCstackReleaseBuffer(ee);	/* Unlock EE-buffer */
}


static void
throwFileNotFoundException(JNIEnv *env, jstring path)
{
    char buf[256];
    int n;
    jobject x;
    jstring why = NULL;

    n = JVM_GetLastErrorString(buf, sizeof(buf));
    if (n > 0) {
	why = JNU_NewStringPlatform(env, buf);
        if (why == NULL) {
            /* If we get here, then we're not able to instantiate the string
               above.  An OutOfMemoryError must have been thrown.  Just return
               to the caller. */
            return;
        }
    }
    x = JNU_NewObjectByName(env,
			    "java/io/FileNotFoundException",
			    "(Ljava/lang/String;Ljava/lang/String;)V",
			    path, why);
    if (x != NULL) {
	(*env)->Throw(env, x);
	(*env)->DeleteLocalRef(env, x);
	if (why != NULL) {
	    (*env)->DeleteLocalRef(env, why);
	}
    }
}


void
fileOpen(JNIEnv *env, jobject thisObj, jstring path, jfieldID fid, int flags)
{
    WITH_PLATFORM_STRING(env, path, ps) {
	jint fd = JVM_Open(ps, flags, 0666);
	if (fd >= 0) {
	    SET_FD(fd, fid);
	} else {
	    throwFileNotFoundException(env, path);
	}
    } END_PLATFORM_STRING(env, ps);
}


/* File.deleteOnExit support
   No synchronization is done here; that is left to the Java level.
 */

struct dlEntry {
    struct dlEntry *next;
    DELETEPROC deleteProc;
    char *name;
};

static void
deleteOnExitHook(void)		/* Called by the VM on exit */
{
    struct dlEntry *e, *next;
    for (e = (struct dlEntry *) JNI_STATIC(io_util, deletionList); e; e = next) {
	next = e->next;
	e->deleteProc(e->name);
	free(e->name);
	free(e);
    }
}

void
deleteOnExit(JNIEnv *env, const char *path, DELETEPROC dp)
{
    struct dlEntry *dl = (struct dlEntry *) JNI_STATIC(io_util, deletionList);
    struct dlEntry *e = (struct dlEntry *)malloc(sizeof(struct dlEntry));
    if (e == NULL) {
	JNU_ThrowOutOfMemoryError(env, 0);
    }
    e->name = strdup(path);
    e->deleteProc = dp;

    if (dl == NULL) {
	JVM_OnExit(deleteOnExitHook);
    }	
    e->next = (struct dlEntry *) JNI_STATIC(io_util, deletionList);
    JNI_STATIC(io_util, deletionList) = e;
}
