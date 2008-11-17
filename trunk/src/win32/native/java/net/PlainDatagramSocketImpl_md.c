/*
 * @(#)PlainDatagramSocketImpl_md.c	1.88 06/10/10 
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
#ifndef WINCE
	#include <errno.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <sys/types.h>
#else
        #include "javavm/include/porting/ansi/errno.h"
	#include <winsock.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifndef IPTOS_TOS_MASK
#define IPTOS_TOS_MASK 0x1e
#endif
#ifndef IPTOS_PREC_MASK
#define IPTOS_PREC_MASK 0xe0
#endif 

#include "java_net_PlainDatagramSocketImpl.h"
#include "java_net_SocketOptions.h"
#include "java_net_NetworkInterface.h"

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#ifndef IN_MULTICAST
#define IN_MULTICAST(i)	   IN_CLASSD(i)
#ifndef IN_CLASSD
#define IN_CLASSD(i)	(((i) & 0xf0000000U) == 0xe0000000U)
#endif
#endif

#ifndef SO_MAX_MSG_SIZE
#define SO_MAX_MSG_SIZE 0x2003
#endif

/************************************************************************
 * PlainDatagramSocketImpl
 */

#include "jni_statics.h"
static jfieldID pdsi_trafficClassID;
jfieldID pdsi_fdID;
jfieldID pdsi_timeoutID;

#define STATIC(field) JNI_STATIC_MD(java_net_PlainDatagramSocketImpl, field)
jfieldID pdsi_connected;

#define IO_fd_fdID	 STATIC(IO_fd_fdID)

#define pdsi_fdID	 STATIC(pdsi_fdID)
#define pdsi_timeoutID	 STATIC(pdsi_timeoutID)
#define pdsi_localPortID STATIC(pdsi_localPortID)

#define ia_clazz	 STATIC(ia_clazz)
#define ia4_ctor	 STATIC(ia4_ctor)
#define ia4_class        STATIC(ia4_class)

static CRITICAL_SECTION sizeCheckLock;

/*
 * Returns a java.lang.Integer based on 'i'
*/
jobject createInteger(JNIEnv *env, int i) {
    static jclass i_class;
    static jmethodID i_ctrID;
    static jfieldID i_valueID;

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


//#define ia_addressID	 JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia_familyID	 JNI_STATIC(java_net_InetAddress, ia_familyID)
#define dp_addressID	 JNI_STATIC(java_net_DatagramPacket, dp_addressID)
#define dp_portID	 JNI_STATIC(java_net_DatagramPacket, dp_portID)
#define dp_offsetID	 JNI_STATIC(java_net_DatagramPacket, dp_offsetID)
#define dp_bufID	 JNI_STATIC(java_net_DatagramPacket, dp_bufID)
#define dp_lengthID	 JNI_STATIC(java_net_DatagramPacket, dp_lengthID)

/*
 * the maximum buffer size. Used for setting
 * SendBufferSize and ReceiveBufferSize.
 */
static const int max_buffer_size = 64 * 1024;

/*
 * Returns a java.lang.Boolean based on 'b'
 */
jobject createBoolean(JNIEnv *env, int b) {
    static jclass b_class;
    static jmethodID b_ctrID;
    static jfieldID b_valueID;

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


static int getFD(JNIEnv *env, jobject this) {
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);

    if (fdObj == NULL) {
	return -1;
    }
    return (*env)->GetIntField(env, fdObj, IO_fd_fdID);
}

/*
 * This function returns JNI_TRUE if the datagram size exceeds the underlying 
 * provider's ability to send to the target address. The following OS
 * oddies have been observed :-
 *
 * 1. On Windows 95/98 if we try to send a datagram > 12k to an application 
 *    on the same machine then the send will fail silently.
 *
 * 2. On Windows ME if we try to send a datagram > supported by underlying
 *    provider then send will not return an error.
 * 
 * 3. On Windows NT/2000 if we exceeds the maximum size then send will fail
 *    with WSAEADDRNOTAVAIL. 
 *
 * 4. On Windows 95/98 if we exceed the maximum size when sending to 
 *    another machine then WSAEINVAL is returned.
 *
 */
jboolean exceedSizeLimit(JNIEnv *env, jint fd, jint addr, jint size) 
{
#define DEFAULT_MSG_SIZE	65527
    static jboolean initDone;
    static jboolean is95or98;
    static int maxmsg;

    typedef struct _netaddr  {		/* Windows 95/98 only */
	unsigned long addr;
	struct _netaddr *next;
    } netaddr;
    static netaddr *addrList;
    netaddr *curr;

    /*
     * First time we are called we must determine which OS this is and also
     * get the maximum size supported by the underlying provider.
     *
     * In addition on 95/98 we must enumerate our IP addresses.  
     */
    if (!initDone) {
	EnterCriticalSection(&sizeCheckLock);

	if (initDone) {
	    /* another thread got there first */
	    LeaveCriticalSection(&sizeCheckLock);

	} else {
	    OSVERSIONINFO ver;	   	
	    int len;
	    
	    /*
	     * Step 1: Determine which OS this is.
	     */
	    ver.dwOSVersionInfoSize = sizeof(ver);
	    GetVersionEx(&ver);

	    is95or98 = JNI_FALSE;
	    if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && 
		ver.dwMajorVersion == 4 &&
		(ver.dwMinorVersion == 0 || ver.dwMinorVersion == 10)) {

		is95or98 = JNI_TRUE;
	    }

	    /*
	     * Step 2: Determine the maximum datagram supported by the 
	     * underlying provider. On Windows 95 if winsock hasn't been 
	     * upgraded (ie: unsupported configuration) then we assume 
	     * the default 64k limit.
	     */
	    len = sizeof(maxmsg);
	    if (NET_GetSockOpt(fd, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&maxmsg, &len) < 0) {
		maxmsg = DEFAULT_MSG_SIZE;
	    } 

	    /*
	     * Step 3: On Windows 95/98 then enumerate the IP addresses on
	     * this machine. This is necesary because we need to check if the
	     * datagram is being sent to an application on the same machine.
	     */
/* CDC does not support Windows 95 or 98, only Windows 2000/XP */
	   //   if (is95or98) {
//  		char hostname[255];
//  		struct hostent *hp;

//  		if (JVM_GetHostName(hostname, sizeof(hostname)) == -1) {
//  		    LeaveCriticalSection(&sizeCheckLock);
//  		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Unable to obtain hostname");
//  		    return JNI_TRUE;
//  		}
//  		hp = (struct hostent *)JVM_GetHostByName(hostname);
//  		if (hp != NULL) {
//  		    struct in_addr **addrp = (struct in_addr **) hp->h_addr_list;

//  		    while (*addrp != (struct in_addr *) 0) {
//  			curr = (netaddr *)malloc(sizeof(netaddr));
//  			if (curr == NULL) {
//  			    while (addrList != NULL) {
//  				curr = addrList->next;
//  				free(addrList);
//  				addrList = curr;
//  			    }
//  			    LeaveCriticalSection(&sizeCheckLock);
//  			    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
//  			    return JNI_TRUE;
//  			}
//  			curr->addr = htonl((*addrp)->S_un.S_addr);
//  			curr->next = addrList;
//  			addrList = curr;
//  			addrp++;
//  		    }
//  		}	
//  	    }

	    /*
	     * Step 4: initialization is done so set flag and unlock cs
	     */
	    initDone = JNI_TRUE;
	    LeaveCriticalSection(&sizeCheckLock);
	}
    }

    /*
     * Now examine the size of the datagram :-
     *
     * (a) If exceeds size of service provider return 'false' to indicate that
     *     we exceed the limit.
     * (b) If not 95/98 then return 'true' to indicate that the size is okay.
     * (c) On 95/98 if the size is <12k we are okay.
     * (d) On 95/98 if size > 12k then check if the destination is the current
     *     machine.
     */
    if (size > maxmsg) {	/* step (a) */
	return JNI_TRUE;
    }
    if (!is95or98) {		/* step (b) */
	return JNI_FALSE;
    }
    if (size <= 12280) {	/* step (c) */
	return JNI_FALSE;
    }

    /* step (d) */

    if ((addr & 0x7f000000) == 0x7f000000) {
	return JNI_TRUE;
    }
    curr = addrList;
    while (curr != NULL) {
	if (curr->addr == addr) {
	    return JNI_TRUE;
	}
	curr = curr->next;
    }
    return JNI_FALSE;
}

