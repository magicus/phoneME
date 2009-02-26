/*
 * @(#)PlainDatagramSocketImpl_md.cpp	1.14 06/10/10
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

#include <e32def.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_SocketOptions.h"
#include "java_net_PlainDatagramSocketImpl.h"
#include "java_net_NetworkInterface.h"
#include "java_net_InetAddress.h"
#include "java_net_Inet4Address.h"
#include "java_net_Inet6Address.h"

/************************************************************************
 * PlainDatagramSocketImpl
 */

#include "jni_statics.h"

#define ia_ctor	\
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_ctor) 
#define ia_clazz \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, ia_clazz)
#define pdsi_fdID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_fdID)
#define IO_fd_fdID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, IO_fd_fdID)
#define pdsi_timeoutID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_timeoutID)
#define pdsi_trafficClassID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_trafficClassID) 
#define pdsi_localPortID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_localPortID) 
#define pdsi_connected \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_connected)
#define pdsi_connectedAddress \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_connectedAddress)
#define pdsi_connectedPort \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_connectedPort)
#define pdsi_multicastInterfaceID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_multicastInterfaceID)
#define pdsi_loopbackID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_loopbackID)
#define pdsi_ttlID \
    JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, pdsi_ttlID)
#define dp_bufID \
    JNI_STATIC(java_net_DatagramPacket, dp_bufID)
#define dp_offsetID \
    JNI_STATIC(java_net_DatagramPacket, dp_offsetID)
#define dp_addressID \
    JNI_STATIC(java_net_DatagramPacket, dp_addressID)
#define dp_bufLengthID \
    JNI_STATIC(java_net_DatagramPacket, dp_bufLengthID)
#define dp_lengthID \
    JNI_STATIC(java_net_DatagramPacket, dp_lengthID)
#define dp_portID \
    JNI_STATIC(java_net_DatagramPacket, dp_portID)
#define ia_addressID \
    JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia_familyID \
    JNI_STATIC(java_net_InetAddress, ia_familyID)
#define ia_class \
    JNI_STATIC(java_net_InetAddress, ia_class)
#define ni_class \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_class)
#define ni_indexID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_indexID)
#define ni_addrsID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_addrsID)
#define ni_ctrID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ctrID)

extern RSocketServ sserv;
RTimer &SYMBIANthreadGetTimer(CVMThreadID *self);

#include "RLockingSocket.hpp"

class RPeekSocket : public RLockingSocket {
public:
    RPeekSocket() : peekDesc(NULL, 0) {
	peekBuf = NULL;
	peekData = 0;
    }

    void RecvFrom(TDes8 &aDesc, TSockAddr &anAddr,
	TRequestStatus &rs, RTimer &waitTimer, TInt timeout)
    {
	if (timeout == 0) {
	    RSocket::RecvFrom(aDesc, anAddr, 0, rs);
	    User::WaitForRequest(rs);
	} else {
	    TInt millis = timeout;
	    TTime time;
	    time.HomeTime();
	    time += TTimeIntervalSeconds(millis / 1000);
	    time += TTimeIntervalMicroSeconds32((millis % 1000) * 1000);
	    TRequestStatus waitStatus = 2;
	    waitTimer.At(waitStatus, time);

	    RSocket::RecvFrom(aDesc, anAddr, 0, rs);

	    User::WaitForRequest(rs, waitStatus);

	    if (rs == KRequestPending) {
		if (waitStatus != KRequestPending) {
		    CancelRecv();
		    waitTimer.Cancel();
		    rs = KErrTimedOut;
		    return;
		} else {
		    /* Should not happen */
		    CancelRecv();
		    assert(0);
		}
	    }
	    waitTimer.Cancel();
	}
    }

    void RecvFrom(TDes8 &aDesc, TSockAddr &anAddr, TUint flags,
	TRequestStatus &aStatus, RTimer &waitTimer, TInt timeout)
    {
	if (peekData) {
peek:
	    anAddr = peekAddr;
	    aDesc.Copy(peekDesc.Ptr(),
		Min(aDesc.MaxLength(), peekDesc.Length()));
	    if ((flags & KSockReadPeek) == 0) {
		peekData = 0;
	    }
	    aStatus = KErrNone;
	    return;
	}
	/*
	   Work around Symbian bug where KSockReadPeek causes a
	   stack overflow (infinite recursion) on the emulator.
	 */
	if ((flags & KSockReadPeek) != 0) {
	    if (peekBuf == NULL) {
		TPckgBuf<TUint> size;
		TInt err = GetOpt(KSORecvBuf, KSOLSocket, size);
		TUint sz = size();
		if (sz == KSocketBufSizeUndefined) {
		    sz = KSocketDefaultBufferSize;
		}
		peekBuf = (char *)CVMmalloc(sz);
		peekDesc.Set((TUint8 *)peekBuf, 0, sz);
	    }
	    TRequestStatus rs;
	    RecvFrom(peekDesc, peekAddr, rs, waitTimer, timeout);
	    if (rs == KErrNone) {
		peekData = 1;
		goto peek;
	    }
	    aStatus = rs;
	} else {
	    RecvFrom(aDesc, anAddr, aStatus, waitTimer, timeout);
	}
    }

    ~RPeekSocket() {
	if (peekBuf != NULL) {
	    CVMfree(peekBuf);
	}
    }

private:

    char *peekBuf;
    int peekData;
    TPtr8 peekDesc;
    TSockAddr peekAddr;
};

/*
 * Returns a java.lang.Integer based on 'i'
 */
