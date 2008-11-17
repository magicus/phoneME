/*
 * @(#)NetworkInterface.cpp	1.7 06/10/10
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

#include <e32def.h>
#include <e32std.h>
#include <es_sock.h>
#include <in_sock.h>
#include <in_iface.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>

#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

#include "java_net_NetworkInterface.h"

/************************************************************************
 * NetworkInterface
 */

#include "jni_statics.h"

#define ni_iacls \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_iacls)
#define ni_ia4cls \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ia4cls)
#define ni_ia6cls \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ia6cls)
#define ni_ia4ctrID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ia4ctrID)
#define ni_ia6ctrID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ia6ctrID)
#define ni_iaaddressID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_iaaddressID)
#define ni_iafamilyID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_iafamilyID)
#define ni_ia6ipaddressID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ia6ipaddressID)

#define ni_class \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_class)
#define ni_nameID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_nameID)
#define ni_indexID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_indexID)
#define ni_descID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_descID)
#define ni_addrsID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_addrsID)
#define ni_ctrID \
    JNI_STATIC_MD(java_net_NetworkInterface, ni_ctrID)

struct netaddr {
    int family;
    TInetAddr ia;
    netaddr *next;
};

struct netif {
    int index;
    char *name;
    netaddr *addr;
    netif *next;
};

extern RSocketServ sserv;

static jobject createNetworkInterface(JNIEnv *env, netif *ifs);

static netif *enumInterfaces(JNIEnv *env);
static netif *enumIPvXInterfaces(JNIEnv *env, netif *ifs, TUint family);

static netif *addif(JNIEnv *env, netif *ifs, const char *if_name, int index,
		    int family, TInetAddr &ia);
static void freeif(netif *ifs);


/*
 * Class:     java_net_NetworkInterface
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_NetworkInterface_init(JNIEnv *env, jclass cls)
{
    init_IPv6Available(env);
    ni_class = (*env)->FindClass(env,"java/net/NetworkInterface");
    ni_class = (*env)->NewGlobalRef(env, ni_class);
    ni_nameID = (*env)->GetFieldID(env, ni_class,"name", "Ljava/lang/String;");
    ni_indexID = (*env)->GetFieldID(env, ni_class, "index", "I");
    ni_addrsID = (*env)->GetFieldID(env, ni_class, "addrs", "[Ljava/net/InetAddress;");
    ni_descID = (*env)->GetFieldID(env, ni_class, "displayName", "Ljava/lang/String;");
    ni_ctrID = (*env)->GetMethodID(env, ni_class, "<init>", "()V");

    ni_iacls = (*env)->FindClass(env, "java/net/InetAddress");
    ni_iacls = (*env)->NewGlobalRef(env, ni_iacls);
    ni_ia4cls = (*env)->FindClass(env, "java/net/Inet4Address");
    ni_ia4cls = (*env)->NewGlobalRef(env, ni_ia4cls);
    ni_ia6cls = (*env)->FindClass(env, "java/net/Inet6Address");
    ni_ia6cls = (*env)->NewGlobalRef(env, ni_ia6cls);
    ni_ia4ctrID = (*env)->GetMethodID(env, ni_ia4cls, "<init>", "()V");
    ni_ia6ctrID = (*env)->GetMethodID(env, ni_ia6cls, "<init>", "()V");
    ni_iaaddressID = (*env)->GetFieldID(env, ni_iacls, "address", "I");
    ni_iafamilyID = (*env)->GetFieldID(env, ni_iacls, "family", "I");
    ni_ia6ipaddressID = (*env)->GetFieldID(env, ni_ia6cls, "ipaddress", "[B");
}


/*
 * Class:     java_net_NetworkInterface
 * Method:    getByName0
 * Signature: (Ljava/lang/String;)Ljava/net/NetworkInterface;
 */
JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByName0
    (JNIEnv *env, jclass cls, jstring name)
{
    netif *ifs, *curr;
    jboolean isCopy;
    const char* name_utf = (*env)->GetStringUTFChars(env, name, &isCopy);
    jobject obj = NULL;

    ifs = enumInterfaces(env);
    if (ifs == NULL) {
	return NULL;
    }

    /*
     * Search the list of interface based on name
     */
    curr = ifs;
    while (curr != NULL) {
        if (strcmp(name_utf, curr->name) == 0) {
            break;
        }
        curr = curr->next;
    }

    /* if found create a NetworkInterface */
    if (curr != NULL) {;
        obj = createNetworkInterface(env, curr);
    }

    /* release the UTF string and interface list */
    (*env)->ReleaseStringUTFChars(env, name, name_utf);
    freeif(ifs);

    return obj;
}


