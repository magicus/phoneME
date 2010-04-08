/*
 * @(#)PlainDatagramSocketImpl_md.c	1.16 06/10/10
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
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_SocketOptions.h"
#include "java_net_PlainDatagramSocketImpl.h"

#include "jni_statics.h"

/************************************************************************
 * PlainDatagramSocketImpl
 */

/*
static jfieldID IO_fd_fdID;

static jfieldID pdsi_fdID;
static jfieldID pdsi_timeoutID;
static jfieldID pdsi_localPortID;

static jclass ia_clazz;
static jmethodID ia_ctor;
*/

/*
 * the maximum buffer size. Used for setting
 * SendBufferSize and ReceiveBufferSize.
 */
static const int max_buffer_size = 64 * 1024;

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_init(JNIEnv *env, jclass cls) {

    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID) = (*env)->GetFieldID(env, cls, "fd",
				   "Ljava/io/FileDescriptor;");
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_timeoutID) = (*env)->GetFieldID(env, cls, "timeout", "I");
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_localPortID) = (*env)->GetFieldID(env, cls, "localPort", "I");
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID) = NET_GetFileDescriptorID(env);

    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_clazz) = (*env)->FindClass(env, "java/net/InetAddress");
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_clazz) = (*env)->NewGlobalRef(env, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_clazz));
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_ctor) = (*env)->GetMethodID(env, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_clazz), "<init>", "()V");
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    bind
 * Signature: (ILjava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_bind(JNIEnv *env, jobject this,
					   jint port, jobject laddr) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    int fd;

    struct sockaddr_in lcladdr;
    int lcladdrlen= sizeof(lcladdr);
    int address;

    if (IS_NULL(laddr)) { /* shouldn't happen, but be safe */
	address = 0;
    } else {
	address = (*env)->GetIntField(env, laddr, JNI_STATIC(java_net_InetAddress, ia_addressID));
    }
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    /* bind - pick a port number for local addr*/
    memset((char *) &lcladdr, 0, sizeof(lcladdr));
    lcladdr.sin_family      = AF_INET;
    lcladdr.sin_port        = htons((short)port);
    lcladdr.sin_addr.s_addr = (laddr ? htonl(address) : INADDR_ANY);
    lcladdr.sin_len = (unsigned char) sizeof(lcladdr);

    if (JVM_Bind(fd, (struct sockaddr *)&lcladdr, sizeof(lcladdr)) == -1) {
	if (errno == EADDRINUSE || errno == EADDRNOTAVAIL ||
	    errno == EPERM || errno == EACCES) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "BindException",
			    strerror(errno));
	} else {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	}
	return;
    }

    /* Find what port system; Interface should in the bind call itself */
    if (JVM_GetSockName(fd, (struct sockaddr *)&lcladdr, &lcladdrlen) == -1) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
	return;
    }
    (*env)->SetIntField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_localPortID),
			ntohs(lcladdr.sin_port));
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    send
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_send(JNIEnv *env, jobject this,
					   jobject packet) {

    char BUF[MAX_BUFFER_LEN];
    char *fullPacket = NULL;
    int mallocedPacket = JNI_FALSE;
    /* The object's field */
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));

    jbyteArray packetBuffer;
    jobject packetAddress;
    jint packetBufferOffset, packetBufferLen, packetPort;

    /* The packetAddress address and family */
    jint address, family;
    /* The fdObj'fd */
    jint fd;

    struct sockaddr_in rmtaddr;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }
    fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));

    if (IS_NULL(packet)) {
	JNU_ThrowNullPointerException(env, "packet");
	return;
    }

    packetBuffer = (*env)->GetObjectField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_bufID));
    packetAddress = (*env)->GetObjectField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_addressID));
    if (IS_NULL(packetBuffer) || IS_NULL(packetAddress)) {
	JNU_ThrowNullPointerException(env, "null buffer || null address");
	return;
    }

    packetPort = (*env)->GetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_portID));
    packetBufferOffset = (*env)->GetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_offsetID));
    packetBufferLen = (*env)->GetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_lengthID));

    address = (*env)->GetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID));
    family = (*env)->GetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_familyID));

    rmtaddr.sin_port = htons((short)packetPort);
    rmtaddr.sin_addr.s_addr = htonl(address);
    rmtaddr.sin_len = (unsigned char) sizeof(rmtaddr);
    rmtaddr.sin_family = family;

    if (packetBufferLen > MAX_BUFFER_LEN) {
	/* When JNI-ifying the JDK's IO routines, we turned
	 * read's and write's of byte arrays of size greater
	 * than 2048 bytes into several operations of size 2048.
	 * This saves a malloc()/memcpy()/free() for big
	 * buffers.  This is OK for file IO and TCP, but that
	 * strategy violates the semantics of a datagram protocol.
	 * (one big send) != (several smaller sends).  So here
	 * we *must* alloc the buffer.  Note it needn't be bigger
	 * than 65,536 (0xFFFF) the max size of an IP packet.
	 * Anything bigger should be truncated anyway.
	 *
	 * We may want to use a smarter allocation scheme at some
	 * point.
	 */
	if (packetBufferLen > MAX_PACKET_LEN) {
	    packetBufferLen = MAX_PACKET_LEN;
	}
	fullPacket = (char *)malloc(packetBufferLen);

	if (!fullPacket) {
	    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
	    return;
	} else {
	    mallocedPacket = JNI_TRUE;
	}
    } else {
	fullPacket = &(BUF[0]);
    }

    (*env)->GetByteArrayRegion(env, packetBuffer, packetBufferOffset, packetBufferLen,
			       (jbyte *)fullPacket);

    switch (JVM_SendTo(fd, fullPacket, packetBufferLen, 0,
			       (struct sockaddr *)&rmtaddr,
		       sizeof(rmtaddr))) {
        case JVM_IO_ERR:
	    JNU_ThrowIOException(env, strerror(errno));
	    break;
        case JVM_IO_INTR:
	    JNU_ThrowByName(env, "java/io/InterruptedIOException",
			    "operation interrupted");
    }
    if (mallocedPacket) {
	free(fullPacket);
    }
    return;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    peek
 * Signature: (Ljava/net/InetAddress;)I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_peek(JNIEnv *env, jobject this,
					   jobject addressObj) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    jint timeout = (*env)->GetIntField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_timeoutID));
    jint fd;

    /* The address and family fields of addressObj */
    jint address, family;

    ssize_t n;
    struct sockaddr_in remote_addr;
    int remote_addrsize = sizeof (remote_addr);
    char buf[1];

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    if (IS_NULL(addressObj)) {
	JNU_ThrowNullPointerException(env, "Null address in peek()");
    } else {
	address = (*env)->GetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
	family = (*env)->GetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_familyID));
    }
    if (timeout) {
	int ret = JVM_Timeout(fd, timeout);
        if (ret == 0) {
            JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "Peek timed out");
	    return ret;
	} else if (ret == JVM_IO_ERR) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    (errno == EBADF)?"Socket closed":strerror(errno));
	    return ret;
	} else if (ret == JVM_IO_INTR) {
	    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "operation interrupted");
	    return ret;	/* WARNING: SHOULD WE REALLY RETURN -2??? */
	}
    }
    n = JVM_RecvFrom(fd, buf, 1, MSG_PEEK,
		     (struct sockaddr *)&remote_addr, &remote_addrsize);

    if (n == JVM_IO_ERR) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			(errno == EBADF) ? "Socket closed" : strerror(errno));
        return 0;
    } else if (n == JVM_IO_INTR) {
	JNU_ThrowByName(env, "java/io/InterruptedIOException", 0);
        return 0;
    }
    (*env)->SetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_addressID),
			ntohl(remote_addr.sin_addr.s_addr));
    (*env)->SetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_familyID),
			remote_addr.sin_family);

    /* return port */
    return ntohs(remote_addr.sin_port);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    receive
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_receive(JNIEnv *env, jobject this,
					      jobject packet) {

    char BUF[MAX_BUFFER_LEN];
    char *fullPacket = NULL;
    int mallocedPacket = JNI_FALSE;
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    jint timeout = (*env)->GetIntField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_timeoutID));

    jbyteArray packetBuffer;
    jint packetBufferOffset, packetBufferLen;

    int fd;

    int n;
    struct sockaddr_in remote_addr;
    int remote_addrsize = sizeof (remote_addr);

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }

    fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));

    if (IS_NULL(packet)) {
	JNU_ThrowNullPointerException(env, "packet");
	return;
    }

    packetBuffer = (*env)->GetObjectField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_bufID));
    if (IS_NULL(packetBuffer)) {
	JNU_ThrowNullPointerException(env, "packet buffer");
        return;
    }
    packetBufferOffset = (*env)->GetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_offsetID));
    packetBufferLen = (*env)->GetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_lengthID));
    if (timeout) {
	int ret = JVM_Timeout(fd, timeout);
        if (ret == 0) {
            JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "Receive timed out");
	    return;
	} else if (ret == JVM_IO_ERR) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    "Socket closed");
	    return;
	} else if (ret == JVM_IO_INTR) {
	    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "operation interrupted");
	    return;
	}
    }

    if (packetBufferLen > MAX_BUFFER_LEN) {

	/* When JNI-ifying the JDK's IO routines, we turned
	 * read's and write's of byte arrays of size greater
	 * than 2048 bytes into several operations of size 2048.
	 * This saves a malloc()/memcpy()/free() for big
	 * buffers.  This is OK for file IO and TCP, but that
	 * strategy violates the semantics of a datagram protocol.
	 * (one big send) != (several smaller sends).  So here
	 * we *must* alloc the buffer.  Note it needn't be bigger
	 * than 65,536 (0xFFFF) the max size of an IP packet.
	 * anything bigger is truncated anyway.
	 *
	 * We may want to use a smarter allocation scheme at some
	 * point.
	 */
	if (packetBufferLen > MAX_PACKET_LEN) {
	    packetBufferLen = MAX_PACKET_LEN;
	}
	fullPacket = (char *)malloc(packetBufferLen);

	if (!fullPacket) {
	    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
	    return;
	} else {
	    mallocedPacket = JNI_TRUE;
	}
    } else {
	fullPacket = &(BUF[0]);
    }
    n = JVM_RecvFrom(fd, fullPacket, packetBufferLen, 0,
		     (struct sockaddr *)&remote_addr, &remote_addrsize);
    /* truncate the data if the packet's length is too small */
    if (n > packetBufferLen) {
	n = packetBufferLen;
    }
    if (n == JVM_IO_ERR) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"socket closed");
	(*env)->SetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_offsetID), 0);
	(*env)->SetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_lengthID), 0);
    } else if (n == JVM_IO_INTR) {
	JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			"operation interrupted");
	(*env)->SetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_offsetID), 0);
	(*env)->SetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_lengthID), 0);
    } else {
        int newAddress;
        int newFamily;
        jobject packetAddress;

        /* source address */
        newAddress = ntohl(remote_addr.sin_addr.s_addr);
        newFamily = remote_addr.sin_family;

        /*
         * Check if there is an InetAddress already associated with this
         * packet. If so we check if it is the same source address. We
         * can't update any existing InetAddress because it is immutable
         */
        packetAddress = (*env)->GetObjectField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_addressID));
        if (packetAddress != NULL) {
            int currAddress;
            int currFamily;
            currAddress = (*env)->GetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID));
            currFamily = (*env)->GetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_familyID));

            if ((currAddress != newAddress) || (currFamily != newFamily)) {
                /* force a new InetAddress to be created */
                packetAddress = NULL;
            }
        }
        if (packetAddress == NULL) {
            packetAddress = (*env)->NewObject(env, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_clazz), JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_ctor));
            (*env)->SetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID), newAddress);
            (*env)->SetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_familyID), newFamily);

            /* stuff the new Inetaddress in the packet */
            (*env)->SetObjectField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_addressID), packetAddress);
        }

        /* populate the packet */
        (*env)->SetByteArrayRegion(env, packetBuffer, packetBufferOffset, n,
                                   (jbyte *)fullPacket);
        (*env)->SetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_portID),
                            ntohs(remote_addr.sin_port));
        (*env)->SetIntField(env, packet, JNI_STATIC(java_net_DatagramPacket, dp_lengthID), n);
    }

    if (mallocedPacket) {
	free(fullPacket);
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    datagramSocketCreate
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketCreate(JNIEnv *env,
							   jobject this) {
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd =  JVM_Socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (fd == JVM_IO_ERR) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
	return;
    }
    (*env)->SetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID), fd);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    datagramSocketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketClose(JNIEnv *env,
							  jobject this) {
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
        fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    JVM_SocketClose(fd);
    (*env)->SetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID), -1);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    socketSetOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_socketSetOption(JNIEnv *env,
						      jobject this,
						      jint opt,
						      jobject value) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    if (IS_NULL(value)) {
	JNU_ThrowNullPointerException(env, "value argument");
	return;
    }
    if (fd < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"socket closed");
    } else if (opt == java_net_SocketOptions_IP_MULTICAST_IF) {
        int address = (*env)->GetIntField(env, value, JNI_STATIC(java_net_InetAddress, ia_addressID));
	struct in_addr in;
	in.s_addr = htonl(address);
	errno = 0;
	if (JVM_SetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		       (const char*)&in, sizeof(in)) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	}
    } else if (opt == java_net_SocketOptions_SO_REUSEADDR) {
	jclass iCls = (*env)->FindClass(env, "java/lang/Integer");
	jfieldID i_valueID = (*env)->GetFieldID(env, iCls, "value", "I");

	int tmp = (*env)->GetIntField(env, value, i_valueID);
	int arg = tmp ? -1: 0;
	if (JVM_SetSockOpt(fd, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&arg, sizeof(arg)) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	}
    } else if (opt == java_net_SocketOptions_SO_SNDBUF) {
	int arg;
	jclass iCls = (*env)->FindClass(env, "java/lang/Integer");
	jfieldID i_valueID = (*env)->GetFieldID(env, iCls, "value", "I");
	arg = (*env)->GetIntField(env, value, i_valueID);
	if (arg > max_buffer_size) {
	    arg = max_buffer_size;
	}
	/* Don't throw an exception even if JVM_SetSockOpt() returns -1
	 * since the buffer size is just a hint.
	 */
	JVM_SetSockOpt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&arg, sizeof(arg));
    } else if (opt == java_net_SocketOptions_SO_RCVBUF) {
	int arg;
	jclass iCls = (*env)->FindClass(env, "java/lang/Integer");
	jfieldID i_valueID = (*env)->GetFieldID(env, iCls, "value", "I");
	arg = (*env)->GetIntField(env, value, i_valueID);
	if (arg > max_buffer_size) {
	    arg = max_buffer_size;
	}
	/* Don't throw an exception even if JVM_SetSockOpt() returns -1
	 * since the buffer size is just a hint.
	 */
	JVM_SetSockOpt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&arg, sizeof(arg));
    } else {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"invalid DatagramSocket option");
    }
}