static jobject createInteger(JNIEnv *env, int i) {
    static jclass i_class;
    static jmethodID i_ctrID;

    if (i_class == NULL) {
 	jclass c = (*env)->FindClass(env, "java/lang/Integer");
	CHECK_NULL_RETURN(c, NULL);
        i_ctrID = (*env)->GetMethodID(env, c, "<init>", "(I)V");
	CHECK_NULL_RETURN(i_ctrID, NULL);
	i_class = (*env)->NewGlobalRef(env, c);
	CHECK_NULL_RETURN(i_class, NULL);
    }

    return ( (*env)->NewObject(env, i_class, i_ctrID, i) );
}

/*
 * Returns a java.lang.Boolean based on 'b'
 */
static jobject createBoolean(JNIEnv *env, int b) {
    static jclass b_class;
    static jmethodID b_ctrID;

    if (b_class == NULL) {
        jclass c = (*env)->FindClass(env, "java/lang/Boolean");
	CHECK_NULL_RETURN(c, NULL);
        b_ctrID = (*env)->GetMethodID(env, c, "<init>", "(Z)V");
	CHECK_NULL_RETURN(b_ctrID, NULL);
        b_class = (*env)->NewGlobalRef(env, c);
	CHECK_NULL_RETURN(b_class, NULL);
    }

    return( (*env)->NewObject(env, b_class, b_ctrID, (jboolean)(b!=0)) );
}


/*
 * Returns the fd for a PlainDatagramSocketImpl or -1
 * if closed.
 */
static int getFD(JNIEnv *env, jobject thisObj) {
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    if (fdObj == NULL) {
        return -1;
    }
    return (*env)->GetIntField(env, fdObj, IO_fd_fdID);
}

/* Do we need this any longer???
 * the maximum buffer size. Used for setting
 * SendBufferSize and ReceiveBufferSize.
 */
static const int max_buffer_size = 64 * 1024;

#define InetAddress	"Ljava/net/InetAddress;"
#define DatagramPacket	"Ljava/net/DatagramPacket;"

static JNINativeMethod methods[] = {
    {"peek",		"(" InetAddress ")I",
	(void *)&Java_java_net_PlainDatagramSocketImpl_peek},
    {"peekData",	"(" DatagramPacket ")I",
	(void *)&Java_java_net_PlainDatagramSocketImpl_peekData}
};

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_init(JNIEnv *env, jclass cls) {
    pdsi_fdID = (*env)->GetFieldID(env, cls, "fd", "Ljava/io/FileDescriptor;");
    CHECK_NULL(pdsi_fdID);
    pdsi_timeoutID = (*env)->GetFieldID(env, cls, "timeout", "I");
    CHECK_NULL(pdsi_timeoutID);
    pdsi_trafficClassID = (*env)->GetFieldID(env, cls, "trafficClass", "I");
    CHECK_NULL(pdsi_trafficClassID);
    pdsi_localPortID = (*env)->GetFieldID(env, cls, "localPort", "I");
    CHECK_NULL(pdsi_localPortID);
    pdsi_connected = (*env)->GetFieldID(env, cls, "connected", "Z");
    CHECK_NULL(pdsi_connected);
    pdsi_connectedAddress
        = (*env)->GetFieldID(env, cls, "connectedAddress", "Ljava/net/InetAddress;");
    CHECK_NULL(pdsi_connectedAddress);
    pdsi_connectedPort = (*env)->GetFieldID(env, cls, "connectedPort", "I");
    CHECK_NULL(pdsi_connectedPort);

    IO_fd_fdID = NET_GetFileDescriptorID(env);
    CHECK_NULL(IO_fd_fdID);

    Java_java_net_InetAddress_init(env, 0);
    Java_java_net_Inet4Address_init(env, 0);
    Java_java_net_Inet6Address_init(env, 0);
    Java_java_net_NetworkInterface_init(env, 0);

#ifdef AF_INET6
    pdsi_multicastInterfaceID =
        (*env)->GetFieldID(env, cls, "multicastInterface", "I");
    CHECK_NULL(pdsi_multicastInterfaceID);
    pdsi_loopbackID =
        (*env)->GetFieldID(env, cls, "loopbackMode", "Z");
    CHECK_NULL(pdsi_loopbackID);
    pdsi_ttlID = 
        (*env)->GetFieldID(env, cls, "ttl", "I");
    CHECK_NULL(pdsi_ttlID);
#endif

    /* FIXME do we need this or are we doing it some other place
    ia_clazz = (*env)->FindClass(env, "java/net/InetAddress");
    ia_clazz = (*env)->NewGlobalRef(env, ia_clazz);
    ia_ctor = (*env)->GetMethodID(env, ia_clazz, "<init>", "()V");*/

    /* init_IPv6Available is already called by the various JNI
     * inet address initialization functions above.
    init_IPv6Available(env);
    */

    (*env)->RegisterNatives(env, cls, methods,
	sizeof methods / sizeof methods[0]);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    bind
 * Signature: (ILjava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_bind(JNIEnv *env, jobject thisObj,
					   jint localport, jobject iaObj) {
    /* fdObj is the FileDescriptor field on this */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    /* fd is an int field on fdObj */
    int fd;
    int len = 0;
    SOCKADDR him;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
  
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "iaObj is null.");
	return;
    }
    
    /* bind - pick a port number for local addr*/
    NET_InetAddressToSockaddr(env, iaObj, localport, &him, &len);

    RPeekSocket *s = (RPeekSocket *)fd;

    if (s->Bind(him.ia) != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "BindException",
			"Bind failed");
	return;
    }