/*
 * Return JNI_TRUE if this Windows edition supports ICMP Port Unreachable
 */
__inline static jboolean supportPortUnreachable() {
    static jboolean initDone;
    static jboolean portUnreachableSupported;

    if (!initDone) {
	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(ver);
	GetVersionEx(&ver);
	if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion >= 5) {
	    portUnreachableSupported = JNI_TRUE;
	} else {
	    portUnreachableSupported = JNI_FALSE;
	}
	initDone = JNI_TRUE;
    }
    return portUnreachableSupported;
}

/*
 * This function "purges" all outstanding ICMP port unreachable packets
 * outstanding on a socket and returns JNI_TRUE if any ICMP messages
 * have been purged. The rational for purging is to emulate normal BSD 
 * behaviour whereby receiving a "connection reset" status resets the
 * socket.
 */
static jboolean purgeOutstandingICMP(JNIEnv *env, jobject this, jint fd)
{
    jboolean got_icmp = JNI_FALSE;
    char buf[1];
    fd_set tbl;
    struct timeval t = { 0, 0 };
    struct sockaddr_in rmtaddr;
    int addrlen = sizeof(rmtaddr);

    /*
     * A no-op if this OS doesn't support it.
     */
    if (!supportPortUnreachable()) {
	return JNI_FALSE;
    }
    
    /*
     * Peek at the queue to see if there is an ICMP port unreachable. If there
     * is then receive it.
     */
    FD_ZERO(&tbl);
    FD_SET(fd, &tbl);
    while(1) {
	if (select(/*ignored*/fd+1, &tbl, 0, 0, &t) <= 0) {
	    break;
	}   
	if (CVMnetRecvFrom(fd, buf, 1, MSG_PEEK,
			 (struct sockaddr *)&rmtaddr, &addrlen) != JVM_IO_ERR) {	    
	    break;
	}
	if (WSAGetLastError() != WSAECONNRESET) {
	    /* some other error - we don't care here */
	    break;
	}

	CVMnetRecvFrom(fd, buf, 1, 0,  (struct sockaddr *)&rmtaddr, &addrlen);
	got_icmp = JNI_TRUE;
    } 

    return got_icmp;
}