/*
 * Returns relevant info as a jint.
 *
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_socketGetOption(JNIEnv *env, jobject this,
						      jint opt) {
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }

    if (fd < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"socket closed");
	return -1;
    } else if (opt == java_net_SocketOptions_SO_BINDADDR) {
	/* find out local IP address, return as 32 bit value */
	struct sockaddr_in him;
	int len = sizeof(struct sockaddr_in);
	memset(&him, 0, len);
	if (JVM_GetSockName(fd, (struct sockaddr *)&him, &len) == -1) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return -1;
	}
	return ntohl(him.sin_addr.s_addr);
	/* == INADDR_ANY) ? -1: him.sin_addr.s_addr;*/
    } else if (opt == java_net_SocketOptions_IP_MULTICAST_IF) {
        struct in_addr in;
	int len = sizeof(struct in_addr);
	if (JVM_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&in, &len) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return -1;
	}
	return ntohl(in.s_addr);
    } else if (opt == java_net_SocketOptions_SO_SNDBUF) {
	int result = -1;
	int len = sizeof(result);
	if (JVM_GetSockOpt(fd, SOL_SOCKET, SO_SNDBUF,
		       (char *)&result, &len) < 0) {
	    NET_ThrowCurrent(env, "JVM_GetSockOpt() SO_SNDBUF");
	    return -1;
	}
	return result;
    } else if (opt == java_net_SocketOptions_SO_RCVBUF) {
	int result = -1;
	int len = sizeof(result);
	if (JVM_GetSockOpt(fd, SOL_SOCKET, SO_RCVBUF,
		       (char *)&result, &len) < 0) {
	    NET_ThrowCurrent(env, "JVM_GetSockOpt() SO_RCVBUF");
	    return -1;
	}
	return result;
    } else {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"invalid DatagramSocket option");
	return -1;
    }
}

