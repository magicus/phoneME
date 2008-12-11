/*
 * @(#)SocketInputStream_md.c	1.42 06/10/10
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#ifndef WINCE
	#include <sys/types.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <winsock.h>
#endif

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_SocketInputStream.h"

#include "net_util.h"
#include "jni_util.h"

/************************************************************************
 * SocketInputStream
 */

#include "jni_statics.h"

#define IO_fd_fdID	JNI_STATIC_MD(java_net_SocketInputStream, IO_fd_fdID)

#define fis_fdID	JNI_STATIC_MD(java_net_SocketInputStream, fis_fdID)
#define sis_implID	JNI_STATIC_MD(java_net_SocketInputStream, sis_implID)

#define psi_timeoutID	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_timeoutID)

/*
 * Class:     java_net_SocketInputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_java_net_SocketInputStream_init(JNIEnv *env, jclass cls) {

    jclass fis_cls =
	(*env)->FindClass(env, "java/io/FileInputStream");
    if (fis_cls == NULL) {
	return;  /* exception */
    }
    fis_fdID = (*env)->GetFieldID(env, fis_cls, "fd", 
				  "Ljava/io/FileDescriptor;");
    IO_fd_fdID = NET_GetFileDescriptorID(env);
    sis_implID = (*env)->GetFieldID(env, cls, "impl", 
				    "Ljava/net/PlainSocketImpl;");
}

/*
 * Class:     java_net_SocketInputStream
 * Method:    socketRead
 * Signature: (Ljava/io/FileDescriptor;[BIII)I
 */
JNIEXPORT jint JNICALL
Java_java_net_SocketInputStream_socketRead0(JNIEnv *env, jobject this, 
					    jobject fdObj, jbyteArray data, 
					    jint off, jint len, jint timeout) 
{
    char BUF[MAX_BUFFER_LEN];
    char *bufP;

    /* The fd field */
    //jobject fdObj = (*env)->GetObjectField(env, this, fis_fdID);
    jint fd, nread;

    /* The impl field */
    jobject impl = (*env)->GetObjectField(env, this, sis_implID);

    jint datalen;

    if (IS_NULL(fdObj)) {
	/* should't this be a NullPointerException? -br */
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			"null fd object");
	return -1;
    } else {
        fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
        /* Bug 4086704 - If the Socket associated with this file descriptor
         * was closed (sysCloseFD), the the file descriptor is set to -1.
         */
        if (fd == -1) {
            JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
            return -1;
        }     
    }
    if (IS_NULL(data)) {
	JNU_ThrowNullPointerException(env, "data argument");
        return -1;
    }
    if (IS_NULL(impl)) {
	JNU_ThrowNullPointerException(env, "socket impl");
	return -1;
    } else {
	timeout = (*env)->GetIntField(env, impl, psi_timeoutID);
    }

    datalen = (*env)->GetArrayLength(env, data);

    if (len < 0 || len + off > datalen) {
        JNU_ThrowByName(env, JNU_JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }

    if (len == 0) {
	return 0;
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

    if (timeout) {
	if (timeout <= 5000 || !isRcvTimeoutSupported) {
	    int ret = NET_Timeout (fd, timeout);
	    
	    if (ret <= 0) {
		if (ret == 0) {
 		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				    "Read timed out");
		} else if (ret == JVM_IO_ERR) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "socket closed");
		} else if (ret == JVM_IO_INTR) {
		    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
				    "Operation interrupted");
		}
		if (bufP != BUF) {
		    free(bufP);
		}
		return -1;
	    }
	}
    }

    nread = CVMnetRecv(fd, bufP, len, 0);
    if (nread > 0) {
	(*env)->SetByteArrayRegion(env, data, off, nread, (jbyte *)bufP);
    } else {
	if (nread < 0) {
	    /*
	     * Recv failed.
	     */
	    switch (WSAGetLastError()) {
	        case WSAEINTR:
		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			"socket closed");
		    break;
	
		case WSAECONNRESET:
		case WSAESHUTDOWN:
		    /*
		     * Connection has been reset - Windows sometimes reports
		     * the reset as a shutdown error.
		     */
		    JNU_ThrowByName(env, "sun/net/ConnectionResetException",
			"");
		    break;

	        case WSAETIMEDOUT :
                    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
                                   "Read timed out");
		    break;

		default:
		    NET_ThrowCurrent(env, "recv failed");
	    }
	}
    }
    if (bufP != BUF) {
	free(bufP);
    }
    return nread;
}
