/*
 * @(#)Inet6AddressImpl_md.c	1.8 06/10/10 
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

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_Inet6AddressImpl.h"

/* the initial size of our hostent buffers */
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif 

#include "jni_statics.h"

/************************************************************************
 * Inet6AddressImpl
 */

/*
 * Class:     java_net_Inet6AddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet6AddressImpl_getLocalHostName(JNIEnv *env, jobject this) {
    char hostname[NI_MAXHOST+1];

    hostname[0] = '\0';
    if (JVM_GetHostName(hostname, MAXHOSTNAMELEN)) {
	/* Something went wrong, maybe networking is not setup? */
	strcpy(hostname, "localhost");
    } else {
#ifdef __linux__
	/* On Linux gethostname() says "host.domain.sun.com".  On
	 * Solaris gethostname() says "host", so extra work is needed.
	 */
#else
	/* Solaris doesn't want to give us a fully qualified domain name.
	 * We do a reverse lookup to try and get one.  This works
	 * if DNS occurs before NIS in /etc/resolv.conf, but fails
	 * if NIS comes first (it still gets only a partial name).
	 * We use thread-safe system calls.
	 */
#ifdef AF_INET6
        if (NET_addrtransAvailable()) {
	    struct addrinfo  hints, *res;
	    int error;

	    bzero(&hints, sizeof(hints));
	    hints.ai_flags = AI_CANONNAME;
	    hints.ai_family = AF_UNSPEC;

	    error = (*getaddrinfo_ptr)(hostname, "domain", &hints, &res);

	    if (error == 0) {
		/* host is known to name service */
	        error = (*getnameinfo_ptr)(res->ai_addr, 
					   res->ai_addrlen, 
					   hostname, 
					   NI_MAXHOST, 
					   NULL, 
					   0, 
					   NI_NAMEREQD);

		/* if getnameinfo fails hostname is still the value
		   from gethostname */

	        (*freeaddrinfo_ptr)(res);
	    }
	}
#endif /* AF_INET6 */
#endif /* __linux__ */
    }
    return (*env)->NewStringUTF(env, hostname);
}

/*
 * Find an internet address for a given hostname.  Note that this
 * code only works for addresses of type INET. The translation
 * of %d.%d.%d.%d to an address (int) occurs in java now, so the
 * String "host" shouldn't *ever* be a %d.%d.%d.%d string
 *
 * Class:     java_net_Inet6AddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 */

