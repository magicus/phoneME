/*
 * @(#)SocketOutputStream_md.c	1.11 06/10/10
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
#include <sys/types.h>
#include <sys/socket.h>

#include "jni_util.h"
#include "jvm.h"
#include "net_util.h"

#include "java_net_SocketOutputStream.h"

#define min(a, b)       ((a) < (b) ? (a) : (b))

/*
 * SocketOutputStream
 */

#include "jni_statics.h"

/*
 * Class:     java_net_SocketOutputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_init(JNIEnv *env, jclass cls) {
    JNI_STATIC_MD(java_net_SocketOutputStream, IO_fd_fdID) 
    	= NET_GetFileDescriptorID(env);
}

/*
 * Class:     java_net_SocketOutputStream
 * Method:    socketWrite0
 * Signature: (Ljava/io/FileDescriptor;[BII)V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_socketWrite0(JNIEnv *env, jobject this, 
					     jobject fdObj, jbyteArray data, 
					     jint off, jint len) {
    /*
     * We allocate a static buffer on the stack, copy successive
     * chunks of the buffer to be written into it, then write that. It
     * is believed that this is faster that doing a malloc and copy.  
     */
    char BUF[MAX_BUFFER_LEN];
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_SocketOutputStream, IO_fd_fdID));
        /* Bug 4086704 - If the Socket associated with this file descriptor
         * was closed (sysCloseFD), the the file descriptor is set to -1.
         */
        if (fd == -1) {
            JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
            return;
        }

    }
   
    while(len > 0) {
	int loff = 0;
	int chunkLen = min(MAX_BUFFER_LEN, len);
	int llen = chunkLen;
	(*env)->GetByteArrayRegion(env, data, off, chunkLen, (jbyte *)BUF);
      
	while(llen > 0) {
	    int n = NET_Send(fd, BUF + loff, llen, 0);
	    if (n > 0) {
		llen -= n;
		loff += n;
		continue;
	    }
	    if (n == JVM_IO_INTR) {
		JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
	    } else {
		if (errno == ECONNRESET) {
                    JNU_ThrowByName(env, "sun/net/ConnectionResetException",
                                    "Connection reset");
		} else {
		    NET_ThrowByNameWithLastError(env, "java/net/SocketException", 
                                                 "Write failed");
		}
	    }
	    return;
	}
	len -= chunkLen;
	off += chunkLen;
    }
}