/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_init(JNIEnv *env, jclass cls) {

    /* get fieldIDs */
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

    cls = (*env)->FindClass(env, "java/io/FileDescriptor");
    CHECK_NULL(cls);
    IO_fd_fdID = NET_GetFileDescriptorID(env);
    CHECK_NULL(IO_fd_fdID);

    ia4_class = (*env)->FindClass(env, "java/net/Inet4Address");
    CHECK_NULL(ia4_class);
    ia4_class = (*env)->NewGlobalRef(env, ia4_class);
    CHECK_NULL(ia4_class);
    ia4_ctor = (*env)->GetMethodID(env, ia4_class, "<init>", "()V"); 
    CHECK_NULL(ia4_ctor);

    InitializeCriticalSection(&sizeCheckLock);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    bind
 * Signature: (ILjava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_bind(JNIEnv *env, jobject this,
					   jint port, jobject addressObj) {
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    int fd, family;

    struct sockaddr_in lcladdr;
    int lcladdrlen = sizeof(lcladdr);
    int address;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (IS_NULL(addressObj)) {
	JNU_ThrowNullPointerException(env, "argument address");
	return;
    } else {
	address = (*env)->GetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
    }

    /*
     * Only AF_INET supported on Windows
     */
    family = (*env)->GetIntField(env, addressObj, ia_familyID);
    if (family != IPv4) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Protocol family not supported");
	return;
    }

    /* bind - pick a port number for local addr*/
    memset((char *) &lcladdr, 0, sizeof(lcladdr));
    lcladdr.sin_family      = AF_INET;
    lcladdr.sin_port        = htons((short)port);
    lcladdr.sin_addr.s_addr = htonl(address);
    if (bind(fd, (struct sockaddr *)&lcladdr, sizeof(lcladdr)) == -1) {
	if (WSAGetLastError() == WSAEACCES) {
	    WSASetLastError(WSAEADDRINUSE);
	}
        NET_ThrowCurrent(env, "Cannot bind");
	return;
    }

    /* find what port system picked for us,
       should have told me in the bind call itself */
    if (getsockname(fd, (struct sockaddr *)&lcladdr, &lcladdrlen) == -1) {
	NET_ThrowCurrent(env, "JVM_GetSockName");
	return;
    }
    (*env)->SetIntField(env, this, pdsi_localPortID, ntohs(lcladdr.sin_port));
#ifdef WINCE
	{
	CVMThreadID *tid = CVMthreadSelf();
	tid->tcp_if = 0;
	tid->tcp_ttl = 1;
	}
#endif
}


/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    connect0
 * Signature: (Ljava/net/InetAddress;I)V
 */

JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_connect0(JNIEnv *env, jobject this,
					       jobject address, jint port) {
    /* The object's field */
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    /* The fdObj'fd */
    jint fd;
    /* The packetAddress address, family and port */
    jint addr, family;
    struct sockaddr_in rmtaddr;

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

    addr = (*env)->GetIntField(env, address, JNI_STATIC(java_net_InetAddress, ia_addressID));

    family = (*env)->GetIntField(env, address, ia_familyID);
    if (family != IPv4) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Protocol family not supported");
	return;
    }
    memset(&rmtaddr, 0, sizeof(rmtaddr));
    rmtaddr.sin_port = htons((short)port);
    rmtaddr.sin_addr.s_addr = htonl(addr);
    rmtaddr.sin_family = AF_INET;

    if (connect(fd, (struct sockaddr *)&rmtaddr, sizeof(rmtaddr)) == -1) {
        NET_ThrowCurrent(env, "connect");
	return;
    }

    /*
     * In the future (Windows 2000 SP2 onwards) use SIO_UDP_CONNRESET
     * to enable ICMP port unreachable handling here.
     */
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    disconnect0
 * Signature: ()V
 */

JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_disconnect0(JNIEnv *env, jobject this) {
    /* The object's field */
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    /* The fdObj'fd */
    jint fd;
    struct sockaddr addr;

    if (IS_NULL(fdObj)) {
	/* disconnect doesn't throw any exceptions */
	return;
    }
    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);

    memset(&addr, 0, sizeof(addr));
    addr.sa_family = AF_INET;
    connect(fd, &addr, sizeof(addr));

    /* 
     * In the future (Windows 2000 SP2 onwards) use SIO_UDP_CONNRESET
     * to disable ICMP port unreachable handling here.
     */
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
    char *fullPacket;
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint fd;

    jobject iaObj;
    jint address;
    jint family;

    jint packetBufferOffset, packetBufferLen, packetPort;
    jbyteArray packetBuffer;
    jboolean connected;

    struct sockaddr_in rmtaddr;
    struct sockaddr_in *addrp = &rmtaddr;
    int addrlen = sizeof(rmtaddr);

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    }

    if (IS_NULL(packet)) {
	JNU_ThrowNullPointerException(env, "null packet");
	return;
    }

    fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    iaObj = (*env)->GetObjectField(env, packet, dp_addressID);

    packetPort = (*env)->GetIntField(env, packet, dp_portID);
    packetBufferOffset = (*env)->GetIntField(env, packet, dp_offsetID);
    packetBuffer = (jbyteArray)(*env)->GetObjectField(env, packet, dp_bufID);
    connected = (*env)->GetBooleanField(env, this, pdsi_connected);

    if (IS_NULL(iaObj) || IS_NULL(packetBuffer)) {
	JNU_ThrowNullPointerException(env, "null address || null buffer");
	return;
    }

    packetBufferLen = (*env)->GetIntField(env, packet, dp_lengthID);

    if (connected) {
        address = 0; /* get rid of compiler warning */
	addrp = 0; /* arg to JVM_Sendto () null in this case */
	addrlen = 0;
    } else {
    	address = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
	/* We only handle IPv4 for now. Will support IPv6 once its in the os */
	family = AF_INET;

    	rmtaddr.sin_port = htons((short)packetPort);
    	rmtaddr.sin_addr.s_addr = htonl(address);
    	rmtaddr.sin_family = family;
    }

    if (packetBufferLen > MAX_BUFFER_LEN) {

	/*
	 * On 95/98 if we try to send a datagram >12k to an application
	 * on the same machine then this will fail silently. Thus we
	 * catch this situation here so that we can throw an exception
	 * when this arises.
	 * On ME if we try to send a datagram with a size greater than
	 * that supported by the service provider then no error is
	 * returned.
	 */
	if (connected) {
	    address = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
	}
	
	if (exceedSizeLimit(env, fd, address, packetBufferLen)) {
	    if (!((*env)->ExceptionOccurred(env))) {
		NET_ThrowNew(env, WSAEMSGSIZE, "Datagram send failed");
	    }
	    return;
	}

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
	 */
	fullPacket = (char *)malloc(packetBufferLen);
	if (!fullPacket) {
	    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
	    return;
	}
    } else {
	fullPacket = &(BUF[0]);
    }

    (*env)->GetByteArrayRegion(env, packetBuffer, packetBufferOffset, packetBufferLen,
			       (jbyte *)fullPacket);

    switch (JVM_SendTo(fd, fullPacket, packetBufferLen, 0,
		       (struct sockaddr *)addrp, addrlen)) {

        case JVM_IO_ERR:
	    NET_ThrowCurrent(env, "Datagram send failed");
	    break;
	    
        case JVM_IO_INTR:
	    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			    "operation interrupted");
    }

    if (packetBufferLen > MAX_BUFFER_LEN) {
	free(fullPacket);
    }
}

