/*
 * @(#)net_util_md.cpp	1.14 06/10/10
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

#include "jni_util.h"
#include "jvm.h"
#include "net_util.h"

#include "java_net_SocketOptions.h"
#include "java_net_InetAddress.h"
#include "java_net_Inet4Address.h"
#include "java_net_Inet6Address.h"

extern "C" {
#include "jni_statics.h"
}

#include <in_sock.h>

#define ia_addressID \
    JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia4_ctrID \
    JNI_STATIC(java_net_Inet4Address, ia4_ctrID)
#define ia_familyID \
    JNI_STATIC(java_net_InetAddress, ia_familyID)
#define ia_addressID \
    JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia6_ipaddressID \
    JNI_STATIC(java_net_Inet6Address, ia6_ipaddressID)
#define ia6_ctrID \
    JNI_STATIC(java_net_Inet6Address, ia6_ctrID)

int initialized = 0;
void init(JNIEnv *env) {
    if (!initialized) {
        Java_java_net_InetAddress_init(env, 0);
        Java_java_net_Inet4Address_init(env, 0);
        Java_java_net_Inet6Address_init(env, 0);
	initialized = 1;
    }
}

jfieldID
NET_GetFileDescriptorID(JNIEnv *env)
{
    jclass cls = (*env)->FindClass(env, "java/io/FileDescriptor");
    CHECK_NULL_RETURN(cls, NULL);
    return (*env)->GetFieldID(env, cls, "fd", "I");
}

void
NET_InetAddressToSockaddr(JNIEnv *env, jobject iaObj, int port,
    struct sockaddr *him, int * /*len*/)
{
    jint family = (*env)->GetIntField(env, iaObj, ia_familyID);
    if (family == IPv4) {
	jint address = (*env)->GetIntField(env, iaObj, ia_addressID);
	him->ia = TInetAddr(address, port);
    } else {
	jbyteArray ipaddress =
	    (*env)->GetObjectField(env, iaObj, ia6_ipaddressID);
	TIp6Addr ipv6address;
	(*env)->GetByteArrayRegion(env, ipaddress, 0, 16,
	    (jbyte *)ipv6address.u.iAddr8);
	him->ia = TInetAddr(ipv6address, port);
    }
}

jobject
NET_SockaddrToInetAddress(JNIEnv *env, struct sockaddr *him, int *port) {
    jobject iaObj;
    init(env);

    if (him->ia.Family() == KAfInet6 && !him->ia.IsV4Mapped()) {
	jbyteArray ipaddress;
	const TIp6Addr &ipv6address = him->ia.Ip6Address();
	jbyte *caddr = (jbyte *)ipv6address.u.iAddr8;
	static jclass inet6Cls = 0;
	if (inet6Cls == 0) {
	    jclass c = (*env)->FindClass(env, "java/net/Inet6Address");
	    CHECK_NULL_RETURN(c, NULL);
	    inet6Cls = (*env)->NewGlobalRef(env, c);
	    CHECK_NULL_RETURN(inet6Cls, NULL);
	    (*env)->DeleteLocalRef(env, c);
	}
	iaObj = (*env)->NewObject(env, inet6Cls, ia6_ctrID);
	CHECK_NULL_RETURN(iaObj, NULL);
	ipaddress = (*env)->NewByteArray(env, 16);
	CHECK_NULL_RETURN(ipaddress, NULL);
	(*env)->SetByteArrayRegion(env, ipaddress, 0, 16, caddr);

	(*env)->SetObjectField(env, iaObj, ia6_ipaddressID, ipaddress);

	(*env)->SetIntField(env, iaObj, ia_familyID, IPv6);
	*port = him->ia.Port();
    } else {
	static jclass inet4Cls = 0;

	if (inet4Cls == 0) {
	    jclass c = (*env)->FindClass(env, "java/net/Inet4Address");
	    CHECK_NULL_RETURN(c, NULL);
	    inet4Cls = (*env)->NewGlobalRef(env, c);
	    CHECK_NULL_RETURN(inet4Cls, NULL);
	    (*env)->DeleteLocalRef(env, c);
	}
	iaObj = (*env)->NewObject(env, inet4Cls, ia4_ctrID);
	CHECK_NULL_RETURN(iaObj, NULL);
	(*env)->SetIntField(env, iaObj, ia_familyID, IPv4);
	(*env)->SetIntField(env, iaObj, ia_addressID,
	    him->ia.Address());
	*port = him->ia.Port();
    }
    return iaObj;
}

jint
NET_SockaddrEqualsInetAddress(JNIEnv *env, struct sockaddr *him, jobject iaObj)
{
    jint jfamily = (*env)->GetIntField(env, iaObj, ia_familyID);

    {
	int addrNew, addrCur;
	if (jfamily != IPv4) {
	    return JNI_FALSE;
	}
	addrNew = him->ia.Address();
	addrCur = (*env)->GetIntField(env, iaObj, ia_addressID);
	if (addrNew == addrCur) {
	    return JNI_TRUE;
	} else {
	    return JNI_FALSE;
	}
    }
}

int
NET_MapSocketOption(jint cmd, int *level, int *optname)
{
    switch (cmd) {
    case java_net_SocketOptions_SO_RCVBUF:
	*level = KSOLSocket;
	*optname = KSORecvBuf;
	break;
    case java_net_SocketOptions_SO_SNDBUF:
	*level = KSOLSocket;
	*optname = KSOSendBuf;
	break;
    case java_net_SocketOptions_SO_REUSEADDR:
	*level = KSolInetIp;
	*optname = KSoReuseAddr;
	break;
    case java_net_SocketOptions_SO_KEEPALIVE:
	*level = KSolInetTcp;
	*optname = KSoTcpKeepAlive;
	break;
    case java_net_SocketOptions_TCP_NODELAY:
	*level = KSolInetTcp;
	*optname = KSoTcpNoDelay;
	break;
    case java_net_SocketOptions_IP_TOS:
	*level = KSolInetIp;
	*optname = KSoIpTOS;
	break;
    case java_net_SocketOptions_IP_MULTICAST_LOOP:
	*level = KSolInetIp;
	*optname = KSoIp6MulticastLoop;
	break;
    default:
	return -1;
    }
    return 0;
}

extern int SYMBIANsocketServerInit();

extern "C" int
IPv6_supported()
{
    SYMBIANsocketServerInit();
    return TRUE;
}
