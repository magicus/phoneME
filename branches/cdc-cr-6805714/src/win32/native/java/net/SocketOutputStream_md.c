/*
 * @(#)SocketOutputStream_md.c	1.32 06/10/10
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
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <winsock.h>
#endif



#include "jni_util.h"
#include "jvm.h"
#include "net_util.h"

#include "java_net_SocketOutputStream.h"

/* #define min(a, b)       ((a) < (b) ? (a) : (b)) */

/*
 * SocketOutputStream
 */

#include "jni_statics.h"

#define fos_fdID	JNI_STATIC_MD(java_net_SocketOutputStream, sos_fdID)

#define IO_fd_fdID	JNI_STATIC_MD(java_net_SocketOutputStream, IO_fd_fdID)

/*
 * Class:     java_net_SocketOutputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_init(JNIEnv *env, jclass cls) {

    jclass fos_cls =
	(*env)->FindClass(env, "java/io/FileOutputStream");
    if (fos_cls == NULL) {
	return;  /* exception */
    }
    fos_fdID = (*env)->GetFieldID(env, fos_cls, "fd",
				  "Ljava/io/FileDescriptor;");
    IO_fd_fdID = NET_GetFileDescriptorID(env);
}

/*
 * Class:     java_net_SocketOutputStream
 * Method:    socketWrite
 * Signature: (Ljava/io/FileDescriptor;[BII)V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_socketWrite0(JNIEnv *env, jobject this, 
					      jobject fdObj, jbyteArray data,
					      jint off, jint len) {
    char *bufP;
    /*
     * We allocate a static buffer on the stack, copy successive
     * chunks of the buffer to be written into it, then write that. It
     * is believed that this is faster that doing a malloc and copy.  
     */
    char BUF[MAX_BUFFER_LEN];
    int buflen;
    int fd;
    jint n;

    jint datalen;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
        /* Bug 4086704 - If the Socket associated with this file descriptor
         * was closed (sysCloseFD), the the file descriptor is set to -1.
         */
        if (fd == -1) {
            JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
            return;
        }

    }
    if (IS_NULL(data)) {
	JNU_ThrowNullPointerException(env, "data argument");
	return;
    }

    datalen = (*env)->GetArrayLength(env, data);

    if ((len < 0) || (off < 0) || (len + off > datalen)) {
        JNU_ThrowByName(env, "java/lang/ArrayIndexOutOfBoundsException", 0);
	return;
    }
    /*
     * Use stack allocate buffer if possible. For large sizes we allocate
     * an intermediate buffer from the heap (up to a maximum). If heap is
     * unavailable just use our stack buffer.
     */
    if (len <= MAX_BUFFER_LEN) {
	bufP = BUF;
	buflen = MAX_BUFFER_LEN;
    } else {
	buflen = min(MAX_HEAP_BUFFER_LEN, len);
	bufP = (char *)malloc((size_t)buflen);
	if (bufP == NULL) {
	    bufP = BUF;
	    buflen = MAX_BUFFER_LEN;
	}
    }
 
    while(len > 0) {
	int loff = 0;
	int chunkLen = min(buflen, len);
	int llen = chunkLen;
	int retry = 0;

	(*env)->GetByteArrayRegion(env, data, off, chunkLen, (jbyte *)bufP);	
      
        while(llen > 0) {
          int n = JVM_Send(fd, bufP + loff, llen, 0);
          if (n > 0) {
            llen -= n;
            loff += n;
            continue;
          }

          /*
           * Due to a bug in Windows Sockets (observed on NT and Windows
           * 2000) it may be necessary to retry the send. The issue is that
           * on blocking sockets send/WSASend is supposed to block if there
           * is insufficient buffer space available. If there are a large
           * number of threads blocked on write due to congestion then it's
           * possile to hit the NT/2000 bug whereby send returns WSAENOBUFS.
           * The workaround we use is to retry the send. If we have a 
           * large buffer to send (>2k) then we retry with a maximum of
           * 2k buffer. If we hit the issue with <=2k buffer then we backoff
           * for 1 second and retry again. We repeat this up to a reasonable
           * limit before bailing out and throwing an exception. In load
           * conditions we've observed that the send will succeed after 2-3
           * attempts but this depends on network buffers associated with
           * other sockets draining. 
           */
          if (WSAGetLastError() == WSAENOBUFS) {
            if (llen > MAX_BUFFER_LEN) {
              buflen = MAX_BUFFER_LEN;
              chunkLen = MAX_BUFFER_LEN;
              llen = MAX_BUFFER_LEN;
              continue;
            }
            if (retry >= 30) {
              JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
                              "No buffer space available - exhausted attempts to queue buffer");
              if (bufP != BUF) {
                free(bufP);
              }
              return;
            }
            Sleep(1000);		
            retry++;
            continue;
          }
          
          /*
           * Send failed - can be caused by close or write error.
           */
          if (WSAGetLastError() == WSAENOTSOCK) {
            JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
          } else {
            NET_ThrowCurrent(env, "socket write error");
          }
          if (bufP != BUF) {
            free(bufP);
          }
          return;
	}
	len -= chunkLen;
	off += chunkLen;
    }
    if (bufP != BUF) {
      free(bufP);
    }
}
