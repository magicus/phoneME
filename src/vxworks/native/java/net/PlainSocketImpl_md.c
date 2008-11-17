/*
 * @(#)PlainSocketImpl_md.c	1.12 06/10/10
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
#include <netinet/tcp.h>	/* Defines TCP_NODELAY, needed for 2.6 */
#include <netinet/in.h>
#include <netdb.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_SocketOptions.h"
#include "java_net_PlainSocketImpl.h"

/************************************************************************
 * PlainSocketImpl
 */

#include "jni_statics.h"

/*
static jfieldID IO_fd_fdID;

jfieldID psi_fdID;
jfieldID psi_addressID;
jfieldID psi_portID;
jfieldID psi_localportID;
jfieldID psi_timeoutID;
*/

/*
 * the level of the TCP protocol for JVM_SetSockOpt and JVM_GetSockOpt
 * we only want to look this up once, from the static initializer
 * of PlainSocketImpl
 */
/*
static int tcp_level = -1;
*/

/*
 * the maximum buffer size. Used for setting
 * SendBufferSize and ReceiveBufferSize.
 */
static const int max_buffer_size = 64 * 1024;

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

    JNI_STATIC_MD(java_net_PlainSocketImpl, tcp_level) = IPPROTO_TCP;

    JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID) = (*env)->GetFieldID(env, cls , "fd",
				  "Ljava/io/FileDescriptor;");
    JNI_STATIC_MD(java_net_PlainSocketImpl, psi_addressID) = (*env)->GetFieldID(env, cls, "address",
					  "Ljava/net/InetAddress;");
    JNI_STATIC_MD(java_net_PlainSocketImpl, psi_portID) = (*env)->GetFieldID(env, cls, "port", "I");
    JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID) = (*env)->GetFieldID(env, cls, "localport", "I");
    JNI_STATIC_MD(java_net_PlainSocketImpl, psi_timeoutID) = (*env)->GetFieldID(env, cls, "timeout", "I");
    JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID) = NET_GetFileDescriptorID(env);
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
Java_java_net_PlainSocketImpl_socketCreate(JNIEnv *env, jobject this,
					   jboolean stream) {
    jobject fdObj;
    int fd;
    int arg = -1;

    if (JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls) == NULL) {
	JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls) =
	    (*env)->FindClass(env, "java/net/SocketException");
	JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls) =
	    (jclass)(*env)->NewGlobalRef(env, JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls));
	if (JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls) == NULL) {
	    return;
	}
    }
    fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));

    if (fdObj == NULL) {
	(*env)->ThrowNew(env, JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls), "null fd object");
	return;
    }
    fd = JVM_Socket(AF_INET, (stream ? SOCK_STREAM: SOCK_DGRAM), 0);

    if (fd == JVM_IO_ERR) {
	/* note: if you run out of fds, you may not be able to load
	 * the exception class, and get a NoClassDefFoundError
	 * instead.
	 */
	(*env)->ThrowNew(env, JNI_STATIC_MD(java_net_PlainSocketImpl, socketExceptionCls), strerror(errno));
	return;
    } else {
	(*env)->SetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID), fd);
    }
    JVM_SetSockOpt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&arg, 4);
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
Java_java_net_PlainSocketImpl_socketConnect(JNIEnv *env, jobject this,
					    jobject iaObj, jint port)
{
    jint localport = (*env)->GetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID));

    /* address, family and localport are int fields of iaObj */
    int address, family;

    /* fdObj is the FileDescriptor field on this */
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
    /* fd is an int field on iaObj */
    jint fd;

    struct sockaddr_in him;
    /* The result of the connection */
    int connect = -1;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "inet address argument null.");
	return;
    } else {
	address = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
	family = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_familyID));
    }

    if (localport != 0) {
	/* This socket is bound to a local port. Need to check before
	 * attempting the connection if we are trying connect to our own
	 * addr/port, and throw an exception if so. This mimics the unix
	 * behavior, as VxWorks seems happy to let a socket connect
	 * to itself.
	 */

	int len = sizeof(him);
	if (JVM_GetSockName(fd, (struct sockaddr *)&him, &len) == -1) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return;
	}

	if ((port == ntohs(him.sin_port)) &&
	    (address == ntohl(him.sin_addr.s_addr))) {
	    char buf[128];
	    errno = EADDRINUSE;
	    jio_snprintf(buf, sizeof(buf), "errno: %d, error: %s for fd: %d",
			 errno, strerror(errno), fd);
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", buf);
	    return;
        }
    }

    /* connect */
    memset((char *) &him, 0, sizeof(him));
    him.sin_port = htons((short) port);
    him.sin_addr.s_addr = (unsigned long) htonl(address);
    him.sin_len = (unsigned char) sizeof(him);
    him.sin_family = family;

    connect = JVM_Connect(fd, (struct sockaddr *) &him, sizeof(him));
    if (connect < 0) {
	if (connect == JVM_IO_INTR) {
	    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "operation interrupted");
	} else if (errno == EPROTO) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "ProtocolException",
			    strerror(errno));
	} else if (errno == ECONNREFUSED) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "ConnectException",
			    strerror(errno));
	} else if (errno == ETIMEDOUT || errno == EHOSTUNREACH) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "NoRouteToHostException",
			    strerror(errno));
	} else {
	    char buf[128];
	    jio_snprintf(buf, sizeof(buf), "errno: %d, error: %s for fd: %d",
			 errno, strerror(errno), fd);
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", buf);
	}
	return;
    }
    (*env)->SetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID), fd);

    /* set the remote peer address and port */
    (*env)->SetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_addressID), iaObj);
    (*env)->SetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_portID), port);

    /*
     * we need to initialize the local port field if bind was called
     * previously to the connect (by the client) then localport field
     * will already be initialized
     */
    if (localport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object.
  	 */
	int len = sizeof(him);
	if (JVM_GetSockName(fd, (struct sockaddr *)&him, &len) == -1) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return;
	}
	(*env)->SetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID), ntohs(him.sin_port));
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketBind
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketBind(JNIEnv *env, jobject this,
					 jobject iaObj, jint localport) {

    /* fdObj is the FileDescriptor field on this */
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
    /* fd is an int field on fdObj */
    int fd;

    /* address and family are int fields on iaObj */
    int address, family;

    struct sockaddr_in him;
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "iaObj is null.");
	return;
    } else {
	address = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
	family = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_familyID));
    }
    /* bind */
    memset((char *) &him, 0, sizeof(him));
    him.sin_port = htons((short)localport);
    him.sin_addr.s_addr = (unsigned long) htonl(address);
    him.sin_len = (unsigned char) sizeof(him);
    him.sin_family = family;

    if (JVM_Bind(fd, (struct sockaddr *)&him, sizeof(him)) == -1) {
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

    /* set the address */
    (*env)->SetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_addressID), iaObj);

    /* intialize the local port */
    if (localport == 0) {
	/* Now that we're a connected socket, let's extract the port number
	 * that the system chose for us and store it in the Socket object.
  	 */
	int len = sizeof(him);
	if (JVM_GetSockName(fd, (struct sockaddr *)&him, &len) == -1) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return;
	}
	(*env)->SetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID),
			    ntohs(him.sin_port));
    } else {
	(*env)->SetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID), localport);
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketListen
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketListen (JNIEnv *env, jobject this,
					    jint count)
{
    /* this FileDescriptor fd field */
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
    /* fdObj's int fd field */
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }

    /*
     * Workaround for bugid 4101691 in Solaris 2.6. See 4106600.
     * If listen backlog is Integer.MAX_VALUE then subtract 1.
     */
    if (count == 0x7fffffff)
	count -= 1;

    if (JVM_Listen(fd, count) == JVM_IO_ERR) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
    }
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketAccept
 * Signature: (Ljava/net/SocketImpl;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketAccept(JNIEnv *env, jobject this,
					   jobject socket)
{
    /* fields on this */
    jint port;
    jint timeout = (*env)->GetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_timeoutID));
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));

    /* the FileDescriptor field on socket */
    jobject socketFdObj;
    /* the InetAddress field on socket */
    jobject socketAddressObj;

    /* the fd int field on fdObj */
    jint fd;

    jthrowable error;

    struct sockaddr_in him;
    jint len = sizeof(him);

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    if (IS_NULL(socket)) {
	JNU_ThrowNullPointerException(env, "socket is null");
	return;
    } else {
	socketFdObj = (*env)->GetObjectField(env, socket, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
	socketAddressObj = (*env)->GetObjectField(env, socket, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_addressID));
    }
    if ((IS_NULL(socketAddressObj)) || (IS_NULL(socketFdObj))) {
	JNU_ThrowNullPointerException(env, "socket address or fd obj");
	return;
    }
    if (timeout) {
	int ret = JVM_Timeout(fd, timeout);
        if (ret == 0) {
            JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "Accept timed out");
	    return;
        } else if (ret == JVM_IO_ERR) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    (errno == EBADF) ? "Socket closed" :
			    strerror(errno));
	    return;
	} else if (ret == JVM_IO_INTR) {
	    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "operation interrupted");
	    return;
	}
    }
    fd = JVM_Accept(fd, (struct sockaddr *)&him, &len);
    if (fd < 0) {
        if (fd == -2) {
            JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "operation interrupted");
        } else {
            JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
                        (errno == EBADF) ? "Socket closed" : strerror(errno));
        }
        return;
    }

    /*
     * This is a gross temporary workaround for a bug in libsocket
     * of solaris2.3.  They exit a mutex twice without entering it
     * and our monitor stuff throws an exception.  The workaround is
     * to simply ignore IllegalMonitorStateException from accept.
     * If we get an exception, and it has an exception object, and
     * we can find the class InternalError in the java pakage, and
     * the exception that was thrown was an instance of
     * IllegalMonitorStateException, then ignore it.
     */
    error = (*env)->ExceptionOccurred(env);
    if (!IS_NULL(error)) {
	jclass ilmeCls =
	    (*env)->FindClass(env, "java/lang/IllegalMonitorStateException");
	jclass errorCls = (*env)->GetObjectClass(env, error);
	if (!IS_NULL(ilmeCls) &&
	    (*env)->IsSameObject(env, ilmeCls, errorCls)) {
	    (*env)->ExceptionClear(env);
	}
    }

    (*env)->SetIntField(env, socketFdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID), fd);

    /*
     * fill up the remote peer port and address in the new socket structure
     */
    (*env)->SetIntField(env, socket, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_portID), ntohs(him.sin_port));
    (*env)->SetIntField(env, socketAddressObj, JNI_STATIC(java_net_InetAddress, ia_familyID), him.sin_family);
    (*env)->SetIntField(env, socketAddressObj, JNI_STATIC(java_net_InetAddress, ia_addressID),
			ntohl(him.sin_addr.s_addr));
    /* also fill up the local port information */
    port = (*env)->GetIntField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID));
    (*env)->SetIntField(env, socket, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_localportID), port);
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketAvailable
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainSocketImpl_socketAvailable(JNIEnv *env, jobject this) {

    jint ret = -1;
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
    jint fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    /* JVM_SocketAvailable returns 0 for failure, 1 for success */
    if (!JVM_SocketAvailable(fd, &ret)){
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			strerror(errno));
    }
    return ret;
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketClose(JNIEnv *env, jobject this) {

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
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
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    (*env)->SetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID), -1);
    JVM_SocketClose(fd);
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketShutdown
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketShutdown(JNIEnv *env, jobject this,
					     jint howto) 
{

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
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
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    JVM_SocketShutdown(fd, howto);
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketSetOption
 * Signature: (IZLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketSetOption(JNIEnv *env, jobject this,
					      jint cmd, jboolean on,
					      jobject value) {
    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
    jint fd;

    /* i is java.lang.Integer */
    jclass iCls;
    jfieldID i_valueID;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    if (fd < 0) {
	/* the fd is closed */
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }
    if (cmd == java_net_SocketOptions_TCP_NODELAY) {
	long onl = (long)on;
	/* enable or disable TCP_NODELAY */
	if (JVM_SetSockOpt(fd, JNI_STATIC_MD(java_net_PlainSocketImpl, tcp_level), TCP_NODELAY,
		       (char *)&onl, sizeof(long)) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	}
	return;
    } else if(cmd == java_net_SocketOptions_SO_LINGER) {
	struct linger arg;
	arg.l_onoff = on;

	if(on) {
	    if (IS_NULL(value)) {
		/* trying to enable w/ no timeout */
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			  "invalid parameter setting SO_LINGER");
		return;
	    }
	    iCls = (*env)->FindClass(env, "java/lang/Integer");
	    i_valueID = (*env)->GetFieldID(env, iCls, "value", "I");
	    arg.l_linger = (*env)->GetIntField(env, value, i_valueID);

	    /* VxWorks stores the timeout value in a short. (See struct
	     * socket in net/socketvar.h.) Truncate arg.l_linger as necessary
	     * to fit within a short. */

	    if (arg.l_linger > SHRT_MAX) {
		arg.l_linger = SHRT_MAX;
	    }

	    if(JVM_SetSockOpt(fd, SOL_SOCKET, SO_LINGER,
			  (char*)&arg, sizeof(arg)) < 0) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
				strerror(errno));
		return;
	    }
	} else {
	    /* we want to *disable* SO_LINGER.  This means (I think)
	     * that we want to *enable* SO_DONTLINGER.  But,
	     * SO_DONTLINGER doesn't seem to be recgnized so
	     * just use SO_LINGER w/ arg.l_onoff off (zero).
	     */
	    if (JVM_SetSockOpt(fd, SOL_SOCKET, SO_LINGER,
			   (char*)&arg, sizeof(arg)) < 0) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
				strerror(errno));
		return;
	    }
	}
    } else if (cmd == java_net_SocketOptions_SO_SNDBUF) {
	int arg;
	iCls = (*env)->FindClass(env, "java/lang/Integer");
	i_valueID = (*env)->GetFieldID(env, iCls, "value", "I");
	arg = (*env)->GetIntField(env, value, i_valueID);
	if (arg > max_buffer_size) {
	    arg = max_buffer_size;
	}
	/* Don't throw an exception even if JVM_SetSockOpt() returns -1
	 * since the buffer size is just a hint.
	 */
	JVM_SetSockOpt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&arg, sizeof(arg));
    } else if (cmd == java_net_SocketOptions_SO_RCVBUF) {
	int arg;
	iCls = (*env)->FindClass(env, "java/lang/Integer");
	i_valueID = (*env)->GetFieldID(env, iCls, "value", "I");
	arg = (*env)->GetIntField(env, value, i_valueID);
	if (arg > max_buffer_size) {
	    arg = max_buffer_size;
	}
        /* Don't throw an exception even if JVM_SetSockOpt() returns -1
	 * since the buffer size is just a hint.
	 */
	JVM_SetSockOpt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&arg, sizeof(arg));
    } else if (cmd == java_net_SocketOptions_SO_KEEPALIVE) {
	long onl = (long)on;
	/* enable or disable SO_KEEPALIVE */
	if (JVM_SetSockOpt(fd, SOL_SOCKET, SO_KEEPALIVE,
		       (char *)&onl, sizeof(long)) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	}
	return;
    } else {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		  "Socket option unsupported");
	return;
    }
    return;
}

