/*
 * @(#)SocketOutputStream_md.c	1.9 06/10/10
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

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "jni_util.h"
#include "jvm.h"
#include "net_util.h"

#include "java_net_SocketOutputStream.h"

#include "javavm/include/porting/io.h"
#include "javavm/include/porting/net.h"

#undef min
#define min(a, b)       ((a) < (b) ? (a) : (b))

/*
 * SocketOutputStream
 */

#include "jni_statics.h"

/*
static jfieldID fos_fdID;

static jfieldID IO_fd_fdID;
*/

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
    JNI_STATIC_MD(java_net_SocketOutputStream, fos_fdID) = (*env)->GetFieldID(env, fos_cls, "fd",
				  "Ljava/io/FileDescriptor;");
    JNI_STATIC_MD(java_net_SocketOutputStream, IO_fd_fdID) = NET_GetFileDescriptorID(env);
}

/*
 * Class:     java_net_SocketOutputStream
 * Method:    socketWrite
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_socketWrite(JNIEnv *env, jobject this, 
					     jbyteArray data,
					     jint off, jint len) {
    /*
     * We allocate a static buffer on the stack, copy successive
     * chunks of the buffer to be written into it, then write that. It
     * is believed that this is faster that doing a malloc and copy.  
     */
    char BUF[MAX_BUFFER_LEN];

    /* The fd field */
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_SocketOutputStream, fos_fdID));
    int fd;

    jint datalen;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, "java/net/SocketException", "Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_SocketOutputStream, IO_fd_fdID));
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

    while(len > 0) {
	int loff = 0;
	int chunkLen = min(MAX_BUFFER_LEN, len);
	int llen = chunkLen;
	(*env)->GetByteArrayRegion(env, data, off, chunkLen, (jbyte *)BUF);
      
	while(llen > 0) {
	    int n = CVMnetSend(fd, BUF + loff, llen, 0);
	    if (n == CVM_IO_ERR) {
		JNU_ThrowByName(env, "java/io/IOException", strerror(errno));
		return;
	    }
	    if (n == CVM_IO_INTR) {
		JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
		return;
	    }
	    llen -= n;
	    loff += n;
	}
	len -= chunkLen;
	off += chunkLen;
    }
}