#ifdef SOCKET_PAIRS
    s[1].allowReuse(1);
    TInt err = s[1].Bind(him.ia);
    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "BindException",
			"Bind failed");
	return;
    }
#endif
    
    /* intialize the local port */
    if (localport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object.
  	 */
	s->LocalName(him.ia);

	localport = him.ia.Port();

	(*env)->SetIntField(env, thisObj, pdsi_localPortID, localport);
    } else {
	(*env)->SetIntField(env, thisObj, pdsi_localPortID, localport);
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    connect0
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_connect0(JNIEnv *env, jobject thisObj,
					       jobject address, jint port) {
    /*The object's field */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    /* The fdObj'fd */
    jint fd;
    /* The packetAddress address, family and port */
    SOCKADDR rmtaddr;
    int len = 0; 

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }
    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    
    if (IS_NULL(address)) {
	JNU_ThrowNullPointerException(env, "address");
	return;
    }

    NET_InetAddressToSockaddr(env, address, port, (struct sockaddr *)&rmtaddr, &len);

    RPeekSocket *s = (RPeekSocket *)fd;
    TRequestStatus rs = 1;
    s->Connect(rmtaddr.ia, rs);
    User::WaitForRequest(rs);

    if (rs != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Connect failed");
	return;
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    disconnect0
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_disconnect0(JNIEnv *env, jobject thisObj) {
    /* The object's field */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    /* The fdObj'fd */
    jint fd;

    SOCKADDR addr; /* unspecified */

    if (IS_NULL(fdObj)) {
        return;
    } 
    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);

    RPeekSocket *s = (RPeekSocket *)fd;
    TRequestStatus rs = 1;
    s->Connect(addr.ia, rs);
    User::WaitForRequest(rs);

    if (rs != KErrNone) {
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    send
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_send(JNIEnv *env, jobject thisObj,
					   jobject packet) {

    char BUF[MAX_BUFFER_LEN];
    char *fullPacket = NULL;
    int ret, mallocedPacket = JNI_FALSE;
    /* The object's field */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint trafficClass = (*env)->GetIntField(env, thisObj, JNI_STATIC_MD(java_net_PlainDatagramSocketImpl,  pdsi_trafficClassID));

    jbyteArray packetBuffer;
    jobject packetAddress;
    jint packetBufferOffset, packetBufferLen, packetPort;
    jboolean connected;

    /* The fdObj'fd */
    jint fd;

    SOCKADDR rmtaddr, *rmtaddrP=&rmtaddr;
    int len;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }
    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);

    if (IS_NULL(packet)) {
	JNU_ThrowNullPointerException(env, "packet");
	return;
    }

    connected = (*env)->GetBooleanField(env, thisObj, pdsi_connected);

    packetBuffer = (*env)->GetObjectField(env, packet, dp_bufID);
    packetAddress = (*env)->GetObjectField(env, packet, dp_addressID);
    if (IS_NULL(packetBuffer) || IS_NULL(packetAddress)) {
	JNU_ThrowNullPointerException(env, "null buffer || null address");
	return;
    }

    packetBufferOffset = (*env)->GetIntField(env, packet, dp_offsetID);
    packetBufferLen = (*env)->GetIntField(env, packet, dp_lengthID);

    if (connected) {
	/* arg to NET_Sendto () null in this case */
	len = 0;
	rmtaddrP = 0;
    } else {
	packetPort = (*env)->GetIntField(env, packet, dp_portID);
	NET_InetAddressToSockaddr(env, packetAddress, packetPort, (struct sockaddr *)&rmtaddr, &len);
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

#ifdef SOCKET_PAIRS
    RPeekSocket *s = (RPeekSocket *)fd + 1;
#else
    RPeekSocket *s = (RPeekSocket *)fd;
#endif

    if (trafficClass != 0) {
	s->SetOpt(KSoIpTOS, KSolInetIp, trafficClass);
    }

    TRequestStatus rs = 1;
    TPtrC8 des((TUint8 *)fullPacket, packetBufferLen);
    TSockXfrLength slen(0);

    s->wr_lock().Wait();

    if (connected) {
	s->Send(des, 0, rs, slen);
    } else {
	s->SendTo(des, rmtaddrP->ia, 0, rs, slen);
    }
    TInt len2 = slen();
    User::WaitForRequest(rs);

    s->wr_lock().Signal();

    if (rs != KErrNone) {
	switch (rs.Int()) {
	    case KErrCouldNotConnect :
		JNU_ThrowByName(env, JNU_JAVANETPKG "PortUnreachableException",
		    "ICMP Port Unreachable");
		break;
	    default:
		JNU_ThrowByName(env, "java/io/IOException", "sendto failed");
	}
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
Java_java_net_PlainDatagramSocketImpl_peek(JNIEnv *env, jobject thisObj,
					   jobject addressObj) {
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint timeout = (*env)->GetIntField(env, thisObj, pdsi_timeoutID);
    jint fd;
    int n;
    SOCKADDR remote_addr;
    int len;
    char buf[1];
    jint family;
    jobject iaObj;
    int port;
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (IS_NULL(addressObj)) {
	JNU_ThrowNullPointerException(env, "Null address in peek()");
    }

    RPeekSocket *s = (RPeekSocket *)fd;
    TRequestStatus rs;
    TPtr8 des((TUint8 *)buf, 1);

    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    RTimer &waitTimer = SYMBIANthreadGetTimer(CVMexecEnv2threadID(ee));

    s->rd_lock().Wait();

    do {
	s->RecvFrom(des, remote_addr.ia, KSockReadPeek, rs, waitTimer, timeout);
    } while (rs == KErrCancel);

    s->rd_lock().Signal();

    if (rs != KErrNone) {
        switch (rs.Int()) {
	case KErrTimedOut:
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
			    "peek timed out");
	    break;
	case KErrCouldNotConnect:
	    JNU_ThrowByName(env, JNU_JAVANETPKG "PortUnreachableException",
			    "ICMP Port Unreachable");
	    break;
	default:
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		"Peek failed");
	}
        return 0;
    }

    iaObj = NET_SockaddrToInetAddress(env, &remote_addr, &port);
    /* this api can't handle IPV6 addresses */
    if ((*env)->GetIntField(env, iaObj, ia_familyID) == IPv4) {
	int address = (*env)->GetIntField(env, iaObj, ia_addressID);
	(*env)->SetIntField(env, addressObj, ia_addressID, address);
    }
    return port;
}

JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_peekData(JNIEnv *env, jobject thisObj,
					   jobject packet) {
    char BUF[MAX_BUFFER_LEN];
    char *fullPacket = NULL;
    int mallocedPacket = JNI_FALSE;
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint timeout = (*env)->GetIntField(env, thisObj, pdsi_timeoutID);

    jbyteArray packetBuffer;
    jint packetBufferOffset, packetBufferLen;

    int fd;

    int n;
    SOCKADDR remote_addr;
    int len;
    int port;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    }

    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);

    if (IS_NULL(packet)) {
	JNU_ThrowNullPointerException(env, "packet");
	return -1;
    }

    packetBuffer = (*env)->GetObjectField(env, packet, dp_bufID);
    if (IS_NULL(packetBuffer)) {
	JNU_ThrowNullPointerException(env, "packet buffer");
        return -1;
    }
    packetBufferOffset = (*env)->GetIntField(env, packet, dp_offsetID);
    packetBufferLen = (*env)->GetIntField(env, packet, dp_lengthID);

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
	    return -1;
	} else {
	    mallocedPacket = JNI_TRUE;
	}
    } else {
	fullPacket = &(BUF[0]);
    }

    RPeekSocket *s = (RPeekSocket *)fd;
    TRequestStatus rs;
    TPtr8 des((TUint8 *)fullPacket, packetBufferLen);

    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    RTimer &waitTimer = SYMBIANthreadGetTimer(CVMexecEnv2threadID(ee));

    s->rd_lock().Wait();

    do {
	s->RecvFrom(des, remote_addr.ia, KSockReadPeek, rs, waitTimer, timeout);
    } while (rs == KErrCancel);

    s->rd_lock().Signal();

    n = des.Length();

    /* truncate the data if the packet's length is too small */
    if (n > packetBufferLen) {
	n = packetBufferLen;
    }
    if (rs != KErrNone) {
	if (rs == KErrTimedOut) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
			    "peekData timed out");
	} else {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		"peekData failed");
	}
	(*env)->SetIntField(env, packet, dp_offsetID, 0);
	(*env)->SetIntField(env, packet, dp_lengthID, 0);
    } else {
	/*
	 * success - fill in received address...
	 *
	 * TODO: Fill in an int on the packet, and create inetadd
	 * object in Java, as a performance improvement. Also
	 * construct the inetadd object lazily.
	 */

        jobject packetAddress;

        /*
         * Check if there is an InetAddress already associated with this
         * packet. If so we check if it is the same source address. We
         * can't update any existing InetAddress because it is immutable
         */
        packetAddress = (*env)->GetObjectField(env, packet, dp_addressID);
        if (packetAddress != NULL) {
            if (!NET_SockaddrEqualsInetAddress(env, &remote_addr, packetAddress)) {
                /* force a new InetAddress to be created */
                packetAddress = NULL;
            }
        }
	if (packetAddress == NULL) {
	    packetAddress = NET_SockaddrToInetAddress(env, &remote_addr, &port);
	    /* stuff the new Inetaddress in the packet */
	    (*env)->SetObjectField(env, packet, dp_addressID, packetAddress);
	} else {
 	    /* only get the new port number */
 	    port = remote_addr.ia.Port();
  	}
	/* and fill in the data, remote address/port and such */
	(*env)->SetByteArrayRegion(env, packetBuffer, packetBufferOffset, n,
				   (jbyte *)fullPacket);
	(*env)->SetIntField(env, packet, dp_portID, port);
	(*env)->SetIntField(env, packet, dp_lengthID, n);
    }

    if (mallocedPacket) {
	free(fullPacket);
    }
    return port;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    receive
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_receive(JNIEnv *env, jobject thisObj,
					      jobject packet) {
    char BUF[MAX_BUFFER_LEN];
    char *fullPacket = NULL;
    int mallocedPacket = JNI_FALSE;
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint timeout = (*env)->GetIntField(env, thisObj, pdsi_timeoutID);

    jbyteArray packetBuffer;
    jint packetBufferOffset, packetBufferLen;

    int fd;

    int n;
    SOCKADDR remote_addr;
    int len;
    jboolean retry;
    jlong prevTime = 0;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }

    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);

    if (IS_NULL(packet)) {
	JNU_ThrowNullPointerException(env, "packet");
	return;
    }

    packetBuffer = (*env)->GetObjectField(env, packet, dp_bufID);
    if (IS_NULL(packetBuffer)) {
	JNU_ThrowNullPointerException(env, "packet buffer");
        return;
    }
    packetBufferOffset = (*env)->GetIntField(env, packet, dp_offsetID);
    packetBufferLen = (*env)->GetIntField(env, packet, dp_bufLengthID);

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

    jboolean connected = (*env)->GetBooleanField(env, thisObj, pdsi_connected);

    do {
	retry = JNI_FALSE;

	/*
	 * Security Note: For Linux 2.2 with connected datagrams ensure that 
	 * you receive into the stack/heap allocated buffer - do not attempt
	 * to receive directly into DatagramPacket's byte array.
	 * (ie: if the virtual machine support pinning don't use 
	 * GetByteArrayElements or a JNI critical section and receive
	 * directly into the byte array)
	 */

	RPeekSocket *s = (RPeekSocket *)fd;
	TRequestStatus rs;
	TPtr8 des((TUint8 *)fullPacket, packetBufferLen);

	CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
	RTimer &waitTimer = SYMBIANthreadGetTimer(CVMexecEnv2threadID(ee));

	s->rd_lock().Wait();

	do {
	    s->RecvFrom(des, remote_addr.ia, 0, rs, waitTimer, timeout);
	} while (rs == KErrCancel);

	s->rd_lock().Signal();

	n = des.Length();

        /* truncate the data if the packet's length is too small */
        if (n > packetBufferLen) {
            n = packetBufferLen;
        }
        if (rs != KErrNone) {
	    if (rs == KErrTimedOut) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"receive timed out");
	    } else {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		    "Receive failed");
	    }
            (*env)->SetIntField(env, packet, dp_offsetID, 0);
            (*env)->SetIntField(env, packet, dp_lengthID, 0);
        } else {
            int port;
            jobject packetAddress;

            /*
             * success - fill in received address...
             *
             * TODO: Fill in an int on the packet, and create inetadd
             * object in Java, as a performance improvement. Also
             * construct the inetadd object lazily.
             */

            /*
             * Check if there is an InetAddress already associated with this
             * packet. If so we check if it is the same source address. We
             * can't update any existing InetAddress because it is immutable
             */
            packetAddress = (*env)->GetObjectField(env, packet, dp_addressID);
            if (packetAddress != NULL) {
                if (!NET_SockaddrEqualsInetAddress(env, &remote_addr, packetAddress)) {
                    /* force a new InetAddress to be created */
                    packetAddress = NULL;
                }
            }
            if (packetAddress == NULL) {
                packetAddress = NET_SockaddrToInetAddress(env, &remote_addr, &port);
                /* stuff the new Inetaddress in the packet */
                (*env)->SetObjectField(env, packet, dp_addressID, packetAddress);
            } else {
                /* only get the new port number */
                port = remote_addr.ia.Port();
            }
            /* and fill in the data, remote address/port and such */
            (*env)->SetByteArrayRegion(env, packetBuffer, packetBufferOffset, n,
                                       (jbyte *)fullPacket);
            (*env)->SetIntField(env, packet, dp_portID, port);
            (*env)->SetIntField(env, packet, dp_lengthID, n);
        }

    } while (retry);

    if (mallocedPacket) {
	free(fullPacket);
    }
}