/*
 * Class:     java_net_PlainSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainSocketImpl_socketGetOption(JNIEnv *env, jobject this,
					      jint opt) {
    /*
     * returning -1 from this method means the option is DISABLED.
     * Any other return value (including 0) is considered an option
     * parameter to be interpreted.
     */

    jobject fdObj = (*env)->GetObjectField(env, this, JNI_STATIC_MD(java_net_PlainSocketImpl, psi_fdID));
    jint fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, JNI_STATIC_MD(java_net_PlainSocketImpl, IO_fd_fdID));
    }
    if(fd < 0) {
	/* the fd is closed */
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    }
    if (opt == java_net_SocketOptions_TCP_NODELAY) {
	/* query for TCP_NODELAY */
	int result, len;
	len = sizeof(int);
	if (JVM_GetSockOpt(fd, JNI_STATIC_MD(java_net_PlainSocketImpl, tcp_level), TCP_NODELAY, (char*)&result, &len) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return -1;
	}
	return (result == 0 ? -1 : 1); /* return of -1 means option is off */
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
	return (him.sin_addr.s_addr == INADDR_ANY) ? -1:
                      ntohl(him.sin_addr.s_addr);
    } else if (opt == java_net_SocketOptions_SO_LINGER) {
	struct linger arg;
	int len = sizeof(struct linger);
	memset(&arg, 0, len);

	if(JVM_GetSockOpt(fd, SOL_SOCKET, SO_LINGER, (char*)&arg, &len) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return -1;
	}
	/* if disabled, return -1, else return the linger time */
	return (arg.l_onoff ? arg.l_linger: -1);
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
    } else if (opt == java_net_SocketOptions_SO_KEEPALIVE) {
	/* query for SO_KEEPALIVE */
	int result, len;
	len = sizeof(int);
	if (JVM_GetSockOpt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&result, &len) < 0) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			    strerror(errno));
	    return -1;
	}
	return (result == 0 ? -1 : 1); /* return of -1 means option is off */
    }


    /* we don't understand the specified option */
    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		    "invalid option");
    return -1;
}