/*
 * Multicast-related calls
 */

JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTTL(JNIEnv *env, jobject this,
					     jbyte ttl) {
    Java_java_net_PlainDatagramSocketImpl_setTimeToLive(env, this, (jint)ttl);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    setTTL
 * Signature: (B)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTimeToLive(JNIEnv *env, jobject this,
						    jint ttl) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    int fd;
    /* it is important to cast this to a char, otherwise setsockopt gets confused */
    char ittl = (char)ttl;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    /* setsockopt to be correct ttl */
    if (JVM_SetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ittl,
		   sizeof(ittl)) < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    getTTL
 * Signature: ()B
 */
JNIEXPORT jbyte JNICALL
Java_java_net_PlainDatagramSocketImpl_getTTL(JNIEnv *env, jobject this) {
    return (jbyte)Java_java_net_PlainDatagramSocketImpl_getTimeToLive(env, this);
}


/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    getTTL
 * Signature: ()B
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_getTimeToLive(JNIEnv *env, jobject this) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    jint fd = -1;

    u_char ttl = 0;
    int len = sizeof(ttl);
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    /* getsockopt of ttl */
    if (JVM_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
		   (char*)&ttl, &len) < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
	return -1;
    }
    return (jint)ttl;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    join
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_join(JNIEnv *env, jobject this,
					   jobject address) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    jint fd, addressAddress;

    struct ip_mreq mname;

    struct in_addr in;
    int len = sizeof(struct in_addr);

  if (IS_NULL(fdObj)) {
      JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		      "Socket closed");
      return;
  } else {
      fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
  }
  if (IS_NULL(address)) {
      JNU_ThrowNullPointerException(env, "address");
      return;
  } else {
      addressAddress = (*env)->GetIntField(env, address, JNI_STATIC(java_net_InetAddress, ia_addressID));
  }

  /*
   * Set the multicast group address in the ip_mreq field
   * eventually this check should be done by the security manager
   */
  mname.imr_multiaddr.s_addr = htonl(addressAddress);
  if (!IN_MULTICAST(addressAddress)) {
      JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "not in multicast");
      return;
  }
  /* Set the interface */
  if (JVM_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&in, &len) < 0) {
      JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", strerror(errno));
      return;
  }
  mname.imr_interface.s_addr = in.s_addr;

  /* Join the multicast group */
  if (JVM_SetSockOpt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mname,
	   sizeof (mname)) < 0) {
      JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException","error setting options");
  }
  return;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    leave
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_leave(JNIEnv *env, jobject this,
					    jobject address) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID));
    jint fd, addressAddress;

    struct ip_mreq mname;

    struct in_addr in;
    int len = sizeof(struct in_addr);


    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID));
    }
    if (IS_NULL(address)) {
	JNU_ThrowNullPointerException(env, "address argument");
	return;
    } else {
	addressAddress = (*env)->GetIntField(env, address, JNI_STATIC(java_net_InetAddress, ia_addressID));
    }
    /*
     * Set the multicast group address in the ip_mreq field
     * eventually this check should be done by the security manager
     */
    mname.imr_multiaddr.s_addr = htonl(addressAddress);
    if (!IN_MULTICAST(addressAddress)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
    }
    /* Set the interface */
    if (JVM_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&in, &len) < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
	return;
    }
    mname.imr_interface.s_addr = in.s_addr;

    /* Join the multicast group */
    if (JVM_SetSockOpt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,(char *) &mname,
	   sizeof (mname)) < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
    }
    return;
}