extern int SYMBIANsocketServerInit();

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    datagramSocketCreate
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketCreate(JNIEnv *env,
							   jobject thisObj) {
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }

    if (!SYMBIANsocketServerInit()) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Error creating socket");
	return;
    }

#if 0
    RPeekSocket *s = (RPeekSocket *)malloc(sizeof *s);
#else
    RPeekSocket *s = (RPeekSocket *)malloc(2 * sizeof *s);
#endif
    if (s == NULL) {
        JNU_ThrowOutOfMemoryError(env, "new failed");
        return;
    }
#ifdef SOCKET_PAIRS
    new (&s[0]) RPeekSocket();
    new (&s[1]) RPeekSocket();
#else
    new (s) RPeekSocket();
#endif
    _LIT(proto, "udp"); /* ? ip6 ? */
#ifndef SOCKET_PAIRS
    TInt err = s->Open(sserv, proto);
#else
    TInt err = s[0].Open(sserv, proto);
    if (err == KErrNone) {
	err = s[1].Open(sserv, proto);
    }
#endif
    //TInt err = s->Open(sserv, KAfInet, KSockDatagram, KProtocolInetUdp);

    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Error creating socket");
	return;
    }

    fd = (int)s;

#if 0
    int t = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*) &t, sizeof(int));
