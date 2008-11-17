/*
 * @(#)InetAddressImpl_md.c	1.13 06/10/10
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

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_InetAddressImpl.h"

/* the initial size of our hostent buffers */
#define HENT_BUF_SIZE 1024
#define BIG_HENT_BUF_SIZE 10240	 /* a jumbo-sized one */

#ifndef __GLIBC__
/* gethostname() is in libc.so but I can't find a header file for it */
extern int gethostname(char *buf, int buf_len);
#endif

#include "jni_statics.h"

/*
 * FIXME: Darwin doesn't support the reentrant calls. Add locking.
 */
#define gethostbyname_r(a,b,c,d,e) gethostbyname(a); \
     (void)(b); (void)(c); (void)(d); (void)(e);
#define gethostbyaddr_r(a,b,c,d,e,f,g) gethostbyaddr(a,b,c); \
     (void)(d); (void)(e); (void)(f); (void)(g); 

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
	/* Something went wrong, maybe networking is not setup? */
	strcpy(hostname, "localhost");
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
	(*env)->SetIntField(env, iaObj,
			    JNI_STATIC(java_net_InetAddress, ia_addressID),
			    INADDR_ANY);
	(*env)->SetIntField(env, iaObj,
			    JNI_STATIC(java_net_InetAddress,
				       ia_familyID), AF_INET);
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
    struct hostent res, *hp = 0;
    char buf[HENT_BUF_SIZE];

    /* temporary buffer, on the off chance we need to expand */
    char *tmp = NULL;
    int h_error=0;

    if (IS_NULL(host)) {
	JNU_ThrowNullPointerException(env, "host is null");
	return 0;
    }
    hostname = JNU_GetStringPlatformChars(env, host, JNI_FALSE);

    /* Try once, with our static buffer. */
#ifdef __GLIBC__
    gethostbyname_r(hostname, &res, buf, sizeof(buf), &hp, &h_error);
#else    
    hp = gethostbyname_r(hostname, &res, buf, sizeof(buf), &h_error);
#endif

    /* With the re-entrant system calls, it's possible that the buffer
     * we pass to it is not large enough to hold an exceptionally
     * large DNS entry.  This is signaled by errno->ERANGE.  We try once
     * more, with a very big size.
     */
    if (hp == NULL && errno == ERANGE) {
	if ((tmp = (char*)malloc(BIG_HENT_BUF_SIZE))) {
#ifdef __GLIBC__
	    gethostbyname_r(hostname, &res, tmp, BIG_HENT_BUF_SIZE,
			    &hp, &h_error);
#else
	    hp = gethostbyname_r(hostname, &res, tmp, BIG_HENT_BUF_SIZE,
				 &h_error);
#endif
	}
    }
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

    struct hostent hent, *hp = 0;
    char buf[HENT_BUF_SIZE];
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
#ifdef __GLIBC__
    gethostbyaddr_r((char *)&addr, sizeof(addr), AF_INET, &hent,
		    buf, sizeof(buf), &hp, &h_error);
#else
    hp = gethostbyaddr_r((char *)&addr, sizeof(addr), AF_INET, &hent,
			 buf, sizeof(buf), &h_error);
#endif
    /* With the re-entrant system calls, it's possible that the buffer
     * we pass to it is not large enough to hold an exceptionally
     * large DNS entry.  This is signaled by errno->ERANGE.  We try once
     * more, with a very big size.
     */
    if (hp == NULL && errno == ERANGE) {
	if ((tmp = (char*)malloc(BIG_HENT_BUF_SIZE))) {
#ifdef __GLIBC__
	    gethostbyaddr_r((char *)&addr, sizeof(addr), AF_INET,
			    &hent, tmp, BIG_HENT_BUF_SIZE, &hp, &h_error);
#else
	    hp = gethostbyaddr_r((char *)&addr, sizeof(addr), AF_INET,
				 &hent, tmp, BIG_HENT_BUF_SIZE, &h_error);
#endif
	} else {
	    JNU_ThrowOutOfMemoryError(env, "getHostByAddr");
	}
    }
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