JNIEXPORT jobjectArray JNICALL
Java_java_net_Inet6AddressImpl_lookupAllHostAddr(JNIEnv *env, jobject this,
						jstring host) {
    const char *hostname;
    jobjectArray ret = 0;
    int retLen = 0;
    jclass byteArrayCls;
    jboolean preferIPv6Address;

    int error=0;
#ifdef AF_INET6
    struct addrinfo hints, *res, *resNew = NULL;
#endif /* AF_INET6 */

    if (IS_NULL(host)) {
	JNU_ThrowNullPointerException(env, "host is null");
	return 0;
    }
    hostname = JNU_GetStringPlatformChars(env, host, JNI_FALSE);
    CHECK_NULL_RETURN(hostname, NULL);

#ifdef AF_INET6
    if (NET_addrtransAvailable()) {
	static jfieldID ia_preferIPv6AddressID;
	if (ia_preferIPv6AddressID == NULL) {
	    jclass c = (*env)->FindClass(env,"java/net/InetAddress");
	    if (c)  {
		ia_preferIPv6AddressID =
                    (*env)->GetStaticFieldID(env, c, "preferIPv6Address", "Z");
	    }
	    if (ia_preferIPv6AddressID == NULL) {
		JNU_ReleaseStringPlatformChars(env, host, hostname);
		return NULL;
	    }
	} 
	/* get the address preference */
	preferIPv6Address 
	    = (*env)->GetStaticBooleanField(env, JNI_STATIC(java_net_InetAddress, ia_class), ia_preferIPv6AddressID); 

	/* Try once, with our static buffer. */
	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	
	/* 
	 * Workaround for Solaris bug 4160367 - if a hostname contains a
	 * white space then 0.0.0.0 is returned
	 */
	if (isspace(hostname[0])) {
	    JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException",
			    (char *)hostname);
	    JNU_ReleaseStringPlatformChars(env, host, hostname);
	    return NULL;
	}
	
	error = (*getaddrinfo_ptr)(hostname, "domain", &hints, &res);
	
	if (error) {
	    /* report error */
	    JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException",
			    (char *)hostname);
	    JNU_ReleaseStringPlatformChars(env, host, hostname);
	    return NULL;
	} else {
	    int i = 0;
	    struct addrinfo *itr, *iterator = res;
            struct addrinfo *last = NULL;
            int inetCount = 0, inet6Count = 0, inetIndex, inet6Index;
	    while (iterator != NULL) {
		int skip = 0;
		itr = resNew;
		while (itr != NULL) {
		    if (iterator->ai_family == itr->ai_family &&
			iterator->ai_addrlen == itr->ai_addrlen) {
			if (itr->ai_family == AF_INET) { /* AF_INET */
			    struct sockaddr_in *addr1, *addr2;
			    addr1 = (struct sockaddr_in *)iterator->ai_addr;
			    addr2 = (struct sockaddr_in *)itr->ai_addr;
			    if (addr1->sin_addr.s_addr == 
				addr2->sin_addr.s_addr) {
				skip = 1;
				break;
			    } 
			} else {
			    int t;
			    struct sockaddr_in6 *addr1, *addr2;
			    addr1 = (struct sockaddr_in6 *)iterator->ai_addr;
			    addr2 = (struct sockaddr_in6 *)itr->ai_addr;
			    
			    for (t = 0; t < 16; t++) {
				if (addr1->sin6_addr.s6_addr[t] !=
				    addr2->sin6_addr.s6_addr[t]) {
				    break;
				} 
			    }
			    if (t < 16) {
				itr = itr->ai_next;
				continue;
			    } else {
				skip = 1;
				break;
			    }
			}
		    } else if (iterator->ai_family != AF_INET && 
			       iterator->ai_family != AF_INET6) { 
			/* we can't handle other family types */
			skip = 1;
			break;	
		    }
		    itr = itr->ai_next;
		}
		
		if (!skip) {
		    struct addrinfo *next 
			= (struct addrinfo*) malloc(sizeof(struct addrinfo));
		    if (!next) {
			JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
			ret = NULL;
			goto cleanupAndReturn;
		    }
		    memcpy(next, iterator, sizeof(struct addrinfo));
		    next->ai_next = NULL;
		    if (resNew == NULL) {
			resNew = next;
		    } else {
			last->ai_next = next;
		    }
		    last = next;
		    i++;
                    if (iterator->ai_family == AF_INET) {
			inetCount ++;
		    } else if (iterator->ai_family == AF_INET6) {
			inet6Count ++;
		    }
		}
		iterator = iterator->ai_next;
	    }
	    retLen = i;
	    iterator = resNew;
	    byteArrayCls = (*env)->FindClass(env, "[B");
	    if (byteArrayCls == NULL) {
		/* pending exception */
		goto cleanupAndReturn;
	    }
	    ret = (*env)->NewObjectArray(env, retLen, byteArrayCls, NULL);
	    
	    if (IS_NULL(ret)) {
		/* we may have memory to free at the end of this */
		goto cleanupAndReturn;
	    }

	    if (preferIPv6Address) {
		/* AF_INET addresses will be offset by inet6Count */
		inetIndex = inet6Count;
		inet6Index = 0;
	    } else {
		/* AF_INET6 addresses will be offset by inetCount */
		inetIndex = 0;
		inet6Index = inetCount;
	    }

	    while (iterator != NULL) {
		/* if ai_addrlen is 16 bytes, it points to sockaddr_in;
		 * if it is 24 bytes, it points to sockaddr_in6. 
		 * in the former case, we need 4 bytes to store ipv4 address;
		 * in the latter case, we need 16 bytes to store ipv6 address.
		 */
		int len = iterator->ai_addrlen== 16 ? 4:16;
		jbyteArray barray = (*env)->NewByteArray(env, len);
		
		if (IS_NULL(barray)) {
		    /* we may have memory to free at the end of this */
		    ret = NULL;
		    goto cleanupAndReturn;
		}
		if (iterator->ai_family == AF_INET) {
		    (*env)->SetByteArrayRegion(env, barray, 0, len,
					       (jbyte *) &((struct sockaddr_in*)(iterator->ai_addr))->sin_addr);
                    (*env)->SetObjectArrayElement(env, ret, inetIndex, barray);
                    inetIndex++;
		} else if (iterator->ai_family == AF_INET6) {
		    (*env)->SetByteArrayRegion(env, barray, 0, len,
					       (jbyte *) ((struct sockaddr_in6*)(iterator->ai_addr))->sin6_addr.s6_addr);
                    (*env)->SetObjectArrayElement(env, ret, inet6Index, barray);
		    inet6Index++;
		}
                iterator = iterator->ai_next;
	    }
	}
    }
    