#endif

    (*env)->SetIntField(env, fdObj, IO_fd_fdID, fd);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    datagramSocketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketClose(JNIEnv *env,
							  jobject thisObj) {
    /*
     * TODO: PUT A LOCK AROUND THIS CODE
     */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    int fd;

    if (IS_NULL(fdObj)) {
	return;
    }
    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    if (fd == -1) {
	return;
    }
    (*env)->SetIntField(env, fdObj, IO_fd_fdID, -1);
    RPeekSocket *s = (RPeekSocket *)fd;
    s->Close();
}


/*
 * Sets the multicast interface. 
 *
 * SocketOptions.IP_MULTICAST_IF :-
 *	value is a InetAddress
 *	IPv4:	set outgoing multicast interface using 
 * 		IPPROTO_IP/IP_MULTICAST_IF
 *	IPv6:	Get the index of the interface to which the
 *		InetAddress is bound
 *		Set outgoing multicast interface using
 *		IPPROTO_IPV6/IPV6_MULTICAST_IF
 *		On Linux 2.2 record interface index as can't
 *		query the multicast interface.
 *
 * SockOptions.IF_MULTICAST_IF2 :-
 *	value is a NetworkInterface 
 *	IPv4:	Obtain IP address bound to network interface
 *		(NetworkInterface.addres[0])
 *		set outgoing multicast interface using 
 *              IPPROTO_IP/IP_MULTICAST_IF
 *	IPv6:	Obtain NetworkInterface.index
 *		Set outgoing multicast interface using
 *              IPPROTO_IPV6/IPV6_MULTICAST_IF
 *              On Linux 2.2 record interface index as can't
 *              query the multicast interface. 
 *
 */
