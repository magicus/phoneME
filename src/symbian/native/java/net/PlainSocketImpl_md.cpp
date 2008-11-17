/*
 * @(#)PlainSocketImpl_md.cpp	1.10 06/10/10
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
#include <stdlib.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_SocketOptions.h"
#include "java_net_PlainSocketImpl.h"

#include <es_sock.h>
#include <in_sock.h>

#include "RLockingSocket.hpp"

extern RSocketServ sserv;

RTimer &SYMBIANthreadGetTimer(CVMThreadID *self);

/************************************************************************
 * PlainSocketImpl
 */

#include "jni_statics.h"

#define IO_fd_fdID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID)
#define psi_addressID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_addressID) 
#define psi_closePendingID \
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_closePendingID)
#define psi_fdID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID)
#define psi_fdLockID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdLockID)
#define psi_localportID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID) 
#define psi_portID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_portID) 
#define psi_serverSocketID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_serverSocketID)
#define psi_timeoutID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_timeoutID) 
#define psi_trafficClassID	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, psi_trafficClassID)
#define socketExceptionCls	\
	JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls)


/*
 * Return the file descriptor given a PlainSocketImpl
 */
static int getFD(JNIEnv *env, jobject thisObj) {
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    CHECK_NULL_RETURN(fdObj, -1);
    return (*env)->GetIntField(env, fdObj, IO_fd_fdID);
}

/*
 * The initProto function is called whenever PlainSocketImpl is
 * loaded, to cache fieldIds for efficiency. This is called everytime
 * the Java class is loaded.
 *
 * Class:     java_net_PlainSocketImpl
 * Method:    initProto
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_initProto(JNIEnv *env, jclass cls) {

    psi_fdID
	= (*env)->GetFieldID(env, cls , "fd", "Ljava/io/FileDescriptor;");
    CHECK_NULL(psi_fdID);
    psi_addressID
	= (*env)->GetFieldID(env, cls, "address", "Ljava/net/InetAddress;");
    CHECK_NULL(psi_addressID);
    psi_portID
	= (*env)->GetFieldID(env, cls, "port", "I");
    CHECK_NULL(psi_portID);
    psi_localportID
	= (*env)->GetFieldID(env, cls, "localport", "I");
    CHECK_NULL(psi_localportID);
    psi_timeoutID
	= (*env)->GetFieldID(env, cls, "timeout", "I");
    CHECK_NULL(psi_timeoutID);
    
    psi_trafficClassID = 
        (*env)->GetFieldID(env, cls, "trafficClass", "I");
    CHECK_NULL(psi_trafficClassID);
    psi_serverSocketID = 
        (*env)->GetFieldID(env, cls, "serverSocket", "Ljava/net/ServerSocket;");
    CHECK_NULL(psi_serverSocketID);
    psi_fdLockID = 
        (*env)->GetFieldID(env, cls, "fdLock", "Ljava/lang/Object;");
    CHECK_NULL(psi_fdLockID);
    psi_closePendingID = 
        (*env)->GetFieldID(env, cls, "closePending", "Z");
    CHECK_NULL(psi_closePendingID);
    IO_fd_fdID
	= NET_GetFileDescriptorID(env);
    CHECK_NULL(IO_fd_fdID);

    init_IPv6Available(env);
}

/* a global reference to the java.net.SocketException class. In
 * socketCreate, we ensure that this is initialized. This is to
 * prevent the problem where socketCreate runs out of file
 * descriptors, and is then unable to load the exception class.
 */