#endif /* AF_INET6 */
    
cleanupAndReturn:
    {
	struct addrinfo *iterator, *tmp;
	iterator = resNew;
	while (iterator != NULL) {
	    tmp = iterator;
	    iterator = iterator->ai_next;
	    free(tmp);
	}
	JNU_ReleaseStringPlatformChars(env, host, hostname);
    }
   
#ifdef AF_INET6
    if (NET_addrtransAvailable())
	(*freeaddrinfo_ptr)(res);
#endif /* AF_INET6 */
    
    return ret;
}

/*
 * Class:     java_net_Inet6AddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet6AddressImpl_getHostByAddr(JNIEnv *env, jobject this,
					    jbyteArray addrArray) {

    jstring ret = NULL;

#ifdef AF_INET6
    char host[NI_MAXHOST+1];
    /* eliminate compile time warning.
      jfieldID fid;
    */
    int error = 0;
    /* eliminate compile time warning.
       jint family;
       struct sockaddr *him ;
    */
    int len = 0;
    jbyte caddr[16];

    if (NET_addrtransAvailable()) {
	struct sockaddr_in him4;
        struct sockaddr_in6 him6;
	struct sockaddr *sa;

	/* 
	 * For IPv4 addresses construct a sockaddr_in structure.
	 */
	if ((*env)->GetArrayLength(env, addrArray) == 4) {
	    jint addr;
	    (*env)->GetByteArrayRegion(env, addrArray, 0, 4, caddr);
	    addr = ((caddr[0]<<24) & 0xff000000);
    	    addr |= ((caddr[1] <<16) & 0xff0000);
            addr |= ((caddr[2] <<8) & 0xff00);
    	    addr |= (caddr[3] & 0xff);
	    memset((char *) &him4, 0, sizeof(him4));
	    him4.sin_addr.s_addr = (uint32_t) htonl(addr);
	    him4.sin_family = AF_INET;
	    sa = (struct sockaddr *) &him4;
	    len = sizeof(him4);
	} else {
	    /*
	     * For IPv6 address construct a sockaddr_in6 structure.
	     */
	    (*env)->GetByteArrayRegion(env, addrArray, 0, 16, caddr); 
            memset((char *) &him6, 0, sizeof(him6)); 
            memcpy((void *)&(him6.sin6_addr), caddr, sizeof(struct in6_addr) ); 
            him6.sin6_family = AF_INET6; 
            sa = (struct sockaddr *) &him6 ;
            len = sizeof(him6) ; 
        }
    
        error = (*getnameinfo_ptr)(sa, len, host, NI_MAXHOST, NULL, 0,
				   NI_NAMEREQD);

        if (!error) {
	    ret = (*env)->NewStringUTF(env, host);
        }
    }
#endif /* AF_INET6 */

    if (ret == NULL) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", NULL);
    }

    return ret;
}