static void setMulticastInterface(JNIEnv *env, jobject thisObj, int fd,
				  jint opt, jobject value)
{
    if (opt == java_net_SocketOptions_IP_MULTICAST_IF) {
	/*
  	 * value is an InetAddress.
	 * On IPv4 system use IP_MULTICAST_IF socket option
	 * On IPv6 system get the NetworkInterface that this IP
	 * address is bound too and use the IPV6_MULTICAST_IF 
	 * option instead of IP_MULTICAST_IF
	 */

	{
	    assert(ni_class != NULL);

	    value = Java_java_net_NetworkInterface_getByInetAddress0(env, ni_class, value);
	    if (value == NULL) {
		if (!(*env)->ExceptionOccurred(env)) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
	                 "bad argument for IP_MULTICAST_IF"
			 ": address not bound to any interface");
		}
		return;
	    }
	    opt = java_net_SocketOptions_IP_MULTICAST_IF2;
	}
    }

    if (opt == java_net_SocketOptions_IP_MULTICAST_IF2) {
	/*
	 * value is a NetworkInterface.
	 * On IPv6 system get the index of the interface and use the
         * IPV6_MULTICAST_IF socket option
	 * On IPv4 system extract addr[0] and use the IP_MULTICAST_IF
         * option.
	 */
        {
	    assert(ni_indexID != NULL);
	    int index = (*env)->GetIntField(env, value, ni_indexID);

	    RPeekSocket *s = (RPeekSocket *)fd;

	    if (s->SetOpt(KSoIp6MulticastIf, KSolInetIp, index) != KErrNone) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			        "Error setting multicast interface");
	    }
        }
    }
}


/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    socketSetOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_socketSetOption(JNIEnv *env,
						      jobject thisObj,
						      jint opt,
						      jobject value) {
    int fd;
    int level, optname, optlen = 0;
    union {
        int i;
	char c;
    } optval;

    /*
     * Check that socket hasn't been closed
     */
    fd = getFD(env, thisObj);
    if (fd < 0) { 
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }

    /*
     * Check argument has been provided
     */
    if (IS_NULL(value)) {
	JNU_ThrowNullPointerException(env, "value argument");
	return;
    }

    /*
     * Setting the multicast interface handled seperately 
     */
    if (opt == java_net_SocketOptions_IP_MULTICAST_IF ||
	opt == java_net_SocketOptions_IP_MULTICAST_IF2) {

	setMulticastInterface(env, thisObj, fd, opt, value);
	return;
    }

    RPeekSocket *s = (RPeekSocket *)fd;

    /*
     * Map the Java level socket option to the platform specific
     * level and option name.
     */
    if (NET_MapSocketOption(opt, &level, &optname)) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Invalid option");
        return;
    }

    switch (opt) {
        case java_net_SocketOptions_SO_SNDBUF :
        case java_net_SocketOptions_SO_RCVBUF :
        case java_net_SocketOptions_IP_TOS :
            {
                jclass cls;
                jfieldID fid;

		cls = (*env)->FindClass(env, "java/lang/Integer");
		CHECK_NULL(cls);
		fid =  (*env)->GetFieldID(env, cls, "value", "I");
		CHECK_NULL(fid);

                optval.i = (*env)->GetIntField(env, value, fid);
                optlen = sizeof(optval.i);
                break;
            }

	case java_net_SocketOptions_SO_REUSEADDR:
	case java_net_SocketOptions_SO_BROADCAST:
	case java_net_SocketOptions_IP_MULTICAST_LOOP:
	    {
		jclass cls;
		jfieldID fid;
		jboolean on;

		cls = (*env)->FindClass(env, "java/lang/Boolean");
		CHECK_NULL(cls);
		fid =  (*env)->GetFieldID(env, cls, "value", "Z");
		CHECK_NULL(fid);

		on = (*env)->GetBooleanField(env, value, fid);
		if (opt == java_net_SocketOptions_IP_MULTICAST_LOOP) {

		    /*
		     * IP_MULTICAST_LOOP may be mapped to IPPROTO (arg
		     * type 'char') or IPPROTO_V6 (arg type 'int').
		     *
		     * In addition setLoopbackMode(true) disables 
		     * IP_MULTICAST_LOOP - doesn't enable it.
		     */
		    optval.i = (!on ? 1 : 0);
		    optlen = sizeof(optval.i);

		} else {
		    /* SO_REUSEADDR or SO_BROADCAST */
		    optval.i = (on ? 1 : 0);
		    optlen = sizeof(optval.i);
		}

	    	break;
	    }

	default :
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
		"Socket option not supported by PlainDatagramSocketImp");
	    break;

    }

    if (s->SetOpt(optname, level, optval.i) != KErrNone) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Error setting socket option");
	return;
    }
}


