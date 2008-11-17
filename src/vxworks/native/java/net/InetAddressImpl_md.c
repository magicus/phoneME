/*
 * @(#)InetAddressImpl_md.c	1.31 06/10/10
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <hostLib.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_InetAddressImpl.h"

/* the initial size of our hostent buffers */
#define HENT_BUF_SIZE 1024
#define BIG_HENT_BUF_SIZE 10240	 /* a jumbo-sized one */

#include "jni_statics.h"

#ifndef __GLIBC__
/* gethostname() is in libc.so but I can't find a header file for it */
extern int gethostname(char *buf, int buf_len);
#endif

/************************************************************************
 * InetAddressImpl
 */

/*
 * Class:     java_net_InetAddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_InetAddressImpl_getLocalHostName(JNIEnv *env, jobject this) {
    char hostname[MAXHOSTNAMELEN+1];

    hostname[0] = '\0';
    if (JVM_GetHostName(hostname, MAXHOSTNAMELEN)) {
	strcpy(hostname, "localhost");
    } else {
#ifdef SOLARIS
	/* Solaris doesn't want to give us a fully qualified domain name.
	 * We do a reverse lookup to try and get one.  This works
	 * if DNS occurs before NIS in /etc/resolv.conf, but fails
	 * if NIS comes first (it still gets only a partial name).
	 * We use thread-safe system calls.
	 */
	struct hostent res, *hp = 0;
	char buf[HENT_BUF_SIZE];
	int h_error=0;

#ifdef __GLIBC__
	gethostnamebyname_r(hostname, &res, buf, sizeof(buf), &hp, &h_error);
#else
	hp = gethostbyname_r(hostname, &res, buf, sizeof(buf), &h_error);
#endif
	if (hp) {
#ifdef __GLIBC__
	    gethostbyaddr_r(hp->h_addr, hp->h_length, AF_INET,
			    &res, buf, sizeof(buf), &hp, &h_error);
#else
	    hp = gethostbyaddr_r(hp->h_addr, hp->h_length, AF_INET,
				 &res, buf, sizeof(buf), &h_error);
#endif
	    if (hp) {
		strcpy(hostname, hp->h_name);
	    }
	}
#endif
    }
    return (*env)->NewStringUTF(env, hostname);
}

/*
 * Class:     java_net_InetAddressImpl
 * Method:    makeAnyLocalAddress
 * Signature: (Ljava/net/InetAddress;)V
 *
 * This doesn't appear to justify its own existence.
 */
JNIEXPORT void JNICALL
Java_java_net_InetAddressImpl_makeAnyLocalAddress(JNIEnv *env, jobject this,
						  jobject iaObj) {
    if (IS_NULL(iaObj)) {
	JNU_ThrowNullPointerException(env, "inet address argument");
    } else {
	(*env)->SetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_addressID), INADDR_ANY);
	(*env)->SetIntField(env, iaObj, JNI_STATIC(java_net_InetAddress, ia_familyID), AF_INET);
    }
}

/*
 * Class:     java_net_InetAddressImpl
 * Method:    getInetFamily
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_java_net_InetAddressImpl_getInetFamily(JNIEnv *env, jobject this)
{
    return AF_INET;
}

/*
 * Find an internet address for a given hostname.  Not this this
 * code only works for addresses of type INET. The translation
 * of %d.%d.%d.%d to an address (int) occurs in java now, so the
 * String "host" shouldn't *ever* be a %d.%d.%d.%d string
 *
 * Class:     java_net_InetAddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 */

JNIEXPORT jobjectArray JNICALL
Java_java_net_InetAddressImpl_lookupAllHostAddr(JNIEnv *env, jobject this,
						jstring host) {
    const char *hostname;
    jobjectArray ret = 0;
    jclass byteArrayCls;
    int ip;

    if (IS_NULL(host)) {
	JNU_ThrowNullPointerException(env, "host is null");
	return 0;
    }
    hostname = JNU_GetStringPlatformChars(env, host, JNI_FALSE);

    ip = hostGetByName((char *)hostname);

    if (ip != ERROR) {
	byteArrayCls = (*env)->FindClass(env, "[B");
        ret = (*env)->NewObjectArray(env, 1, byteArrayCls, NULL);

	if (IS_NULL(ret)) {
	    /* we may have memory to free at the end of this */
	    goto cleanupAndReturn;
	}

	{
	    jbyteArray barray = (*env)->NewByteArray(env, 4);
	    jbyte ipAddr[4];
	    if (IS_NULL(barray)) {
		/* we may have memory to free at the end of this */
	        ret = NULL;
		goto cleanupAndReturn;
	    }

	    /* convert from network order to host order */
	    ip = ntohl(ip);

	    ipAddr[0] = ip >> 24;
	    ipAddr[1] = (ip >> 16) & 0xff;
	    ipAddr[2] = (ip >> 8) & 0xff;
	    ipAddr[3] = ip & 0xff;
	    (*env)->SetByteArrayRegion(env, barray, 0, 4, ipAddr);
	    (*env)->SetObjectArrayElement(env, ret, 0, barray);
	}
    } else {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException",
			(char *)hostname);
	ret = NULL;
    }

cleanupAndReturn:
    JNU_ReleaseStringPlatformChars(env, host, hostname);
    return ret;
}

/*
 * Class:     java_net_InetAddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_InetAddressImpl_getHostByAddr(JNIEnv *env, jobject this,
					    jint addr) {

    jstring ret = NULL;
    char buf[MAXHOSTNAMELEN + 1];
    STATUS status;

    addr = htonl(addr);

    status = hostGetByAddr(addr, buf);

    if (status != OK) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", NULL);
    } else {
	ret = (*env)->NewStringUTF(env, buf);
    }
    return ret;
}