/*  static jclass socketExceptionCls; */

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketCreate
 * Signature: (Z)V */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketCreate(JNIEnv *env, jobject thisObj,
					   jboolean stream) {
    jobject fdObj, ssObj;
    int fd;

    if (socketExceptionCls == NULL) {
	jclass c = (*env)->FindClass(env, "java/net/SocketException");
	socketExceptionCls
	    = (jclass)(*env)->NewGlobalRef(env, c);
	CHECK_NULL(socketExceptionCls);
    }
    fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);

    if (fdObj == NULL) {
	(*env)->ThrowNew(env, socketExceptionCls, "null fd object");
	return;
    }

    RLockingSocket *s = (RLockingSocket *)malloc(sizeof *s);
    if (s == NULL) {
	JNU_ThrowOutOfMemoryError(env, "new failed");
	return;
    }
    new (s) RLockingSocket();
    TUint ktype, kproto;
    if (stream) {
	kproto = KProtocolInetTcp;
	ktype = KSockStream;
    } else {
	kproto = KProtocolInetUdp;
	ktype = KSockDatagram;
    }
    //TInt err = s->Open(sserv, KAfInet6, ktype, kproto);
    TInt err = s->Open(sserv, KAfInet, ktype, kproto);

    if (err != KErrNone) {
	(*env)->ThrowNew(env, socketExceptionCls, "RLockingSocket Open failed");
	s->~RLockingSocket();
	free(s);
	return;
    } else {
	fd = (jint)s;
	(*env)->SetIntField(env, fdObj, IO_fd_fdID, fd);
    }

    /*
     * If this is a server socket then enable SO_REUSEADDR
     * automatically
     */
    ssObj = (*env)->GetObjectField(env, thisObj, psi_serverSocketID);
    if (ssObj != NULL) {
	s->SetOpt(KSoReuseAddr, KSolInetIp, 1);
    }
}

