/*
 * @(#)Inet4AddressImpl.c	1.15 06/10/10
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

#include <ctype.h>
#include <stdio.h>
#include <malloc.h>
#ifndef WINCE
#include <sys/types.h>
#endif

#include "java_net_InetAddress.h"
#include "java_net_Inet4AddressImpl.h"

#include "jni_statics.h"

#define ia_addressID	JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia_familyID	JNI_STATIC(java_net_InetAddress, ia_familyID)

/* the initial size of our hostent buffers */
#define HENT_BUF_SIZE 1024
#define BIG_HENT_BUF_SIZE 10240	 /* a jumbo-sized one */

#define MAXHOSTNAMELEN 256

/*
 * Returns true if hostname is in dotted IP address format. Note that this 
 * function performs a syntax check only. For each octet it just checks that
 * the octet is at most 3 digits.
 */
jboolean isDottedIPAddress(const char *hostname, unsigned int *addrp) {
    char *c = (char *)hostname;
    int octets = 0;
    unsigned int cur = 0;
    int digit_cnt = 0;

    while (*c) {
	if (*c == '.') {
	    if (digit_cnt == 0) {
		return JNI_FALSE;
	    } else {
		if (octets < 4) {
		    addrp[octets++] = cur;
		    cur = 0;
		    digit_cnt = 0;
		} else {		
		    return JNI_FALSE;
		}
	    }
	    c++;
	    continue;
	}

	if ((*c < '0') || (*c > '9')) {
	    return JNI_FALSE;	
	}

	digit_cnt++;
	if (digit_cnt > 3) {
	    return JNI_FALSE;
	}

	/* don't check if current octet > 255 */
	cur = cur*10 + (*c - '0');			  
			    
	/* Move onto next character and check for EOF */
	c++;
	if (*c == '\0') {
	    if (octets < 4) {
		addrp[octets++] = cur;
	    } else {		
		return JNI_FALSE;
	    }
	}
    }

    return (jboolean)(octets == 4);
}

/*
 * Inet4AddressImpl
 */

/*
 * Class:     java_net_Inet4AddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL 
Java_java_net_Inet4AddressImpl_getLocalHostName (JNIEnv *env, jobject this) {
 /*    char hostname[MAXHOSTNAMELEN+1]; */
/*     hostname[0] = '\0'; */
/*     if (gethostname(hostname, MAXHOSTNAMELEN) == SOCKET_ERROR ) { */
/* 		strcpy(hostname, "localhost"); */
/*     } else { */
/*         struct hostent *hp; */
/*         int h_error=0; */

/*         hp = gethostbyname(hostname); */
/*         if (hp) { */
/*             hp = gethostbyaddr(hp->h_addr, hp->h_length, AF_INET); */
/*             if (hp) { */
/*               strcpy(hostname, hp->h_name); */
/*             } else { */
/*                 int err = WSAGetLastError(); */
/*             } */
/*         } else { */
/*             int err = WSAGetLastError(); */
/*         } */
/*     } */
/*     return (*env)->NewStringUTF(env, hostname); */

    char hostname[256];
    if (JVM_GetHostName(hostname, sizeof hostname) == -1) {
	strcpy(hostname, "localhost");
    }
    return JNU_NewStringPlatform(env, hostname);
}

/*
 * Find an internet address for a given hostname.  Not this this
 * code only works for addresses of type INET. The translation
 * of %d.%d.%d.%d to an address (int) occurs in java now, so the
 * String "host" shouldn't be a %d.%d.%d.%d string. The only 
 * exception should be when any of the %d are out of range and
 * we fallback to a lookup.
 *
 * Class:     java_net_Inet4AddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 *
 * This is almost shared code
 */

JNIEXPORT jobjectArray JNICALL 
Java_java_net_Inet4AddressImpl_lookupAllHostAddr(JNIEnv *env, jobject this, 
						jstring host) {
    const char *hostname;
    jobjectArray ret = 0;
    jclass byteArrayCls;
    struct hostent *hp = 0;

    unsigned int addr[4];

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

    /*
     * The NT/2000 resolver tolerates a space in front of localhost. This
     * is not consistent with other implementations of gethostbyname.
     * In addition we must do a white space check on Solaris to avoid a
     * bug whereby 0.0.0.0 is returned if any host name has a white space.
     */
    if (isspace(hostname[0])) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", hostname);
	goto cleanupAndReturn;
    } 

    /*
     * If the format is x.x.x.x then don't use gethostbyname as Windows
     * is unable to handle octets which are out of range.
     */
    if (isDottedIPAddress(hostname, &addr[0])) {
	unsigned int address;
	jbyteArray barray;
	jobjectArray oarray;  

	/* 
	 * Are any of the octets out of range?
	 */
	if (addr[0] > 255 || addr[1] > 255 || addr[2] > 255 || addr[3] > 255) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", hostname);
	    goto cleanupAndReturn;
	} 

	/*
	 * Return an byte array with the populated address.
	 */
	address = (addr[3]<<24) & 0xff000000;
	address |= (addr[2]<<16) & 0xff0000;
	address |= (addr[1]<<8) & 0xff00;
	address |= addr[0];

	byteArrayCls = (*env)->FindClass(env, "[B");
	if (byteArrayCls == NULL) {
	    goto cleanupAndReturn;
	}
	
	barray = (*env)->NewByteArray(env, 4);
	oarray = (*env)->NewObjectArray(env, 1, byteArrayCls, NULL);

	if (barray == NULL || oarray == NULL) {
	    /* pending exception */
	    goto cleanupAndReturn;
	}
	(*env)->SetByteArrayRegion(env, barray, 0, 4, (jbyte *)&address);
	(*env)->SetObjectArrayElement(env, oarray, 0, barray);

	JNU_ReleaseStringPlatformChars(env, host, hostname);
	return oarray;	
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
 * Class:     java_net_Inet4AddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet4AddressImpl_getHostByAddr(JNIEnv *env, jobject this, 
					    jbyteArray addrArray) {

    struct hostent *hp;
    jbyte caddr[4];
    jint addr;
    (*env)->GetByteArrayRegion(env, addrArray, 0, 4, caddr);
    addr = ((caddr[0]<<24) & 0xff000000);
    addr |= ((caddr[1] <<16) & 0xff0000);
    addr |= ((caddr[2] <<8) & 0xff00);
    addr |= (caddr[3] & 0xff); 
    addr = htonl(addr);

    hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
    if (hp == NULL) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", 0);
	return NULL;
    }
    if (hp->h_name == NULL) { /* Deal with bug in Windows XP */
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", 0);
	return NULL;
    }
    return JNU_NewStringPlatform(env, hp->h_name);
}