#define ssize_t int
/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    peek
 * Signature: (Ljava/net/InetAddress;)I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_peek(JNIEnv *env, jobject this,
					   jobject addressObj) {

    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint timeout = (*env)->GetIntField(env, this, pdsi_timeoutID);
    jint fd;

    /* The address and family fields of addressObj */
    jint address, family;

	ssize_t n;
	struct sockaddr_in remote_addr;
	jint remote_addrsize = sizeof (remote_addr);
	char buf[1];
    BOOL retry;
    jlong prevTime = 0;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
        if (fd < 0) {
           JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
                           "socket closed");
           return -1;
        }
    }
    if (IS_NULL(addressObj)) {
	JNU_ThrowNullPointerException(env, "Null address in peek()");
    } else {
	address = (*env)->GetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
	/* We only handle IPv4 for now. Will support IPv6 once its in the os */
	family = AF_INET;
    }    

    do {
	retry = FALSE;

	/* 
	 * If a timeout has been specified then we select on the socket
	 * waiting for a read event or a timeout.
	 */
	if (timeout) {
	    int ret;
	    prevTime = JVM_CurrentTimeMillis(env, 0);
	    ret = NET_Timeout (fd, timeout);
	    if (ret == 0) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"Peek timed out");
		return ret;
	    } else if (ret == JVM_IO_ERR) {
		NET_ThrowCurrent(env, "timeout in datagram socket peek");
		return ret;
	    } else if (ret == JVM_IO_INTR) {
		JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
				"operation interrupted");
		return ret;
	    }
	}

	/* now try the peek */
	n = CVMnetRecvFrom(fd, buf, 1, MSG_PEEK,
			 (struct sockaddr *)&remote_addr, &remote_addrsize);

	if (n == JVM_IO_ERR) {
	    if (WSAGetLastError() == WSAECONNRESET) {
		jboolean connected;

		/*
		 * An icmp port unreachable - we must receive this as Windows
		 * does not reset the state of the socket until this has been
		 * received.
		 */
		purgeOutstandingICMP(env, this, fd);

		connected =  (*env)->GetBooleanField(env, this, pdsi_connected);
		if (connected) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "PortUnreachableException",
		 		       "ICMP Port Unreachable");
		    return 0;
		}

		/*
		 * If a timeout was specified then we need to adjust it because
		 * we may have used up some of the timeout befor the icmp port
		 * unreachable arrived.
		 */
		if (timeout) {
		    jlong newTime = JVM_CurrentTimeMillis(env, 0);
		    timeout -= (newTime - prevTime);
		    if (timeout <= 0) {
			JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"Receive timed out");
			return 0;
		    }
		    prevTime = newTime;
		}

		/* Need to retry the recv */
		retry = TRUE;
	    }
	}
    } while (retry);

    if (n == JVM_IO_ERR && WSAGetLastError() != WSAEMSGSIZE) {
	NET_ThrowCurrent(env, "Datagram peek failed");
        return 0;
    }
    if (n == JVM_IO_INTR) {
        JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException", 0);
	return 0;
    }
    (*env)->SetIntField(env, addressObj, JNI_STATIC(java_net_InetAddress, ia_addressID), ntohl(remote_addr.sin_addr.s_addr));
    (*env)->SetIntField(env, addressObj, ia_familyID, IPv4);

    /* return port */
    return ntohs(remote_addr.sin_port);
}

JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_peekData(JNIEnv *env, jobject this,
					   jobject packet) {

     char BUF[MAX_BUFFER_LEN];
    char *fullPacket;
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint timeout = (*env)->GetIntField(env, this, pdsi_timeoutID);

    jbyteArray packetBuffer;
    jint packetBufferOffset, packetBufferLen;

    int fd, errorCode;
    jbyteArray data;

    int datalen;
    int n;
    struct sockaddr_in remote_addr;
    jint remote_addrsize = sizeof (remote_addr);
    BOOL retry;
    jlong prevTime = 0;

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
	 */
	fullPacket = (char *)malloc(packetBufferLen);
	if (!fullPacket) {
	    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
	    return -1;
	}
    } else {
	fullPacket = &(BUF[0]);
    }

    do {
	retry = FALSE;

	/* 
	 * If a timeout has been specified then we select on the socket
	 * waiting for a read event or a timeout.
	 */
	if (timeout) {
	    int ret;

	    if (prevTime == 0) {
		prevTime = JVM_CurrentTimeMillis(env, 0);
	    }

	    ret = NET_Timeout (fd, timeout);
	    
	    if (ret <= 0) {
		if (ret == 0) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				    "Receive timed out");
		} else if (ret == JVM_IO_ERR) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
				    "Socket closed");
		} else if (ret == JVM_IO_INTR) {
		    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
				    "operation interrupted");
		}
	        if (packetBufferLen > MAX_BUFFER_LEN) {
		    free(fullPacket);
		}
		return -1;
	    }
	}

	/* receive the packet */
	n = CVMnetRecvFrom(fd, fullPacket, packetBufferLen, MSG_PEEK,
			 (struct sockaddr *)&remote_addr, &remote_addrsize);

	if (n == JVM_IO_ERR) {
	    if (WSAGetLastError() == WSAECONNRESET) {
		jboolean connected;

		/*
		 * An icmp port unreachable - we must receive this as Windows
		 * does not reset the state of the socket until this has been
		 * received.
		 */
		purgeOutstandingICMP(env, this, fd);

		connected = (*env)->GetBooleanField(env, this, pdsi_connected);
		if (connected) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "PortUnreachableException",
		 		       "ICMP Port Unreachable");

		    if (packetBufferLen > MAX_BUFFER_LEN) {
			free(fullPacket);
		    }
		    return -1;
		}

		/*
		 * If a timeout was specified then we need to adjust it because
		 * we may have used up some of the timeout befor the icmp port
		 * unreachable arrived.
		 */
		if (timeout) {
		    jlong newTime = JVM_CurrentTimeMillis(env, 0);
		    timeout -= (newTime - prevTime);
		    if (timeout <= 0) {
			JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"Receive timed out");
			if (packetBufferLen > MAX_BUFFER_LEN) {
			    free(fullPacket);
			}
			return -1;
		    }
		    prevTime = newTime;
		}
		retry = TRUE;
	    }
	}
    } while (retry);

    /* truncate the data if the packet's length is too small */
    if (n > packetBufferLen) {
	n = packetBufferLen;
    }
    if (n < 0) {
	errorCode = WSAGetLastError();
	/* check to see if it's because the buffer was too small */
	if (errorCode == WSAEMSGSIZE) {
	    /* it is because the buffer is too small. It's UDP, it's
	     * unreliable, it's all good. discard the rest of the
	     * data..
	     */
	    n = packetBufferLen;
	} else {
	    /* failure */
	    (*env)->SetIntField(env, packet, dp_lengthID, 0);
	}
    }
    if (n == -1) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "socket closed");
    } else if (n == -2) {
	JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			"operation interrupted");
    } else if (n < 0) {
	NET_ThrowCurrent(env, "Datagram receive failed");
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
	packetAddress = (*env)->GetObjectField(env, packet, dp_addressID);
	if (packetAddress != NULL) {
	    int currAddress;
	    int currFamily;
	    currAddress = (*env)->GetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID));
	    /* We only handle IPv4 for now. 
	     * Will support IPv6 once its in the os */
	    currFamily = AF_INET;
	    
	    if ((currAddress != newAddress) || (currFamily != newFamily)) {
		/* force a new InetAddress to be created */
	        packetAddress = NULL;
	    }
	}
	if (packetAddress == NULL) {
	    packetAddress = (*env)->NewObject(env, ia4_class, ia4_ctor);
	    CHECK_NULL_RETURN(packetAddress, -1); 
	    (*env)->SetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID), newAddress);
	    (*env)->SetIntField(env, packetAddress, ia_familyID, IPv4);

	    /* stuff the new Inetaddress in the packet */
	    (*env)->SetObjectField(env, packet, dp_addressID, packetAddress);
	}

	/* populate the packet */
	(*env)->SetByteArrayRegion(env, packetBuffer, packetBufferOffset, n,
				   (jbyte *)fullPacket);
	(*env)->SetIntField(env, packet, dp_portID, ntohs(remote_addr.sin_port));
	(*env)->SetIntField(env, packet, dp_lengthID, n);
    }
    if (packetBufferLen > MAX_BUFFER_LEN) {
	free(fullPacket);
    }
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
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint timeout = (*env)->GetIntField(env, this, pdsi_timeoutID);

    jbyteArray packetBuffer;
    jint packetBufferOffset, packetBufferLen;

    int fd, errorCode;
    jbyteArray data;

    int datalen;
    int n;
    struct sockaddr_in remote_addr;
    jint remote_addrsize = sizeof (remote_addr);
    BOOL retry;
    jlong prevTime = 0;
    jboolean connected;

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
	 */
	fullPacket = (char *)malloc(packetBufferLen);
	if (!fullPacket) {
	    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
	    return;
	}
    } else {
	fullPacket = &(BUF[0]);
    }

    

    /*
     * If this Windows edition supports ICMP port unreachable and if we
     * are not connected then we need to know if a timeout has been specified
     * and if so we need to pick up the current time. These are required in
     * order to implement the semantics of timeout, viz :-
     * timeout set to t1 but ICMP port unreachable arrives in t2 where
     * t2 < t1. In this case we must discard the ICMP packets and then 
     * wait for the next packet up to a maximum of t1 minus t2.
     */
    connected = (*env)->GetBooleanField(env, this, pdsi_connected);
    if (supportPortUnreachable() && !connected && timeout) {
	prevTime = JVM_CurrentTimeMillis(env, 0);
    }

    if (timeout) {
	int ret = NET_Timeout(fd, timeout);
	if (ret <= 0) {
	    if (ret == 0) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
				"Receive timed out");
	    } else if (ret == JVM_IO_ERR) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
				"Socket closed");
	    } else if (ret == JVM_IO_INTR) {
		JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
				"operation interrupted");
	    }
	    if (packetBufferLen > MAX_BUFFER_LEN) {
		free(fullPacket);
	    }
	    return;
	}
    }   

    /*
     * Loop only if we discarding ICMP port unreachable packets
     */
    do {
	retry = FALSE;

	/* receive the packet */
	n = CVMnetRecvFrom(fd, fullPacket, packetBufferLen, 0,
			 (struct sockaddr *)&remote_addr, &remote_addrsize);

	if (n == JVM_IO_ERR) {
	    if (WSAGetLastError() == WSAECONNRESET) {
		/*
		 * An icmp port unreachable has been received - consume any other
		 * outstanding packets.
		 */
		purgeOutstandingICMP(env, this, fd);

		/*
		 * If connected throw a PortUnreachableException
		 */
		
		if (connected) {
		    JNU_ThrowByName(env, JNU_JAVANETPKG "PortUnreachableException",
		 		       "ICMP Port Unreachable");

		    if (packetBufferLen > MAX_BUFFER_LEN) {
			free(fullPacket);
		    }
		    
		    return;
		}

		/*
		 * If a timeout was specified then we need to adjust it because
		 * we may have used up some of the timeout before the icmp port
		 * unreachable arrived.
		 */
		if (timeout) {
		    int ret;
		    jlong newTime = JVM_CurrentTimeMillis(env, 0);
		    timeout -= (newTime - prevTime);
		    prevTime = newTime;

		    if (timeout <= 0) {
			ret = 0;
		    } else {
			ret = NET_Timeout(fd, timeout);
		    }

		    if (ret <= 0) {
			if (ret == 0) {
			    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketTimeoutException",
					    "Receive timed out");
			} else if (ret == JVM_IO_ERR) {
			    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
					    "Socket closed");
			} else if (ret == JVM_IO_INTR) {
			    JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
					    "operation interrupted");
			}
			if (packetBufferLen > MAX_BUFFER_LEN) {
			    free(fullPacket);
			}
			return;
		    }
		}

		/*
		 * An ICMP port unreachable was received but we are
		 * not connected so ignore it.
		 */
		retry = TRUE;
	    }
	}
    } while (retry);

    /* truncate the data if the packet's length is too small */
    if (n > packetBufferLen) {
	n = packetBufferLen;
    }
    if (n < 0) {
	errorCode = WSAGetLastError();
	/* check to see if it's because the buffer was too small */
	if (errorCode == WSAEMSGSIZE) {
	    /* it is because the buffer is too small. It's UDP, it's
	     * unreliable, it's all good. discard the rest of the
	     * data..
	     */
	    n = packetBufferLen;
	} else {
	    /* failure */
	    (*env)->SetIntField(env, packet, dp_lengthID, 0);
	}
    }
    if (n == -1) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "socket closed");
    } else if (n == -2) {
	JNU_ThrowByName(env, JNU_JAVAIOPKG "InterruptedIOException",
			"operation interrupted");
    } else if (n < 0) {
	NET_ThrowCurrent(env, "Datagram receive failed");
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
	packetAddress = (*env)->GetObjectField(env, packet, dp_addressID);
	if (packetAddress != NULL) {
	    int currAddress;
	    int currFamily;
	    currAddress = (*env)->GetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID));
	    /* We only handle IPv4 for now. 
	     * Will support IPv6 once its in the os */
	    currFamily = AF_INET;
	    
	    if ((currAddress != newAddress) || (currFamily != newFamily)) {
		/* force a new InetAddress to be created */
	        packetAddress = NULL;
	    }
	}
	if (packetAddress == NULL) {
	    packetAddress = (*env)->NewObject(env, ia4_class, ia4_ctor);
	    CHECK_NULL(packetAddress);
	    (*env)->SetIntField(env, packetAddress, JNI_STATIC(java_net_InetAddress, ia_addressID), newAddress);
	    (*env)->SetIntField(env, packetAddress, ia_familyID, IPv4);

	    /* stuff the new Inetaddress in the packet */
	    (*env)->SetObjectField(env, packet, dp_addressID, packetAddress);
	}

	/* populate the packet */
	(*env)->SetByteArrayRegion(env, packetBuffer, packetBufferOffset, n,
				   (jbyte *)fullPacket);
	(*env)->SetIntField(env, packet, dp_portID,
			    ntohs(remote_addr.sin_port));
	(*env)->SetIntField(env, packet, dp_lengthID, n);
    }
    if (packetBufferLen > MAX_BUFFER_LEN) {
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
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    int fd;

    int arg = -1;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "Socket closed");
	return;
    } else {
	fd =  (int) socket (AF_INET, SOCK_DGRAM, 0);
    }
    if (fd == JVM_IO_ERR) {
	NET_ThrowCurrent(env, "Socket creation failed");
	return;
    }
    {
    BOOL t = TRUE;
    NET_SetSockOpt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&t, sizeof(BOOL));
    }
    (*env)->SetIntField(env, fdObj, IO_fd_fdID, fd);


    /* 
     * In the future (Windows 2000 SP2 onwards) use SIO_UDP_CONNRESET
     * to disable ICMP port unreachable handling here. By default on
     * Windows 2000 an ICMP port unreachable packet will cause a 
     * connection reset.
     */
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    datagramSocketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketClose(JNIEnv *env,
							  jobject this) {
    /*
     * TODO: PUT A LOCK AROUND THIS CODE
     */
    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    int fd;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
        fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    JVM_SocketClose(fd);
    (*env)->SetIntField(env, fdObj, IO_fd_fdID, -1);
}