/*
 * inetAddress is the address object passed to the socket connect
 * call.
 *
 * Class:     java_net_PlainSocketImpl
 * Method:    socketConnect
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketConnect(JNIEnv *env, jobject thisObj,
					    jobject iaObj, jint port,
					    jint timeout)
{
    jint localport = (*env)->GetIntField(env, thisObj, psi_localportID);
    int len = 0;

    /* fdObj is the FileDescriptor field on this */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    jobject fdLock;

    jint trafficClass = (*env)->GetIntField(env, thisObj, psi_trafficClassID);

    /* fd is an int field on iaObj */
    jint fd;

    SOCKADDR him;
    /* The result of the connection */
    int connect_rv = -1;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "inet address argument null.");
	return;
    }

    /* connect */
    RLockingSocket *s = (RLockingSocket *)fd;

    NET_InetAddressToSockaddr(env, iaObj, port, (struct sockaddr *)&him, &len);
    if (trafficClass != 0) {
	s->SetOpt(KSoIpTOS, KSolInetIp, trafficClass);
    }
    TRequestStatus rs = 1;
    if (timeout <= 0) {
	s->Connect(him.ia, rs);
	User::WaitForRequest(rs);
    } else {
	CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
	TInt millis = timeout;
        TTime time;
        time.HomeTime();
        time += TTimeIntervalSeconds(millis / 1000);
        time += TTimeIntervalMicroSeconds32((millis % 1000) * 1000);
	RTimer &waitTimer = SYMBIANthreadGetTimer(CVMexecEnv2threadID(ee));
	TRequestStatus waitStatus = 2;
        waitTimer.At(waitStatus, time);
	s->Connect(him.ia, rs);

	User::WaitForRequest(rs, waitStatus);

	if (rs == KRequestPending) {
	    if (waitStatus != KRequestPending) {
		waitTimer.Cancel();
		s->CancelConnect();
		s->Shutdown(RSocket::EImmediate, rs);
		User::WaitForRequest(rs);
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"connect timed out");
		return;
	    } else {
		/* Should not happen */
		s->CancelConnect();
		assert(0);
	    }
	}
	waitTimer.Cancel();
    }

    /* report the appropriate exception */
    if (rs != KErrNone) {
	switch (rs.Int()) {
	case KErrNoProtocolOpt:
	    JNU_ThrowByName(env,
		JNU_JAVANETPKG "ProtocolException",
		"Protocol error");
	    break;
	case KErrCouldNotConnect:
	    JNU_ThrowByName(env, JNU_JAVANETPKG "ConnectException",
			    "Connection refused");
	    break;
	case KErrHostUnreach:
            JNU_ThrowByName(env,
		JNU_JAVANETPKG "NoRouteToHostException",
		"Host unreachable");
	    break;
	case KErrNetUnreach:
            JNU_ThrowByName(env,
		JNU_JAVANETPKG "NoRouteToHostException",
		"Network unreachable");
	    break;
	default:
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		"connect failed");
	}
	return;
    }

    /*
     * The socket may have been closed while we were connecting.
     * To avoid any race conditions we therefore grab the
     * fd lock, check if the socket has been closed, and 
     * set the various fields whilst holding the lock
     */
    fdLock = (*env)->GetObjectField(env, thisObj, psi_fdLockID);
    (*env)->MonitorEnter(env, fdLock);

    if ((*env)->GetBooleanField(env, thisObj, psi_closePendingID)) {

	/* release fdLock */
	(*env)->MonitorExit(env, fdLock);

	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
                            "Socket closed");
	return;
    }

    (*env)->SetIntField(env, fdObj, IO_fd_fdID, fd);

    /* set the remote peer address and port */
    (*env)->SetObjectField(env, thisObj, psi_addressID, iaObj);
    (*env)->SetIntField(env, thisObj, psi_portID, port);

    /*
     * we need to initialize the local port field if bind was called
     * previously to the connect (by the client) then localport field
     * will already be initialized
     */
    if (localport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object.
  	 */
	s->LocalName(him.ia);
	localport = him.ia.Port();
	(*env)->SetIntField(env, thisObj, psi_localportID, localport);
    }

    /*
     * Finally release fdLock
     */
    (*env)->MonitorExit(env, fdLock);
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketBind
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketBind(JNIEnv *env, jobject thisObj,
					 jobject iaObj, jint localport) {

    /* fdObj is the FileDescriptor field on this */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    /* fd is an int field on fdObj */
    int fd;
    int len;
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

    /* bind */
    NET_InetAddressToSockaddr(env, iaObj, localport, (struct sockaddr *)&him, &len);

    RLockingSocket *s = (RLockingSocket *)fd;
    TInt err = s->Bind(him.ia);

    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			"Bind failed");
    }

    /* set the address */
    (*env)->SetObjectField(env, thisObj, psi_addressID, iaObj);

    /* intialize the local port */
    if (localport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object.
  	 */
	s->LocalName(him.ia);
	localport = him.ia.Port();
        (*env)->SetIntField(env, thisObj, psi_localportID, localport);
    } else {
	(*env)->SetIntField(env, thisObj, psi_localportID, localport);
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketListen
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketListen (JNIEnv *env, jobject thisObj,
					    jint count)
{
    /* this FileDescriptor fd field */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    /* fdObj's int fd field */
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }

    RLockingSocket *s = (RLockingSocket *)fd;

    // Work around Symbian bug
    if (count > 0x007fffff) {
	count = 0x007fffff;
    }
    TInt err = s->Listen(count);

    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Listen failed");
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketAccept
 * Signature: (Ljava/net/SocketImpl;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketAccept(JNIEnv *env, jobject thisObj,
					   jobject socket)
{
    /* fields on this */
    int port;
    jint timeout = (*env)->GetIntField(env, thisObj, psi_timeoutID);
    jlong prevTime = 0;
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);

    /* the FileDescriptor field on socket */
    jobject socketFdObj;
    /* the InetAddress field on socket */
    jobject socketAddressObj;

    /* the ServerSocket fd int field on fdObj */
    jint fd;

    /* accepted fd */
    jint newfd;

    SOCKADDR him;
    int len = SOCKADDR_LEN;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (IS_NULL(socket)) {
	JNU_ThrowNullPointerException(env, "socket is null");
	return;
    }

    RLockingSocket *s = (RLockingSocket *)fd;
    RLockingSocket *a = (RLockingSocket *)malloc(sizeof *s);
    if (s == NULL) {
	JNU_ThrowOutOfMemoryError(env, "new failed");
	return;
    }
    new (a) RLockingSocket();

    TInt err = a->Open(sserv);
    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
	    "Accept failed");
	return;
    }

    TRequestStatus rs = 1;
    if (timeout <= 0) {
	s->Accept(*a, rs);
	User::WaitForRequest(rs);
    } else {
	CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
	TInt millis = timeout;
	TTime time;
	time.HomeTime();
	time += TTimeIntervalSeconds(millis / 1000);
	time += TTimeIntervalMicroSeconds32((millis % 1000) * 1000);
	RTimer &waitTimer = SYMBIANthreadGetTimer(CVMexecEnv2threadID(ee));
	TRequestStatus waitStatus = 2;
	waitTimer.At(waitStatus, time);
	s->Accept(*a, rs);

	User::WaitForRequest(rs, waitStatus);

	if (rs == KRequestPending) {
	    if (waitStatus != KRequestPending) {
		s->CancelAccept();
		waitTimer.Cancel();
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"connect timed out");
		return;
	    } else {
		/* Should not happen */
		s->CancelAccept();
		assert(0);
	    }
	}
	waitTimer.Cancel();
    }

    if (rs != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
	    "Accept failed");
	return;
    }
    a->RemoteName(him.ia);

    /*
     * fill up the remote peer port and address in the new socket structure.
     */
    socketAddressObj = NET_SockaddrToInetAddress(env, (struct sockaddr *)&him, &port);
    if (socketAddressObj == NULL) {
	/* should be pending exception */
	a->Close();
	a->~RLockingSocket();
	free(a);
	return;
    }
    newfd = (jint)a;

    /*
     * Populate SocketImpl.fd.fd
     */
    socketFdObj = (*env)->GetObjectField(env, socket, psi_fdID);
    (*env)->SetIntField(env, socketFdObj, IO_fd_fdID, newfd);

    (*env)->SetObjectField(env, socket, psi_addressID, socketAddressObj);
    (*env)->SetIntField(env, socket, psi_portID, port);

    /* also fill up the local port information */
    port = (*env)->GetIntField(env, thisObj, psi_localportID);
    (*env)->SetIntField(env, socket, psi_localportID, port);
}


