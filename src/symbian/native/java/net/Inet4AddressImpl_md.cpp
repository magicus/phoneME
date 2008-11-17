/*
 * @(#)Inet4AddressImpl_md.cpp	1.15 06/10/10
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
#include <ctype.h>
#include <es_sock.h>
#include <in_sock.h>
#include <utf.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_Inet4AddressImpl.h"

/* the initial size of our hostent buffers */
#define HENT_BUF_SIZE 1024
#define BIG_HENT_BUF_SIZE 10240	 /* a jumbo-sized one */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#include "jni_statics.h"

#define ia_addressID \
    JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia_familyID \
    JNI_STATIC(java_net_InetAddress, ia_familyID)

extern RSocketServ sserv;

/************************************************************************
 * Inet4AddressImpl
 */

_LIT(localhost, "localhost");
#if 0
_LIT(symbian, "symbian");
#endif

TInt openResolver(RHostResolver &r)
{
    _LIT(udp, "udp");
    TProtocolDesc desc;
    TProtocolName pname(udp);
    TInt err = sserv.FindProtocol(pname, desc);
    if (err != KErrNone) {
	return NULL;
    }
    assert((desc.iNamingServices & KNSNameResolution) != 0);

    return r.Open(sserv, desc.iAddrFamily, desc.iProtocol);
}

TInt SYMBIANresolveInit()
{
    RHostResolver r;
    TInt err = openResolver(r);
    if (err != KErrNone) {
	return err;
    }
#if 0
    err = r.SetHostName(symbian);
#endif
    r.Close();
    return err;
}

#if 0
static void
list_interfaces(RSocketServ &ss)
{
    TPckgBuf<TSoInet6InterfaceInfo> info6;
    TSoInet6InterfaceInfo &i6 = info6();
    TPckgBuf<TSoInetInterfaceInfo> info;
    TSoInetInterfaceInfo &i = info();
    RSocket r;
    //_LIT(proto, "udp");
    _LIT(proto, "ip");
    TInt err = r.Open(ss, proto);
    err = r.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl, 1);
    while ((err = r.GetOpt(KSoInetNextInterface,
	KSolInetIfCtrl, info)) == KErrNone)
    {
	TName n = i.iName;
#ifdef _UNICODE
	TUint8 n8[0x20];
	TPtr8 name8(n8, sizeof n8);
	TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, n);
	fprintf(stderr, "Interface '%s' status %x\n", name8.PtrZ(),
	    i.iState);
#endif
	if (i.iState == EIfUp) {
	    TInetAddr ia = i.iAddress;
	    if (ia.Family() == KAfInet) {
		TUint32 a = ia.Address();
		fprintf(stderr, "Address %x\n", a);
	    } else if (ia.Family() == KAfInet6) {
		const TIp6Addr &a = ia.Ip6Address();
		fprintf(stderr, "Address %x:%x:%x:%x:%x:%x:%x:%x\n",
			a.u.iAddr16[0],
			a.u.iAddr16[1],
			a.u.iAddr16[2],
			a.u.iAddr16[3],
			a.u.iAddr16[4],
			a.u.iAddr16[5],
			a.u.iAddr16[6],
			a.u.iAddr16[7]);
	    } else {
		fprintf(stderr, "Address %x (unknown)\n", ia.Address());
	    }
	}
    }
    //i.iAddress = 0;
    //i.iName = 0;
    //i.iHwAddr = 0;
    //err = r.GetOpt(KSoInetConfigInterface, KSolInetIfCtrl, info);

    TPckgBuf<TSoInetIfQuery> q1;
    TSoInetIfQuery &q = q1();
    TInt inum = 1;
    q.iIndex = inum;
    while ((err = r.GetOpt(KSoInetIfQueryByIndex,
	KSolInetIfQuery, q1)) == KErrNone)
    {
	fprintf(stderr, "Interface %d up %d\n", inum, q.iIsUp);
#ifdef _UNICODE
	TUint8 n8[0x20];
	TPtr8 name8(n8, sizeof n8);
	TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, q.iName);
	fprintf(stderr, "Interface %d name '%s'\n", inum, name8.PtrZ());
