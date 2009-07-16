/*
 * @(#)SocketInputStream_md.cpp	1.7 06/10/10
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_SocketInputStream.h"

#include "RLockingSocket.hpp"

RTimer &SYMBIANthreadGetTimer(CVMThreadID *self);

/************************************************************************
 * SocketInputStream
 */

extern "C" {
#include "jni_statics.h"
}

#define IO_fd_fdID \
    JNI_STATIC_MD(java_net_SocketInputStream, IO_fd_fdID)

/*
 * Class:     java_net_SocketInputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_java_net_SocketInputStream_init(JNIEnv *env, jclass cls) {
    IO_fd_fdID = NET_GetFileDescriptorID(env);
}

/*
 * Class:     java_net_SocketInputStream
 * Method:    socketRead0
 * Signature: (Ljava/io/FileDescriptor;[BIII)I
 */
JNIEXPORT jint JNICALL
Java_java_net_SocketInputStream_socketRead0(JNIEnv *env, jobject thisObj, 
                                            jobject fdObj, jbyteArray data, 
                                            jint off, jint len, jint timeout)
{
    char BUF[MAX_BUFFER_LEN];
    char *bufP;
    jint fd, nread;

    if (IS_NULL(fdObj)) {
	/* should't this be a NullPointerException? -br */
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			"Socket closed");
	return -1;
    } else {
        fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_SocketInputStream, IO_fd_fdID));
        /* Bug 4086704 - If the Socket associated with this file descriptor
         * was closed (sysCloseFD), the the file descriptor is set to -1.
         */
        if (fd == -1) {
            JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
            return -1;
        }
    }

    /* If requested amount to be read is > MAX_BUFFER_LEN then
     * we allocate a buffer from the heap (up to the limit
     * specified by MAX_HEAP_BUFFER_LEN). If memory is exhausted
     * we always use the stack buffer.
     */
    if (len <= MAX_BUFFER_LEN) {
        bufP = BUF;
    } else {
        if (len > MAX_HEAP_BUFFER_LEN) {
            len = MAX_HEAP_BUFFER_LEN;
        }
        bufP = (char *)malloc((size_t)len);
        if (bufP == NULL) {
            /* allocation failed so use stack buffer */
            bufP = BUF;
            len = MAX_BUFFER_LEN;
        }
    }

    RLockingSocket *s = (RLockingSocket *)fd;
    TRequestStatus rs = 1;
    TPtr8 des((TUint8 *)bufP, len);
    TSockXfrLength xlen;
    if (timeout <= 0) {
	s->rd_lock().Wait();
	do {
	    s->RecvOneOrMore(des, 0, rs, xlen);
	    User::WaitForRequest(rs);
#if 1
if (rs == KErrCancel) {
fprintf(stderr, "Recv cancelled, retrying...\n");
}
#endif
	} while (rs == KErrCancel);
	s->rd_lock().Signal();
    } else {
	s->rd_lock().Wait();

	CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
	TInt millis = timeout;
        TTime time;
        time.HomeTime();
        time += TTimeIntervalSeconds(millis / 1000);
        time += TTimeIntervalMicroSeconds32((millis % 1000) * 1000);
	RTimer &waitTimer = SYMBIANthreadGetTimer(CVMexecEnv2threadID(ee));
	TRequestStatus waitStatus = 2;
        waitTimer.At(waitStatus, time);

	s->RecvOneOrMore(des, 0, rs, xlen);

	User::WaitForRequest(rs, waitStatus);

	s->rd_lock().Signal();

	if (rs == KRequestPending) {
	    if (waitStatus != KRequestPending) {
		s->CancelRecv();
		waitTimer.Cancel();
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"connect timed out");
		return 0;
	    } else {
		/* Should not happen */
		s->CancelRecv();
		assert(0);
	    }
	}
	waitTimer.Cancel();
    }
    nread = des.Length();
    assert(xlen() == nread);

    if (rs != KErrNone) {
	JNU_ThrowByName(env, 
	    JNU_JAVANETPKG "SocketException", "Read failed");
    } else {
        (*env)->SetByteArrayRegion(env, data, off, nread, (jbyte *)bufP);
    }

    if (bufP != BUF) {
	free(bufP);
    }
    return nread;
}