/*
 * Class:     java_net_NetworkInterface
 * Method:    getByIndex
 * Signature: (Ljava/lang/String;)Ljava/net/NetworkInterface;
 */
JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByIndex
    (JNIEnv *env, jclass cls, jint index)
{
    netif *ifs, *curr;
    jobject obj = NULL;

    if (index <= 0) {
        return NULL;
    }

    ifs = enumInterfaces(env);
    if (ifs == NULL) {
        return NULL;
    }

    /*
     * Search the list of interface based on index
     */
    curr = ifs;
    while (curr != NULL) {
	if (index == curr->index) {
            break;
        }
        curr = curr->next;
    }

    /* if found create a NetworkInterface */
    if (curr != NULL) {;
        obj = createNetworkInterface(env, curr);
    }

    freeif(ifs);
    return obj;
}

/*
 * Class:     java_net_NetworkInterface
 * Method:    getByInetAddress0
 * Signature: (Ljava/net/InetAddress;)Ljava/net/NetworkInterface;
 */
JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByInetAddress0
    (JNIEnv *env, jclass cls, jobject iaObj)
{
    netif *ifs, *curr;
    int family = KAfInet;
    jobject obj = NULL;
    jboolean match = JNI_FALSE;

    jint jfamily = (*env)->GetIntField(env, iaObj, ni_iafamilyID);
    if (jfamily != IPv4) {
	family = KAfInet6;
    }

    ifs = enumInterfaces(env);
    if (ifs == NULL) {
	return NULL;
    }

    curr = ifs;
    while (curr != NULL) {
	netaddr *addrP = curr->addr;

	/*
	 * Iterate through each address on the interface
	 */
	while (addrP != NULL) {

	    if (family == addrP->family) {
		if (family == KAfInet) {
		    int address1 = addrP->ia.Address();
                    int address2 = (*env)->GetIntField(env, iaObj, ni_iaaddressID);

                    if (address1 == address2) {
			match = JNI_TRUE;
			break;
		    }
		}

		if (family == KAfInet6) {
		    const TIp6Addr &ipv6address = addrP->ia.Ip6Address();
		    jbyte *bytes = (jbyte *)ipv6address.u.iAddr8;
		    jbyteArray ipaddress = (*env)->GetObjectField(env, iaObj,
			ni_ia6ipaddressID);
		    jbyte caddr[16];
		    int i;

		    (*env)->GetByteArrayRegion(env, ipaddress, 0, 16, caddr);
		    i = 0;
		    while (i < 16) {
                        if (caddr[i] != bytes[i]) {
			    break;
			}
			i++;
                    }
		    if (i >= 16) {
			match = JNI_TRUE;
			break;
		    }
		}

	    }

	    if (match) {
		break;
	    }
	    addrP = addrP->next;
	}

	if (match) {
	    break;
	}
	curr = curr->next;
    }

    /* if found create a NetworkInterface */
    if (match) {;
        obj = createNetworkInterface(env, curr);
    }

    freeif(ifs);
    return obj;
}


/*
 * Class:     java_net_NetworkInterface
 * Method:    getAll
 * Signature: ()[Ljava/net/NetworkInterface;
 */
JNIEXPORT jobjectArray JNICALL Java_java_net_NetworkInterface_getAll
    (JNIEnv *env, jclass cls)
{
    netif *ifs, *curr;
    jobjectArray netIFArr;
    jint arr_index, ifCount;

    ifs = enumInterfaces(env);
    if (ifs == NULL) {
        return NULL;
    }

    /* count the interface */
    ifCount = 0;
    curr = ifs;
    while (curr != NULL) { 
	ifCount++;
	curr = curr->next;
    } 

    /* allocate a NetworkInterface array */
    netIFArr = (*env)->NewObjectArray(env, ifCount, cls, NULL);
    if (netIFArr == NULL) {
	freeif(ifs);
        return NULL;
    }

    /*
     * Iterate through the interfaces, create a NetworkInterface instance
     * for each array element and populate the object.
     */
    curr = ifs;
    arr_index = 0;
    while (curr != NULL) {
        jobject netifObj;

        netifObj = createNetworkInterface(env, curr);
        if (netifObj == NULL) {
	    freeif(ifs);
            return NULL;
        }

        /* put the NetworkInterface into the array */
        (*env)->SetObjectArrayElement(env, netIFArr, arr_index++, netifObj);

        curr = curr->next;
    }

    freeif(ifs);
    return netIFArr;
}