/*
 * Return the multicast interface:
 *
 * SocketOptions.IP_MULTICAST_IF
 *	IPv4:	Query IPPROTO_IP/IP_MULTICAST_IF
 *		Create InetAddress
 *		IP_MULTICAST_IF returns struct ip_mreqn on 2.2
 *		kernel but struct in_addr on 2.4 kernel
 *	IPv6:	Query IPPROTO_IPV6 / IPV6_MULTICAST_IF or
 *		obtain from impl is Linux 2.2 kernel
 *		If index == 0 return InetAddress representing
 *		anyLocalAddress.
 *		If index > 0 query NetworkInterface by index
 *		and returns addrs[0]
 *
 * SocketOptions.IP_MULTICAST_IF2
 *	IPv4:	Query IPPROTO_IP/IP_MULTICAST_IF
 *		Query NetworkInterface by IP address and
 *		return the NetworkInterface that the address
 *		is bound too.
 *	IPv6:	Query IPPROTO_IPV6 / IPV6_MULTICAST_IF
 *		(except Linux .2 kernel)
 *		Query NetworkInterface by index and
 *		return NetworkInterface.	
 */
jobject getMulticastInterface(JNIEnv *env, jobject thisObj, int fd, jint opt)
{

    /*
     * IPv6 implementation
     */
    assert ((opt == java_net_SocketOptions_IP_MULTICAST_IF) ||
            (opt == java_net_SocketOptions_IP_MULTICAST_IF2));
    {
	static jmethodID ia_anyLocalAddressID;

	int index;
	int len = sizeof(index);

	jobjectArray addrArray;
        jobject addr;
	jobject ni; 

	RPeekSocket *s = (RPeekSocket *)fd;

	if (s->GetOpt(KSoIp6MulticastIf, KSolInetIp, index) != KErrNone) {
#if 0
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			     "Error getting multicast interrace");
	    return NULL;
#else
	    index = 0;
#endif
	}

	jclass c = (*env)->FindClass(env, "java/net/NetworkInterface");
	CHECK_NULL_RETURN(c, NULL);
	CHECK_NULL_RETURN(ni_class, NULL);
	assert(ni_class != NULL);
	c = (*env)->FindClass(env, "java/net/InetAddress");
	CHECK_NULL_RETURN(c, NULL);
	CHECK_NULL_RETURN(ia_class, NULL);
	{
	    ia_anyLocalAddressID = (*env)->GetStaticMethodID(env, 
							     ia_class,
						             "anyLocalAddress",
							     "()Ljava/net/InetAddress;");
	    CHECK_NULL_RETURN(ia_anyLocalAddressID, NULL);
	}

	/*
	 * If multicast to a specific interface then return the 
	 * interface (for IF2) or the any address on that interface
	 * (for IF).
	 */
	if (index > 0) {
	    ni = Java_java_net_NetworkInterface_getByIndex(env, ni_class,
								   index);
	    if (ni == NULL) {
		char errmsg[255];
		sprintf(errmsg, 
			"IPV6_MULTICAST_IF returned index to unrecognized interface: %d",
			index);
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", errmsg);
		return NULL;
	    }

            /*
             * For IP_MULTICAST_IF2 return the NetworkInterface
             */
	    if (opt == java_net_SocketOptions_IP_MULTICAST_IF2) {
                return ni;
            }

	    /*
	     * For IP_MULTICAST_IF return addrs[0] 
	     */
	    addrArray = (*env)->GetObjectField(env, ni, ni_addrsID);
	    if ((*env)->GetArrayLength(env, addrArray) < 1) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		    "IPV6_MULTICAST_IF returned interface without IP bindings");
		return NULL;
	    }

	    addr = (*env)->GetObjectArrayElement(env, addrArray, 0);
	    return addr;
  	}

	/* 
	 * Multicast to any address - return anyLocalAddress
	 * or a NetworkInterface with addrs[0] set to anyLocalAddress
	 */

	addr = (*env)->CallStaticObjectMethod(env, ia_class, ia_anyLocalAddressID,
					      NULL);
	if (opt == java_net_SocketOptions_IP_MULTICAST_IF) {
	    return addr;
	}

	ni = (*env)->NewObject(env, ni_class, ni_ctrID, 0);
	CHECK_NULL_RETURN(ni, NULL);
        (*env)->SetIntField(env, ni, ni_indexID, -1);
        addrArray = (*env)->NewObjectArray(env, 1, ia_class, NULL);
	CHECK_NULL_RETURN(addrArray, NULL);
        (*env)->SetObjectArrayElement(env, addrArray, 0, addr);
        (*env)->SetObjectField(env, ni, ni_addrsID, addrArray);
        return ni;
    }
}



/*
 * Returns relevant info as a jint.
 *
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    socketGetOption
 * Signature: (I)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
Java_java_net_PlainDatagramSocketImpl_socketGetOption(JNIEnv *env, jobject thisObj,
						      jint opt) {
    int fd;
    int level, optname, optlen;
    union {
        int i;
	char c;
    } optval;

    fd = getFD(env, thisObj);
    if (fd < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"socket closed");
	return NULL;
    }

    /*
     * Handle IP_MULTICAST_IF seperately
     */
    if (opt == java_net_SocketOptions_IP_MULTICAST_IF ||
	opt == java_net_SocketOptions_IP_MULTICAST_IF2) {
	return getMulticastInterface(env, thisObj, fd, opt);

    }

    /*
     * SO_BINDADDR implemented using getsockname
     */
    if (opt == java_net_SocketOptions_SO_BINDADDR) {
	/* find out local IP address */
	SOCKADDR him;
	int port;
	jobject iaObj;

	RPeekSocket *s = (RPeekSocket *)fd;
	s->LocalName(him.ia);

	iaObj = NET_SockaddrToInetAddress(env, &him, &port);

	return iaObj;
    }

    /*
     * Map the Java level socket option to the platform specific
     * level and option name.
     */
    if (NET_MapSocketOption(opt, &level, &optname)) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Invalid option");
        return NULL;
    }