#endif
	if (q.iIsUp) {
	    TInetAddr ia = q.iSrcAddr;
	    if (ia.Family() == KAfInet) {
		TUint32 a = ia.Address();
		fprintf(stderr, "src %x\n", a);
	    } else if (ia.Family() == KAfInet6) {
		const TIp6Addr &a = ia.Ip6Address();
		fprintf(stderr, "src %x:%x:%x:%x\n",
			a.u.iAddr32[0],
			a.u.iAddr32[1],
			a.u.iAddr32[2],
			a.u.iAddr32[3]);
	    } else {
		fprintf(stderr, "src fam %x (unknown)\n", ia.Family());
	    }
	    ia = q.iDstAddr;
	    if (ia.Family() == KAfInet) {
		TUint32 a = ia.Address();
		fprintf(stderr, "dst %x\n", a);
	    } else if (ia.Family() == KAfInet6) {
		const TIp6Addr &a = ia.Ip6Address();
		fprintf(stderr, "dst %x:%x:%x:%x\n",
			a.u.iAddr32[0],
			a.u.iAddr32[1],
			a.u.iAddr32[2],
			a.u.iAddr32[3]);
	    } else {
		fprintf(stderr, "dst fam %x (unknown)\n", ia.Family());
	    }
	}
	q.iIndex = ++inum;
    }
#if 0
    TSoInetIfQuery qq;
    TPckg<TSoInetIfQuery> q2(qq);
    qq.iName = n;
    TUint isUp1 = q.iIsUp;
    err = r.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, q2);
    TUint isUp2 = qq.iIsUp;
 
    TRequestStatus rs;
    TPckgBuf<TSoIfConfig> ifc;	/* IPv4 */
    TSoIfConfig &c = ifc();
    c.iFamily = KAfInet;
    err = r.GetOpt(KSoIfConfig, KSOLInterface, ifc);
    TSockAddr local = c.iLocalId;
    TSockAddr remote = c.iRemoteId;

    TPckgBuf<TSoInet6IfConfig> ifc;	/* IPv6 */
    TSoInet6IfConfig &c = ifc();
    c.iFamily = KAfInet6;
    err = r.GetOpt(KSoIfConfig, KSOLInterface, ifc);
    TInetIfConfig r = c.iConfig;
    TInetAddr ia = r.iAddress;

    TPckgBuf<TSoIfInfo> info;
    err = r.GetOpt(KSoIfInfo, KSOLInterface, info);
    TSoIfInfo &if = info();
    TInterfaceName n = if.iName;

    TPckgBuf<TSoIfHardwareAddr> addr;
    err = r.GetOpt(KSoIfHardwareAddr, KSOLInterface, addr);
    TSoIfHardwareAddr a = addr();
#endif
    r.Close();
}
#endif

static THostName
get_local_host(RSocketServ &ss, RHostResolver &res)
{
#ifdef EKA2
    THostName host = (THostName)localhost;
#else
    THostName host = localhost;
#endif
    TPckgBuf<TSoInet6InterfaceInfo> info6;
    TSoInet6InterfaceInfo &i6 = info6();
    TPckgBuf<TSoInetInterfaceInfo> info;
    TSoInetInterfaceInfo &i = info();
    RSocket r;
    //_LIT(proto, "udp");
    _LIT(proto, "ip");
    TInt err = r.Open(ss, proto);
    err = r.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl, 1);
    while ((err = r.GetOpt(KSoInetNextInterface,
	KSolInetIfCtrl, info)) == KErrNone)
    {
	TName n = i.iName;
	if (i.iState == EIfUp && !i.iAddress.IsLoopback()) {
#ifdef _UNICODE
	    TUint8 n8[0x20];
	    TPtr8 name8(n8, sizeof n8);
	    TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, n);
	    fprintf(stderr, "Interface '%s'\n", name8.PtrZ());
#endif
	    TNameEntry ne;
	    TRequestStatus rs;
	    res.GetByAddress(i.iAddress, ne, rs);
	    User::WaitForRequest(rs);
	    if (rs == KErrNone) {
		THostName name;
		name = ne().iName;
		if (name.Compare(localhost) != 0) {
		    host = name;
		    break;
		}
		int found = 0;
		while (res.Next(ne) == KErrNone) {
		    if (name.Compare(localhost) != 0) {
			host = name;
			found = 1;
			break;
		    }
		}
		if (found) {
		    break;
		}
	    }

	    TInetAddr ia = i.iAddress;
	    if (ia.Family() == KAfInet) {
		TUint32 a = ia.Address();
		_LIT(fmt, "%d.%d.%d.%d");
		host.Format(fmt,
		    ((a >> 24) & 0xff),
		    ((a >> 16) & 0xff),
		    ((a >> 8) & 0xff),
		    ((a) & 0xff));
		break;
	    } else if (ia.Family() == KAfInet6) {
		const TIp6Addr &a = ia.Ip6Address();
		_LIT(fmt, "%x:%x:%x:%x:%x:%x:%x:%x");
		host.Format(fmt,
		    a.u.iAddr16[0],
		    a.u.iAddr16[1],
		    a.u.iAddr16[2],
		    a.u.iAddr16[3],
		    a.u.iAddr16[4],
		    a.u.iAddr16[5],
		    a.u.iAddr16[6],
		    a.u.iAddr16[7]);
		break;
	    }
	}
    }
    