/*
 * datagramsocket options
 */

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    socketSetOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_socketSetOption(JNIEnv *env,jobject this,
						      jint opt,jobject value) {

    int fd;
    int level, optname, optlen;
    union {
        int i;
	char c;
    } optval;

    fd = getFD(env, this);
    if (fd < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "socket closed");
	return;
    }

    if ((opt == java_net_SocketOptions_IP_MULTICAST_IF) ||
	(opt == java_net_SocketOptions_IP_MULTICAST_IF2)) {

	static jfieldID ia_addressID;
	struct in_addr in;

	if (ia_addressID == NULL) {
	    jclass c = (*env)->FindClass(env,"java/net/InetAddress");
	    CHECK_NULL(c);
	    ia_addressID = (*env)->GetFieldID(env, c, "address", "I");
	    CHECK_NULL(ia_addressID);
	}

	/*
	 * For IP_MULTICAST_IF value is of type NetworkInterface so we
	 * extract addrs[0]
	 */
	if (opt == java_net_SocketOptions_IP_MULTICAST_IF2) {
	    static jfieldID ni_addrsID;
	    jobjectArray addrArray;
	    jsize len;
	    jobject addr;

	    if (ni_addrsID == NULL) {
		jclass c = (*env)->FindClass(env, "java/net/NetworkInterface");
		CHECK_NULL(c);
		ni_addrsID = (*env)->GetFieldID(env, c, "addrs", 
						"[Ljava/net/InetAddress;");
		CHECK_NULL(ni_addrsID);
	    }
	    addrArray = (*env)->GetObjectField(env, value, ni_addrsID);
	    len = (*env)->GetArrayLength(env, addrArray);

	    /*
 	     * Check that there is at least one address bound to this
	     * interface.
	     */
	    if (len < 1) {
		JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		    "bad argument for IP_MULTICAST_IF2: No IP addresses bound to interface");
		return;
	    }	   
	    value = (*env)->GetObjectArrayElement(env, addrArray, 0);
	}

	in.s_addr = htonl((*env)->GetIntField(env, value, ia_addressID));
	if (NET_SetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		           (char*)&in, sizeof(in)) < 0) {
	    NET_ThrowCurrent(env, "setsockopt IP_MULTICAST_IF");
	    return;
	}

	return;
    }

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
            }
	    break;

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
		optval.i = (on ? 1 : 0);

		/* 
		 * setLoopbackMode(true) disables IP_MULTICAST_LOOP rather
		 * than enabling it.
		 */
		if (opt == java_net_SocketOptions_IP_MULTICAST_LOOP) {
		    optval.i = !optval.i;
		}

		optlen = sizeof(optval.i);
	    }
	    break;

	default :
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
		"Socket option not supported by PlainDatagramSocketImp");
	    return;

    }

    if (NET_SetSockOpt(fd, level, optname, (void *)&optval, optlen) < 0) {
        NET_ThrowCurrent(env, "setsockopt");
	return;
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
Java_java_net_PlainDatagramSocketImpl_socketGetOption(JNIEnv *env, jobject this,
						      jint opt) {
    
    int fd;
    int level, optname, optlen;
    union {
        int i;
    } optval;

    fd = getFD(env, this);
    if (fd < 0) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return NULL;
    }

    if (opt == java_net_SocketOptions_IP_MULTICAST_IF
	|| opt == java_net_SocketOptions_IP_MULTICAST_IF2) {

	static jclass inet4_class;
	static jmethodID inet4_ctrID;
	static jfieldID inet4_addrID;

	static jclass ni_class;
	static jmethodID ni_ctrID;
	static jfieldID ni_indexID;
	static jfieldID ni_addrsID;

        jobjectArray addrArray;
        jobject addr;
	jobject ni;

        struct in_addr in;
        int len = sizeof(struct in_addr);

        if (NET_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
                           (char *)&in, &len) < 0) {
            NET_ThrowCurrent(env, "get IP_MULTICAST_IF failed");
            return NULL;
        }

	/*
	 * Construct and populate an Inet4Address
	 */
	if (inet4_class == NULL) {
	    jclass c = (*env)->FindClass(env, "java/net/Inet4Address");
	    CHECK_NULL_RETURN(c, NULL);
	    inet4_ctrID = (*env)->GetMethodID(env, c, "<init>", "()V");
	    CHECK_NULL_RETURN(inet4_ctrID, NULL);
	    inet4_addrID = (*env)->GetFieldID(env, c, "address", "I");
	    CHECK_NULL_RETURN(inet4_addrID, NULL);
	    inet4_class = (*env)->NewGlobalRef(env, c);
	    CHECK_NULL_RETURN(inet4_class, NULL);
	}
	addr = (*env)->NewObject(env, inet4_class, inet4_ctrID, 0);
	CHECK_NULL_RETURN(addr, NULL);
        (*env)->SetIntField(env, addr, inet4_addrID, ntohl(in.s_addr));

	/*
	 * For IP_MULTICAST_IF return InetAddress
	 */
	if (opt == java_net_SocketOptions_IP_MULTICAST_IF) {
	    return addr;
	}

	/* 
	 * For IP_MULTICAST_IF2 we get the NetworkInterface for
	 * this address and return it
	 */
	if (ni_class == NULL) {
	    jclass c = (*env)->FindClass(env, "java/net/NetworkInterface");
	    CHECK_NULL_RETURN(c, NULL);
	    ni_ctrID = (*env)->GetMethodID(env, c, "<init>", "()V");
	    CHECK_NULL_RETURN(ni_ctrID, NULL);
	    ni_indexID = (*env)->GetFieldID(env, c, "index", "I");
	    CHECK_NULL_RETURN(ni_indexID, NULL);
	    ni_addrsID = (*env)->GetFieldID(env, c, "addrs", 
					    "[Ljava/net/InetAddress;");
	    CHECK_NULL_RETURN(ni_addrsID, NULL);
	    ni_class = (*env)->NewGlobalRef(env, c);
	    CHECK_NULL_RETURN(ni_class, NULL);
	}
        ni = Java_java_net_NetworkInterface_getByInetAddress0(env, ni_class, addr);
	if (ni) {
	    return ni;
	}

	/*
	 * The address doesn't appear to be bound at any known
	 * NetworkInterface. Therefore we construct a NetworkInterface
	 * with this address.
	 */
	ni = (*env)->NewObject(env, ni_class, ni_ctrID, 0);
	CHECK_NULL_RETURN(ni, NULL);

	(*env)->SetIntField(env, ni, ni_indexID, -1);
        addrArray = (*env)->NewObjectArray(env, 1, inet4_class, NULL);
	CHECK_NULL_RETURN(addrArray, NULL);
        (*env)->SetObjectArrayElement(env, addrArray, 0, addr);
        (*env)->SetObjectField(env, ni, ni_addrsID, addrArray);
	return ni;
    } 
    
    if (opt == java_net_SocketOptions_SO_BINDADDR) {
	/* find out local IP address, return as 32 bit value */
	static jclass inet4_class;
	static jmethodID inet4_ctrID;
	static jfieldID inet4_addrID;

	struct sockaddr_in him;
	jobject iaObj;
	jclass iaCntrClass;
	jfieldID iaFieldID;
	jclass inet4Cls = (*env)->FindClass(env, "java/net/Inet4Address");
	int len = sizeof(struct sockaddr_in);

	CHECK_NULL_RETURN(inet4Cls, NULL);

	memset(&him, 0, len);
	if (getsockname(fd, (struct sockaddr *)&him, &len) == -1) {
	    NET_ThrowCurrent(env, "getsockname");
	    return NULL;
	}

	if (inet4_class == NULL) {
	    jclass c = (*env)->FindClass(env, "java/net/Inet4Address");
	    CHECK_NULL_RETURN(c, NULL);
	    inet4_ctrID = (*env)->GetMethodID(env, c, "<init>", "()V");
	    CHECK_NULL_RETURN(inet4_ctrID, NULL);
	    inet4_addrID = (*env)->GetFieldID(env, c, "address", "I");
	    CHECK_NULL_RETURN(inet4_addrID, NULL);
	    inet4_class = (*env)->NewGlobalRef(env, c);
	    CHECK_NULL_RETURN(inet4_class, NULL);
	}
	iaObj = (*env)->NewObject(env, inet4_class, inet4_ctrID, 0);
	CHECK_NULL_RETURN(iaObj, NULL);
	(*env)->SetIntField(env, iaObj, inet4_addrID, htonl(him.sin_addr.s_addr));
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

    optlen = sizeof(optval.i);
    if (NET_GetSockOpt(fd, level, optname, (void *)&optval, &optlen) < 0) {
        char errmsg[255];
        sprintf(errmsg, "error getting socket option: %s\n", strerror(errno));
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", errmsg);
        return NULL;
    }

    switch (opt) {
	case java_net_SocketOptions_SO_BROADCAST:
	case java_net_SocketOptions_SO_REUSEADDR:
	    return createBoolean(env, optval.i);

	case java_net_SocketOptions_IP_MULTICAST_LOOP:
	    /* getLoopbackMode() returns true if IP_MULTICAST_LOOP is disabled */
	    return createBoolean(env, !optval.i);

	case java_net_SocketOptions_SO_SNDBUF:
 	case java_net_SocketOptions_SO_RCVBUF:
	case java_net_SocketOptions_IP_TOS:
	    return createInteger(env, optval.i);

	default :
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
		"Socket option not supported by PlainDatagramSocketImpl");
	    return NULL;

    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    setTimeToLive
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTimeToLive(JNIEnv *env, jobject this,
						    jint ttl) {

    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    int fd;
    /* it is important to cast this to a char, otherwise setsockopt gets confused */
    int ittl = (int)ttl;

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }

#ifdef WINCE
	{
	CVMThreadID *tid = CVMthreadSelf();
	tid->tcp_ttl = ittl;
	}
#endif
    /* setsockopt to be correct ttl */
    if (NET_SetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ittl,
		   sizeof (ittl)) < 0) {
	NET_ThrowCurrent(env, "set IP_MULTICAST_TTL failed");
    }
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    setTTL
 * Signature: (B)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTTL(JNIEnv *env, jobject this,
					     jbyte ttl) {
    Java_java_net_PlainDatagramSocketImpl_setTimeToLive(env, this, 
							(jint)ttl & 0xFF);
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    getTimeToLive
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_java_net_PlainDatagramSocketImpl_getTimeToLive(JNIEnv *env, jobject this) {

    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint fd = -1;

    int ttl = 0;
    int len = sizeof(ttl);
    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return -1;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    /* getsockopt of ttl */
    if (NET_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, &len) < 0) {
	NET_ThrowCurrent(env, "get IP_MULTICAST_TTL failed");
	return -1;
    }
#ifdef WINCE
	{
	CVMThreadID *tid = CVMthreadSelf();
	ttl = tid->tcp_ttl;
	}
#endif
    return (jint)ttl;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    getTTL
 * Signature: ()B
 */
JNIEXPORT jbyte JNICALL
Java_java_net_PlainDatagramSocketImpl_getTTL(JNIEnv *env, jobject this) {
    int result = Java_java_net_PlainDatagramSocketImpl_getTimeToLive(env, this);

    return (jbyte)result;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    join
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_join(JNIEnv *env, jobject this,
					   jobject iaObj, jobject niObj) {

    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint fd, address;

    struct sockaddr_in name;
    struct ip_mreq mname;

    struct in_addr in;
    int len = sizeof(struct in_addr);

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "address");
	return;
    } else {
	address = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
    }


    /* Set the multicast group address in the ip_mreq field
     * eventually this check should be done by the security manager
     */
    mname.imr_multiaddr.s_addr = htonl((*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID)));
    if (!IN_MULTICAST(address)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "not in multicast");
    }
    if (IS_NULL(niObj)) {
	if (NET_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&in, &len) < 0) {
	    NET_ThrowCurrent(env, "get IP_MULTICAST_IF failed");
	    return;
	}
	mname.imr_interface.s_addr = in.s_addr;
    } else {
	static jfieldID ni_addrsID;
	jobjectArray addrArray;
	jobject addr;

 	if (ni_addrsID == NULL) {
    	    jclass c = (*env)->FindClass(env, "java/net/NetworkInterface");
	    CHECK_NULL(c);
            ni_addrsID = (*env)->GetFieldID(env, c, "addrs",
                                            "[Ljava/net/InetAddress;");
	    CHECK_NULL(ni_addrsID);
	}

	addrArray = (*env)->GetObjectField(env, niObj, ni_addrsID); 
	addr = (*env)->GetObjectArrayElement(env, addrArray, 0);
	mname.imr_interface.s_addr = htonl((*env)->GetIntField(env, addr, JNI_STATIC(java_net_InetAddress, ia_addressID)));
    }
    
    /* Join the multicast group */
    if (NET_SetSockOpt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mname,
		       sizeof (mname)) < 0) {
	if (WSAGetLastError() == WSAENOBUFS) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
		"IP_ADD_MEMBERSHIP failed (out of hardware filters?)");
	} else {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException","error setting options");
	}
    }

