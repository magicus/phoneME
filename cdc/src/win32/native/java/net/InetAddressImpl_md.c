/*
 * @(#)InetAddressImpl_md.c	1.9 06/10/10
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
#else
	#include "javavm/include/porting/ansi/errno.h"
	#include <winsock.h>
#endif

#include <string.h>
#include <stdlib.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_InetAddressImpl.h"

#include "jni_statics.h"

#define ia_addressID	JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia_familyID	JNI_STATIC(java_net_InetAddress, ia_familyID)

/* the initial size of our hostent buffers */
#define HENT_BUF_SIZE 1024
#define BIG_HENT_BUF_SIZE 10240	 /* a jumbo-sized one */


#define MAXHOSTNAMELEN 256
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
	if (gethostname(hostname, MAXHOSTNAMELEN) == SOCKET_ERROR ) {
		strcpy(hostname, "localhost");
    } else {
		struct hostent *hp;
		int h_error=0;

		hp = gethostbyname(hostname);
		if (hp) {
			hp = gethostbyaddr(hp->h_addr, hp->h_length, AF_INET);
			if (hp) {
				strcpy(hostname, hp->h_name);
			} else {
				int err = WSAGetLastError();
			}
		} else {
			int err = WSAGetLastError();
		}
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
	(*env)->SetIntField(env, iaObj, ia_addressID, INADDR_ANY);
	(*env)->SetIntField(env, iaObj, ia_familyID, AF_INET);
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
    struct hostent *hp = 0;

    /* temporary buffer, on the off chance we need to expand */
    char *tmp = NULL;
    int h_error=0;

    if (IS_NULL(host)) {
	JNU_ThrowNullPointerException(env, "host is null");
	return 0;
    }
    hostname = JNU_GetStringPlatformChars(env, host, JNI_FALSE);

    if (hostname == NULL) {
	return 0;
    }

    /* Try once, with our static buffer. */
    hp = gethostbyname(hostname);

	if (hp != NULL) {
		struct in_addr **addrp = (struct in_addr **) hp->h_addr_list;
		int len = sizeof(struct in_addr);
		int i = 0;

		while (*addrp != (struct in_addr *) 0) {
			i++;
			addrp++;
		}
		byteArrayCls = (*env)->FindClass(env, "[B");
		ret = (*env)->NewObjectArray(env, i, byteArrayCls, NULL);

		if (IS_NULL(ret)) {
		/* we may have memory to free at the end of this */
			goto cleanupAndReturn;
		}
		addrp = (struct in_addr **) hp->h_addr_list;
		i = 0;
		while (*addrp) {
	    	jbyteArray barray = (*env)->NewByteArray(env, len);
			if (IS_NULL(barray)) {
				/* we may have memory to free at the end of this */
				ret = NULL;
				goto cleanupAndReturn;
			}
			(*env)->SetByteArrayRegion(env, barray, 0, len,
								       (jbyte *) (*addrp));
			(*env)->SetObjectArrayElement(env, ret, i, barray);
			addrp++;
			i++;
		}
	} else {
		JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException",
			(char *)hostname);
		ret = NULL;
	}

cleanupAndReturn:
	JNU_ReleaseStringPlatformChars(env, host, hostname);
	if (tmp != NULL) {
		free(tmp);
	}
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

    struct hostent *hp = 0;
    int h_error = 0;
    char *tmp = NULL;

    /*
     * We are careful here to use the reentrant version of
     * gethostbyname because at the Java level this routine is not
     * protected by any synchronization.
     *
     * Still keeping the reentrant platform dependent calls temporarily
     * We should probably conform to one interface later.
     * 
     */
    addr = htonl(addr);
    hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);

    if (hp == NULL) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", NULL);
    } else {
	ret = (*env)->NewStringUTF(env, hp->h_name);
    }
    if (tmp) {
	free(tmp);
    }
    return ret;
}