done:
    r.Close();
#ifdef _UNICODE
{
	TUint8 n8[0x20];
	TPtr8 name8(n8, sizeof n8);
	TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, host);
	fprintf(stderr, "get_local_host '%s'\n", name8.PtrZ());
}
#endif
    return host;
}

static void
dump(TNameRecord &nr)
{
    TInetAddr ia = nr.iAddr;
    if (ia.Family() == KAfInet) {
	TUint32 a = ia.Address();
	fprintf(stderr, "address %x\n", a);
    } else if (ia.Family() == KAfInet6) {
	const TIp6Addr &a = ia.Ip6Address();
	fprintf(stderr, "address %x:%x:%x:%x\n",
		a.u.iAddr32[0],
		a.u.iAddr32[1],
		a.u.iAddr32[2],
		a.u.iAddr32[3]);
    } else {
	fprintf(stderr, "address %x (unknown)\n", ia.Address());
    }
#ifdef _UNICODE
{
    TUint8 n8[0x20];
    TPtr8 name8(n8, sizeof n8);
    TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, nr.iName);
    fprintf(stderr, "name '%s'\n", name8.PtrZ());
}
#else
    fprintf(stderr, "name '%s'\n", nr.iName.PtrZ());
#endif
    fprintf(stderr, "flags %x\n", nr.iFlags);
}

static int
check(RHostResolver &r, const TDesC &name);

/*
 * Class:     java_net_Inet4AddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet4AddressImpl_getLocalHostName(JNIEnv *env,
						jobject /*thisObj*/)
{
    THostName name;
    
    RHostResolver r;
    TInt err = openResolver(r);
    if (err != KErrNone) {
	goto fail;
    }
    err = r.GetHostName(name);
    if (err != KErrNone) {
fail:
	/* Something went wrong, maybe networking is not setup? */
#ifdef CVM_DEBUG
	fprintf(stderr, "GetHostName failed with %d\n", err);
#endif
	name = localhost;
    } else {
	if (check(r, name)) {
	    goto done;
	}
	// Try null descriptor, which is supposed to
	// return the "current primary network interface"
	TBufC<1> n;
	if (name.Compare(n) != 0) {
	    if (check(r, n)) {
		name = n;
		goto done;
	    }
	}
	// Search active interfaces for a non-loopback address,
	// which is what the TCK expects
	name = get_local_host(sserv, r);
    }
done:
    r.Close();
#ifdef _UNICODE
    return (*env)->NewString(env, name.Ptr(), name.Length());
#else
    TBuf8<name.MaxLength() + 1> name8 = name;
    return (*env)->NewStringUTF(env, name8.PtrZ());
#endif
}

static int
check(RHostResolver &r, const TDesC &name)
{
    TRequestStatus rs;
    TNameRecord nr;
    TNameEntry res;
    r.GetByName(name, res, rs);
    User::WaitForRequest(rs);
    if (rs == KErrNone) {
	nr = res();
	TInetAddr ia = nr.iAddr;
#ifdef CVM_DEBUG
dump(nr);
#endif
	if (!ia.IsLoopback()) {
	    return 1;
	}
	while (r.Next(res) == KErrNone) {
	    nr = res();
	    ia = nr.iAddr;
	    if (!ia.IsLoopback()) {
		return 1;
	    }
	}
    }
    return 0;
}


/*
 * Find an internet address for a given hostname.  Not this this
 * code only works for addresses of type INET. The translation
 * of %d.%d.%d.%d to an address (int) occurs in java now, so the
 * String "host" shouldn't *ever* be a %d.%d.%d.%d string
 *
 * Class:     java_net_Inet4AddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 */