#if 0
    if (opt == java_net_SocketOptions_IP_MULTICAST_LOOP &&
	level == IPPROTO_IP) {
	optlen = sizeof(optval.c);
    } else {
	optlen = sizeof(optval.i);
    }
#endif

    RPeekSocket *s = (RPeekSocket *)fd;
    if (s->GetOpt(optname, level, optval.i) != KErrNone) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
                         "Error getting socket option");
        return NULL;
    }

    switch (opt) {
	case java_net_SocketOptions_IP_MULTICAST_LOOP:
	    /* getLoopbackMode() returns true if IP_MULTICAST_LOOP disabled */
#if 0
	    if (level == IPPROTO_IP) {
		return createBoolean(env, (int)!optval.c);
	    } else {
		return createBoolean(env, !optval.i);
	    }
#endif

	case java_net_SocketOptions_SO_BROADCAST:
	case java_net_SocketOptions_SO_REUSEADDR:
	    return createBoolean(env, optval.i);

	case java_net_SocketOptions_SO_SNDBUF:
 	case java_net_SocketOptions_SO_RCVBUF:
	case java_net_SocketOptions_IP_TOS:
	    return createInteger(env, optval.i);

    }

    /* should never rearch here */
    return NULL;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    setTTL
 * Signature: (B)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTimeToLive(JNIEnv *env, jobject thisObj,
						    jint ttl)
{
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    int fd;
    /* it is important to cast this to a char, otherwise setsockopt gets confused */

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    
    RPeekSocket *s = (RPeekSocket *)fd;
    TInt ittl = (TInt)ttl;
    if (s->SetOpt(KSoIpTTL, KSolInetIp, ittl) != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		       "Error setting socket option");
    }
}


/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    getTTL
 * Signature: ()B
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_getTimeToLive(JNIEnv *env, jobject thisObj) {
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint fd = -1;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    /* getsockopt of ttl */
    {
	TInt ttl = 0;
	RPeekSocket *s = (RPeekSocket *)fd;
	if (s->GetOpt(KSoIpTTL, KSolInetIp, ttl) != KErrNone) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			   "Error getting socket option");
	    return -1;
	}
	return (jint)ttl;
    }
}


/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    join
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_join(JNIEnv *env, jobject thisObj,
					   jobject address, jobject niaddress)
{
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint fd, addressAddress;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }

    if (IS_NULL(address)) {
      JNU_ThrowNullPointerException(env, "address");
      return;
    } else {
      addressAddress = (*env)->GetIntField(env, address, ia_addressID);
    }

    TIp6Mreq req;

    /*
     * Set the multicast group address in the ip_mreq field
     * eventually this check should be done by the security manager
     */
    TInetAddr ia;
    ia.SetAddress(addressAddress);
    req.iAddr = ia.Ip6Address();
    req.iInterface = 0;
    
    if (!ia.IsMulticast()) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
	    "not in multicast");
	return;
    }

    RPeekSocket *s = (RPeekSocket *)fd;
    TInt inf = 0;

    if (IS_NULL(niaddress)) {
	/* Get the interface */
	if (s->GetOpt(KSoIp6MulticastIf, KSolInetIp, inf) != KErrNone) {
#if 0
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			     "Error getting multicast interface");
	    return;
#else
	    inf = 0;
#endif
	}
    } else {
	inf = (*env)->GetIntField(env, niaddress, ni_indexID);
    }
    req.iInterface = inf;

    /* Join the multicast group */
    TPckgBuf<TIp6Mreq> pkg(req);
    if (s->SetOpt(KSoIp6JoinGroup, KSolInetIp, pkg) != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			 "Error joining multicast group");
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    leave
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_leave(JNIEnv *env, jobject thisObj,
					    jobject address, jobject niaddress) {
    jobject fdObj = (*env)->GetObjectField(env, thisObj, pdsi_fdID);
    jint fd, addressAddress;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }

    if (IS_NULL(address)) {
	JNU_ThrowNullPointerException(env, "address argument");
	return;
    } else {
	addressAddress = (*env)->GetIntField(env, address, ia_addressID);
    }

    TIp6Mreq req;

    /*
     * Set the multicast group address in the ip_mreq field
     * eventually this check should be done by the security manager
     */

    TInetAddr ia;
    ia.SetAddress(addressAddress);
    req.iAddr = ia.Ip6Address();
    req.iInterface = 0;

    if (!ia.IsMulticast()) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
	return;
    }

    RPeekSocket *s = (RPeekSocket *)fd;
    TInt inf;

    /* Get the interface */
    if (IS_NULL(niaddress)) {
	if (s->GetOpt(KSoIp6MulticastIf, KSolInetIp, inf) != KErrNone) {
#if 0
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			     "Error getting multicast interface");
	    return;
#else
	    inf = 0;
#endif
	}
    } else {
	inf = (*env)->GetIntField(env, niaddress, ni_indexID);
    }
    req.iInterface = inf;

    /* Leave the multicast group */
    TPckgBuf<TIp6Mreq> pkg(req);
    if (s->SetOpt(KSoIp6LeaveGroup, KSolInetIp, pkg) != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			 "Error leaving multicast group");
    }
}