#ifdef WINCE
    /* Fix for 6589699, interface addr get overwritten inside winsock */
    if (NET_SetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&mname.imr_interface.s_addr, sizeof(in)) < 0) {
	JNU_ThrowByName(env,
	    JNU_JAVANETPKG "SocketException","error setting options");
	return;
    }
#endif
    return;
}

/*
 * Class:     java_net_PlainDatagramSocketImpl
 * Method:    leave
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_leave(JNIEnv *env, jobject this,
					    jobject iaObj, jobject niObj) {

    jobject fdObj = (*env)->GetObjectField(env, this, pdsi_fdID);
    jint fd, addressAddress;

    struct sockaddr_in name;
    struct ip_mreq mname;

    struct in_addr in;
    int len = sizeof(struct in_addr);

    if (IS_NULL(fdObj)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException",
			"Socket closed");
	return;
    } else {
	fd = (*env)->GetIntField(env, fdObj, IO_fd_fdID);
    }
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "address argument");
	return;
    } else {
	addressAddress = (*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID));
    }
    /*
     * Set the multicast group address in the ip_mreq field
     * eventually this check should be done by the security manager
     */
    mname.imr_multiaddr.s_addr = htonl((*env)->GetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID)));
    if (!IN_MULTICAST(addressAddress)) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", "not multicast address");
    }
    if (IS_NULL(niObj)) {
	if (NET_GetSockOpt(fd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&in, &len) < 0) {
	    NET_ThrowCurrent(env, "get IP_MULTICAST_IF failed");
	    return;
	}
	mname.imr_interface.s_addr = in.s_addr;
    } else {
        static jfieldID ni_addrsID;
	jobjectArray addrArray;
	jobject addr;

        if (ni_addrsID == NULL) {
            jclass c = (*env)->FindClass(env, "java/net/NetworkInterface");
	    CHECK_NULL(c);
            ni_addrsID = (*env)->GetFieldID(env, c, "addrs",
                                            "[Ljava/net/InetAddress;");
	    CHECK_NULL(ni_addrsID);
        }

	addrArray = (*env)->GetObjectField(env, niObj, ni_addrsID); 
	addr = (*env)->GetObjectArrayElement(env, addrArray, 0);
	mname.imr_interface.s_addr = htonl((*env)->GetIntField(env, addr, JNI_STATIC(java_net_InetAddress, ia_addressID)));
    }

    /* Leave the multicast group */
    if (NET_SetSockOpt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,(char *) &mname,
		       sizeof (mname)) < 0) {
	NET_ThrowCurrent(env, "set IP_DROP_MEMBERSHIP failed");
    }

    return;
}