/*
 * Create a NetworkInterface object, populate the name and index, and
 * populate the InetAddress array based on the IP addresses for this
 * interface.
 */
jobject createNetworkInterface(JNIEnv *env, netif *ifs)
{
    jobject netifObj;
    jobject name;
    jobjectArray addrArr;
    /* eliminate compile time warning
       netaddr *addrs;
    */
    jint addr_index, addr_count;
    netaddr *addrP;

    /*
     * Create a NetworkInterface object and populate it
     */
    netifObj = (*env)->NewObject(env, ni_class, ni_ctrID); 
    name = (*env)->NewStringUTF(env, ifs->name);
    if (netifObj == NULL || name == NULL) {
        return NULL;
    }
    (*env)->SetObjectField(env, netifObj, ni_nameID, name);
    (*env)->SetObjectField(env, netifObj, ni_descID, name); 
    (*env)->SetIntField(env, netifObj, ni_indexID, ifs->index);

    /*
     * Count the number of address on this interface
     */
    addr_count = 0;
    addrP = ifs->addr;
    while (addrP != NULL) {
	addr_count++;
  	addrP = addrP->next;
    }

    /*
     * Create the array of InetAddresses
     */
    addrArr = (*env)->NewObjectArray(env, addr_count,  ni_iacls, NULL);
    if (addrArr == NULL) {
        return NULL;
    }

    addrP = ifs->addr;
    addr_index = 0;
    while (addrP != NULL) {
	jobject iaObj = NULL;

	if (addrP->family == KAfInet) {
            iaObj = (*env)->NewObject(env, ni_ia4cls, ni_ia4ctrID);
            if (iaObj) {
                 (*env)->SetIntField(env, iaObj, ni_iaaddressID, 
		     addrP->ia.Address());
	    }
	}

	if (addrP->family == KAfInet6) {
	    iaObj = (*env)->NewObject(env, ni_ia6cls, ni_ia6ctrID);
	    if (iaObj) {
		jbyteArray ipaddress = (*env)->NewByteArray(env, 16);
		if (ipaddress == NULL) {
		    return NULL;
		}

		const TIp6Addr &ipv6address = addrP->ia.Ip6Address();
		jbyte *caddr = (jbyte *)ipv6address.u.iAddr8;
		
		(*env)->SetByteArrayRegion(env, ipaddress, 0, 16, caddr);
		(*env)->SetObjectField(env, iaObj, ni_ia6ipaddressID,
		    ipaddress);
	    }
	}

	if (iaObj == NULL) {
	    return NULL;
	}

        (*env)->SetObjectArrayElement(env, addrArr, addr_index++, iaObj);
        addrP = addrP->next;
    }
 
    (*env)->SetObjectField(env, netifObj, ni_addrsID, addrArr);

    /* return the NetworkInterface */
    return netifObj;
}

/* 
 * Enumerates all interfaces
 */
static netif *enumInterfaces(JNIEnv *env) {
    netif *ifs;

    /*
     * Enumerate IPv4 addresses
     */
    ifs = enumIPvXInterfaces(env, NULL, KAfInet);
    if (ifs == NULL) {
	if ((*env)->ExceptionOccurred(env)) {
	    return NULL;
	}
    }

    /*
     * If IPv6 is available then enumerate IPv6 addresses.
     */
    if (ipv6_available()) {
        ifs = enumIPvXInterfaces(env, ifs, KAfInet6);

        if ((*env)->ExceptionOccurred(env)) {
            return NULL;
        }
    }

    return ifs;
}


/*
 * Enumerates and returns all IPv4 interfaces
 */