JNIEXPORT jobjectArray JNICALL
Java_java_net_Inet4AddressImpl_lookupAllHostAddr(JNIEnv *env, jobject thisObj,
						jstring host)
{
    const char *hostname;
    jobjectArray ret = 0;
    jclass byteArrayCls;

    if (IS_NULL(host)) {
	JNU_ThrowNullPointerException(env, "host is null");
	return 0;
    }

    hostname = JNU_GetStringPlatformChars(env, host, JNI_FALSE);
    CHECK_NULL_RETURN(hostname, NULL);
#ifndef _UNICODE
    TPtrC8 des((const TUint8 *)hostname, strlen(hostname));
#else
    jboolean copy;
    jint len = (*env)->GetStringLength(env, host);
    const jchar *h  = (*env)->GetStringChars(env, host, &copy);
    CHECK_NULL_RETURN(h, NULL);
    TPtrC16 des((const TUint16 *)h, len);
#endif
    RHostResolver r;
    TNameRecord results[16];
    TNameEntry res;
    TRequestStatus rs;
#ifdef CVM_DEBUG
    fprintf(stderr, "GetByName %s ...\n", hostname);
#endif

    TInt err = openResolver(r);
    if (err != KErrNone) {
	goto fail;
    }

    r.GetByName(des, res, rs);
    User::WaitForRequest(rs);

    if (rs == KErrNone) {
	results[0] = res();
	int i = 1;

	while (i < 16 && r.Next(res) == KErrNone) {
	    TUint fam = res().iAddr.Family();
	    if (fam  == KAfInet || fam == KAfInet6) {
		results[i] = res();
		++i;
	    }
	}

	byteArrayCls = (*env)->FindClass(env, "[B");
 	if (byteArrayCls == NULL) {
	    /* pending exception */
	    goto cleanupAndReturn;
	}
        ret = (*env)->NewObjectArray(env, i, byteArrayCls, NULL);

	if (IS_NULL(ret)) {
	    /* we may have memory to free at the end of this */
	    goto cleanupAndReturn;
	}

	int j;
	for (j = 0; j < i; ++j) {
	    jint len = 4;
	    TNameRecord &r = results[j];
	    if (r.iAddr.Family() == KAfInet6) {
		len = 16;
	    }
	    jbyteArray barray = (*env)->NewByteArray(env, len);
	    if (IS_NULL(barray)) {
		/* we may have memory to free at the end of this */
	        ret = NULL;
		goto cleanupAndReturn;
	    }
	    TInetAddr ia = r.iAddr;
	    if (len  == 4) {
		TUint32 a = ia.Address();
		jbyte ja[4];
		ja[0] = (jbyte)(a >> 24);
		ja[1] = (jbyte)(a >> 16);
		ja[2] = (jbyte)(a >> 8);
		ja[3] = (jbyte)(a);
		(*env)->SetByteArrayRegion(env, barray, 0, len, ja);
#ifdef CVM_DEBUG
		fprintf(stderr, "GetByName %s --> %x\n", hostname, a);
#endif
	    } else {
		const TIp6Addr &a = ia.Ip6Address();
		(*env)->SetByteArrayRegion(env, barray, 0, len,
		    (jbyte *)a.u.iAddr8);
#ifdef CVM_DEBUG
		fprintf(stderr, "GetByName %s --> %x:%x:%x:%x\n",
			hostname,
			a.u.iAddr32[0],
			a.u.iAddr32[1],
			a.u.iAddr32[2],
			a.u.iAddr32[3]);
#endif
	    }
	    (*env)->SetObjectArrayElement(env, ret, j, barray);
	}
    } else {
fail:
	JNU_ThrowByName(env, JNU_JAVANETPKG "UnknownHostException",
			(char *)hostname);
	ret = NULL;
    }

cleanupAndReturn:
    r.Close();
#ifdef _UNICODE
    (*env)->ReleaseStringChars(env, host, h);
#endif
    JNU_ReleaseStringPlatformChars(env, host, hostname);
    return ret;
}

/*
 * Class:     java_net_Inet4AddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_net_Inet4AddressImpl_getHostByAddr(JNIEnv *env, jobject thisObj,
					    jbyteArray addrArray) {
    jstring ret = NULL;
    jint addr;

    RHostResolver r;
    TNameEntry res;
    TInt err = openResolver(r);

    if (err == KErrNone) {
	TUint8 caddr[4];
	(*env)->GetByteArrayRegion(env, addrArray, 0, 4, (jbyte *)caddr);
	TInt a = INET_ADDR(caddr[0], caddr[1], caddr[2], caddr[3]);
	TInetAddr ia(a, 0);
	TRequestStatus rs;
	r.GetByAddress(ia, res, rs);
	User::WaitForRequest(rs);
	r.Close();
	err = rs.Int();
    }

    if (err != KErrNone) {
fail:
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
