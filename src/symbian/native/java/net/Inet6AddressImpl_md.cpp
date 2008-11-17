/*
 * @(#)Inet6AddressImpl_md.cpp	1.5 06/10/10
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_Inet4AddressImpl.h"
#include "java_net_Inet6AddressImpl.h"

#include "jni_statics.h"

extern RSocketServ sserv;
TInt openResolver(RHostResolver &r);

/************************************************************************
 * Inet6AddressImpl
 */

/*
 * Class:     java_net_Inet6AddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet6AddressImpl_getLocalHostName(JNIEnv *env, jobject thisObj)
{
    return Java_java_net_Inet4AddressImpl_getLocalHostName(env, thisObj);
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
Java_java_net_Inet6AddressImpl_lookupAllHostAddr(JNIEnv *env, jobject thisObj,
						jstring host)
{
    return Java_java_net_Inet4AddressImpl_lookupAllHostAddr(env, thisObj, host);
}

/*
 * Class:     java_net_Inet6AddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet6AddressImpl_getHostByAddr(JNIEnv *env, jobject thisObj,
					    jbyteArray addrArray)
{
    /* 
     * IPv4 ...
     */
    if ((*env)->GetArrayLength(env, addrArray) == 4) {
	return Java_java_net_Inet4AddressImpl_getHostByAddr(env,
	    thisObj, addrArray);
    }

    TIp6Addr ip6;

    /*
     * For IPv6 address construct a IPv6 TInetAddr.
     */
    (*env)->GetByteArrayRegion(env, addrArray, 0, 16, (jbyte *)ip6.u.iAddr8); 
    TInetAddr ia(ip6, 0);
    jstring ret = NULL;
    jint addr;

    RHostResolver r;
    TNameEntry res;
    TInt err = openResolver(r);

    if (err == KErrNone) {
	TRequestStatus rs;
	r.GetByAddress(ia, res, rs);
	User::WaitForRequest(rs);
	r.Close();
	err = rs.Int();
    }

    if (err != KErrNone) {
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException", NULL);
    } else {
	TNameRecord &r = res();
#ifdef _UNICODE
	ret = (*env)->NewString(env, r.iName.Ptr(), r.iName.Length());
#else
	TBuf8<r.iName.MaxLength() + 1> name = r.iName();
	ret = (*env)->NewStringUTF(env, name.PtrZ());
#endif
    }
    return ret;
}