/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketAvailable
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainSocketImpl_socketAvailable(JNIEnv *env, jobject thisObj) {

    jint ret = -1;
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    jint fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    RLockingSocket *s = (RLockingSocket *)fd;
    TInt n;
    TInt err = s->GetOpt(KSOReadBytesPending, KSOLSocket, n);

    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"GetOpt failed");
    }
    return n;
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketClose0
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketClose0(JNIEnv *env, jobject thisObj,
					  jboolean /* useDeferredClose */) {

    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    jint fd;

    /*
     * WARNING: THIS NEEDS LOCKING. ALSO: SHOULD WE CHECK for fd being
     * -1 already?
     */
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"socket already closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (fd != -1) {
	(*env)->SetIntField(env, fdObj, IO_fd_fdID, -1);
	RLockingSocket *s = (RLockingSocket *)fd;
	s->Close();
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketShutdown
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketShutdown(JNIEnv *env, jobject thisObj,
					     jint howto) 
{

    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    jint fd;

    /*
     * WARNING: THIS NEEDS LOCKING. ALSO: SHOULD WE CHECK for fd being
     * -1 already?
     */
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"socket already closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    RLockingSocket *s = (RLockingSocket *)fd;
    TRequestStatus rs;
    if (howto == java_net_PlainSocketImpl_SHUT_RD) {
	s->Shutdown(RSocket::EStopInput, rs);
    } else if (howto == java_net_PlainSocketImpl_SHUT_WR) {
	s->Shutdown(RSocket::EStopOutput, rs);
    }
    User::WaitForRequest(rs);
}


/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketSetOption
 * Signature: (IZLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketSetOption(JNIEnv *env, jobject thisObj,
                                              jint cmd, jboolean on,
                                              jobject value)
{
    int fd;

    /* 
     * Check that socket hasn't been closed 
     */
    fd = getFD(env, thisObj);
    if (fd == -1) {
	/* the fd is closed */
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }

    /*
     * SO_TIMEOUT is a no-op
     */
    if (cmd == java_net_SocketOptions_SO_TIMEOUT) {
	return;
    }

    TInt level;
    TInt optname;
    /*
     * Map the Java level socket option to the platform specific
     * level and option name.
     */
    if (NET_MapSocketOption(cmd, &level, &optname)) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
	    "Invalid option");
        return;
    }

    RLockingSocket *s = (RLockingSocket *)fd;

    TInt ival = 0;
    TInt err = KErrNotSupported;
    switch (cmd) {
	case java_net_SocketOptions_SO_TIMEOUT:
	case java_net_SocketOptions_SO_SNDBUF :
	case java_net_SocketOptions_SO_RCVBUF :
	case java_net_SocketOptions_SO_LINGER :
	case java_net_SocketOptions_IP_TOS : {
	    jclass cls;
	    jfieldID fid;

	    cls = (*env)->FindClass(env, "java/lang/Integer");
	    CHECK_NULL(cls);
	    fid = (*env)->GetFieldID(env, cls, "value", "I");
	    CHECK_NULL(fid);
	    ival = (*env)->GetIntField(env, value, fid);
	    break;
	}
	// Boolean --> int
	default :
	    ival = (on ? 1 : 0);
    }

    err = s->SetOpt(optname, level, ival);

    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Error setting socket option");
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainSocketImpl_socketGetOption(JNIEnv *env, jobject thisObj,
                                              jint cmd, jobject iaContainerObj) {

    int fd;

    /*
     * Check that socket hasn't been closed
     */
    fd = getFD(env, thisObj);
    if(fd == -1) {
	/* the fd is closed */
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    }

    RLockingSocket *s = (RLockingSocket *)fd;

    /*
     * SO_BINDADDR isn't a socket option
     */
    if (cmd == java_net_SocketOptions_SO_BINDADDR) {
	SOCKADDR him;
        int len = 0;
        int port;
        jobject iaObj;
        jclass iaCntrClass;
        jfieldID iaFieldID;

        len = SOCKADDR_LEN;

	s->LocalName(him.ia);

        iaObj = NET_SockaddrToInetAddress(env, (struct sockaddr *)&him, &port);
	CHECK_NULL_RETURN(iaObj, -1);

        iaCntrClass = (*env)->GetObjectClass(env, iaContainerObj);
        iaFieldID = (*env)->GetFieldID(env, iaCntrClass, "addr",
		"Ljava/net/InetAddress;");
	CHECK_NULL_RETURN(iaFieldID, -1);
        (*env)->SetObjectField(env, iaContainerObj, iaFieldID, iaObj);
        return 0; /* notice change from before */
    }

    TInt level = 0;
    TInt optname = 0;
    TInt ival = 0;

    /*
     * Map the Java level socket option to the platform specific
     * level and option name.
     */
    if (NET_MapSocketOption(cmd, &level, &optname)) {
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Invalid option");
        return -1;
    }

    if (s->GetOpt(optname, level, ival) != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
	    "Error getting socket option");
        return -1;
    }

    switch (cmd) {
	case java_net_SocketOptions_SO_LINGER:
	case java_net_SocketOptions_SO_SNDBUF:
        case java_net_SocketOptions_SO_RCVBUF:
        case java_net_SocketOptions_IP_TOS:
            return ival;

	default :
	    return (ival == 0) ? -1 : 1;
    }
}


/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketSendUrgentData
 * Signature: (B)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketSendUrgentData(JNIEnv *env, jobject thisObj, jint data) {
    /* The fd field */
    jobject fdObj = (*env)->GetObjectField(env, thisObj, psi_fdID);
    int n, fd;
    unsigned char d = data & 0xFF;

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
    RLockingSocket *s = (RLockingSocket *)fd;
    TPtr8 des((TUint8 *)&d, 1);
    TRequestStatus rs;
    s->Send(des, KSockWriteUrgent, rs);
    User::WaitForRequest(rs);
    if (rs != KErrNone) {
        JNU_ThrowByName(env, "java/io/IOException", "Write failed");
        return;
    }
    n = des.Length();
}