static netif *enumIPvXInterfaces(JNIEnv *env, netif *ifs, TUint family)
{
    TPckgBuf<TSoInetInterfaceInfo> info;
    TSoInetInterfaceInfo &i = info();
    RSocket r;
    _LIT(proto, "ip");
    TInt err = r.Open(sserv, proto);
    if (err != KErrNone) {
	JNU_ThrowByName(env , JNU_JAVANETPKG "SocketException",
			 "Socket creation failed");
	return ifs;
    }

    /*
     * Iterate through each interface
     */
    int idx = 1;
    err = r.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl, 1);
    while ((err = r.GetOpt(KSoInetNextInterface,
	KSolInetIfCtrl, info)) == KErrNone)
    {
#ifdef _UNICODE
	TName n = i.iName;
	TUint8 n8[0x20];
	TPtr8 name8(n8, sizeof n8);
	TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, n);
	fprintf(stderr, "Interface proto %s status %x\n", name8.PtrZ(),
	    i.iState);
{
	TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8, i.iTag);
	fprintf(stderr, "tag %s\n", name8.PtrZ());
}
	const char *if_name = (const char *)name8.PtrZ();
#else
	const char *if_name = (const char *)i.iTag.PtrZ();
#endif
	if (i.iState == EIfUp) {
	    TInetAddr ia = i.iAddress;
	    fprintf(stderr, "Address %x\n", ia.Address());
	}
	if (i.iAddress.Family() == family) {
	    TInetAddr ia = i.iAddress;
	    /*
	     * Add to the list 
	     */
	    ifs = addif(env, ifs, if_name, idx, KAfInet, ia);

	    /*
	     * If an exception occurred then free the list
	     */
	    if ((*env)->ExceptionOccurred(env)) {
		freeif(ifs);
		ifs = NULL;
		goto done;
	    }
	}
	++idx;
    }

{
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
	fprintf(stderr, "Interface %d name %s\n", inum, name8.PtrZ());
#endif
	fprintf(stderr, "src addr %x\n", q.iSrcAddr.Address());
	fprintf(stderr, "dst addr %x\n", q.iDstAddr.Address());
	q.iIndex = ++inum;
    }
}

done:
    r.Close();
    return ifs;
}

/*
 * Free an interface list (including any attached addresses)
 */
void freeif(netif *ifs) {
    netif *currif = ifs;

    while (currif != NULL) {
	netaddr *addrP = currif->addr;
	while (addrP != NULL) {
	    netaddr *next = addrP->next;
	    free(addrP);
	    addrP = next;
	}

	free(currif->name);

	ifs = currif->next;
	free(currif);
	currif = ifs;
    }
}

/*
 * Add an interface to the list. If known interface just link
 * a new netaddr onto the list. If new interface create new
 * netif structure.
 */
netif *addif(JNIEnv *env, netif *ifs, const char *if_name,
    int index, int family, TInetAddr &ia)
{
    netif *currif = ifs;
    netaddr *addrP;
    char *name;

    name = strdup(if_name);
    if (name == NULL) {
	goto out_of_memory;
    }

    /*
     * Create and populate the netaddr node. If allocation fails
     * return an un-updated list.
     */
    addrP = (netaddr *)malloc(sizeof(netaddr));
    if (addrP == NULL) {
	goto out_of_memory0;
    }
    (void) new (&addrP->ia) TInetAddr(ia);
    addrP->family = family;

    /*
     * Check if this is a "new" interface.
     */
    while (currif != NULL) {
        if (strcmp(name, currif->name) == 0) {
	    break;
        }
	currif = currif->next;
    }

    /*
     * If "new" then create an netif structure and
     * insert it onto the list.
     */
    if (currif == NULL) {
	currif = (netif *)malloc(sizeof(netif));
	if (currif == NULL) {
	    goto out_of_memory1;
 	}
	currif->name = name;
	currif->index = index;
	currif->addr = NULL;
	currif->next = ifs;
	ifs = currif;
    }

    /*
     * Finally insert the address on the interface
     */
    addrP->next = currif->addr;
    currif->addr = addrP;

    return ifs;
out_of_memory1:
    free(addrP);
out_of_memory0:
    free(name);
out_of_memory:
    JNU_ThrowOutOfMemoryError(env, "heap allocation failed");
    return ifs;
}
