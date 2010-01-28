/*
 * @(#)NetworkInterface.c	1.26 06/10/10 
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

#include <stdlib.h>
#include <windows.h>
#ifndef WINCE
#include <winsock2.h>		/* needed for htonl */
#endif
#include <iprtrmib.h>
#include <assert.h>
#include <tchar.h>

#include "java_net_InetAddress.h" 
#include "java_net_NetworkInterface.h" 
#include "jni_util.h"

#include "NetworkInterface.h"
#include "net_util.h"
#include "jni_statics.h"

#define ia_addressID	JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia4_ctrID       JNI_STATIC(java_net_Inet4Address, ia4_ctrID)
#define ia_class        JNI_STATIC(java_net_InetAddress, ia_class)

/*
 * Windows implementation of the java.net.NetworkInterface native methods.
 * This module provides the implementations of getAll, getByName, getByIndex,
 * and getByAddress. 
 * 
 * Interfaces and addresses are enumerated using the IP helper routines
 * GetIfTable, GetIfAddrTable resp. These routines are available on Windows
 * 98, NT SP+4, 2000, and XP. They are also available on Windows 95 if 
 * IE is upgraded to 5.x.
 *
 * Windows does not have any standard for device names so we are forced
 * to use our own convention which is based on the normal Unix naming
 * convention ("lo" for the loopback, eth0, eth1, .. for ethernet devices,
 * tr0, tr1, .. for token ring, and so on). This convention gives us
 * consistency across multiple Windows editions and also consistency with
 * Solaris/Linux device names. Note that we always enumerate in index
 * order and this ensures consistent device number across invocations.
 */


/* IP helper library routines */
int (PASCAL FAR *GetIpAddrTable_fn)();
int (PASCAL FAR *GetIfTable_fn)();
int (PASCAL FAR *GetFriendlyIfIndex_fn)();

/* Enumeration routines */
typedef int (*EnumerateNetInterfaces)(JNIEnv *, netif **);
typedef int(*EnumerateNetAddresses)(JNIEnv *, netif *, netaddr **);

static EnumerateNetInterfaces enumInterfaces_fn;
static EnumerateNetAddresses enumAddresses_fn;

#if !defined(_WIN64) && !defined(WINCE)
#define WIN32_9X_SUPPORT
#endif

/* Windows 9x routines are external (not needed on 64-bit) */
#ifdef WIN32_9X_SUPPORT
extern int enumInterfaces_win9x(JNIEnv *, netif **);
extern int enumAddresses_win9x(JNIEnv *, netif *, netaddr **);
extern int init_win9x(void);
#endif


/* Windows 95/98/ME running */
static jboolean isW9x;


/* various JNI ids */
static jclass ni_class;		    /* NetworkInterface */

static jmethodID ni_ctor;	    /* NetworkInterface() */

static jfieldID ni_indexID;	    /* NetworkInterface.index */
static jfieldID ni_addrsID;	    /* NetworkInterface.addrs */
static jfieldID ni_nameID;	    /* NetworkInterface.name */
static jfieldID ni_displayNameID;   /* NetworkInterface.displayName */


/*
 * Support routines to free netif and netaddr lists
 */
void free_netif(netif *netifP) {
    netif *curr = netifP;
    while (curr != NULL) {
	if (curr->name != NULL) 
	    free(curr->name);
	if (curr->displayName != NULL) 
	    free(curr->displayName);
	netifP = netifP->next;
	free(curr);
	curr = netifP;	
    }
}

void free_netaddr(netaddr *netaddrP) {
    netaddr *curr = netaddrP;
    while (curr != NULL) {
	netaddrP = netaddrP->next;
	free(curr);
	curr = netaddrP;	
    }
}

/* 
 * Enumerate network interfaces using IP Helper Library routine GetIfTable.
 * We use GetIfTable rather than other IP helper routines because it's
 * available on 98 & NT SP4+.
 *
 * Returns the number of interfaces found or -1 if error. If no error
 * occurs then netifPP be returned as list of netif structures or NULL
 * if no interfaces are found.
 */
int enumInterfaces_win(JNIEnv *env, netif **netifPP)
{
    MIB_IFTABLE *tableP;
    MIB_IFROW *ifrowP;
    ULONG size;
    DWORD ret;
    int count;
    netif *netifP;
    DWORD i;
    int lo=0, eth=0, tr=0, fddi=0, ppp=0, sl=0, net=0;

    /*
     * Ask the IP Helper library to enumerate the adapters
     */
    size = sizeof(MIB_IFTABLE);
    tableP = (MIB_IFTABLE *)malloc(size);
    ret = (*GetIfTable_fn)(tableP, &size, TRUE);
    if (ret == ERROR_INSUFFICIENT_BUFFER || ret == ERROR_BUFFER_OVERFLOW) {
	tableP = (MIB_IFTABLE *)realloc(tableP, size);
	ret = (*GetIfTable_fn)(tableP, &size, TRUE);
    }

    if (ret != NO_ERROR) {
	if (tableP != NULL) 
	    free(tableP);

#ifdef WIN32_9X_SUPPORT
	if (isW9x && ret == ERROR_NOT_SUPPORTED) {	    
	    /* 
	     * If ERROR_NOT_SUPPORTED is returned on Windows 98 it means that
	     * IE5.0 has been installed. In this case we revert to the Windows 95
	     * approach and avoid using the IP Helper Library.
	     * See: http://support.microsoft.com/support/kb/articles/q234/5/73.asp
	     */
	    enumInterfaces_fn = enumInterfaces_win9x;
	    enumAddresses_fn = enumAddresses_win9x;
	    init_win9x();

	    return (*enumInterfaces_fn)(env, netifPP);
	}
#endif

	JNU_ThrowByName(env, "java/lang/Error", 
		"IP Helper Library GetIfTable function failed");

	return -1;
    }

    /*
     * Iterate through the list of adapters
     */
    count = 0;
    netifP = NULL;

    ifrowP = tableP->table;
    for (i=0; i<tableP->dwNumEntries; i++) {
	char dev_name[8];		   
	netif *curr;
	
	/*
	 * Generate a name for the device as Windows doesn't have any
	 * real concept of a device name.
	 */
	switch (ifrowP->dwType) {
	    case MIB_IF_TYPE_ETHERNET:
		sprintf(dev_name, "eth%d", eth++);
		break;

	    case MIB_IF_TYPE_TOKENRING:
		sprintf(dev_name, "tr%d", tr++);
		break;

	    case MIB_IF_TYPE_FDDI:
		sprintf(dev_name, "fddi%d", fddi++);
		break;
	    
	    case MIB_IF_TYPE_LOOPBACK:
		/* There should only be only IPv4 loopback address */
		if (lo > 0) {
		    continue;
		}
		strcpy(dev_name, "lo");
		lo++;
		break;

	    case MIB_IF_TYPE_PPP:
		sprintf(dev_name, "ppp%d", ppp++);
		break;

	    case MIB_IF_TYPE_SLIP:
		sprintf(dev_name, "sl%d", sl++);
		break;

	    default:
		sprintf(dev_name, "net%d", net++);
	}
						
	/*
	 * Allocate a netif structure and space for the name and 
	 * display name (description in this case).
	 */
	curr = (netif *)calloc(1, sizeof(netif));
	if (curr != NULL) {				    
	    curr->name = (char *)malloc(strlen(dev_name) + 1);	
	    curr->displayName = (char *)malloc(ifrowP->dwDescrLen + 1);

	    if (curr->name == NULL || curr->displayName == NULL) {
		if (curr->name) free(curr->name);
		if (curr->displayName) free(curr->displayName);
		curr = NULL;
	    }
	}
	if (curr == NULL) {
	    JNU_ThrowOutOfMemoryError(env, "heap allocation failure");
	    free_netif(netifP);
	    free(tableP);
	    return -1;
	}	    

	/* 
	 * Populate the interface. Note that we need to convert the
	 * index into its "friendly" value as otherwise we will expose
	 * 32-bit numbers as index values.
	 */
	strcpy(curr->name, dev_name);
	strncpy(curr->displayName, ifrowP->bDescr, ifrowP->dwDescrLen);
	curr->displayName[ifrowP->dwDescrLen] = '\0';
	curr->dwIndex = ifrowP->dwIndex;
	curr->index = (*GetFriendlyIfIndex_fn)(ifrowP->dwIndex);

	/* 
	 * Put the interface at tail of list as GetIfTable(,,TRUE) is
	 * returning the interfaces in index order.
	 */		
	count++;
	if (netifP == NULL) {
	    netifP = curr;
	} else {
	    netif *tail = netifP;
	    while (tail->next != NULL) {
		tail = tail->next;
	    }
	    tail->next = curr;
	}	

	/* onto the next interface */
	ifrowP++;
    }

    /*
     * Free the interface table and return the interface list
     */
    if (tableP) {
	free(tableP);
    }
    *netifPP = netifP;
    return count;
}

/*
 * Enumerate the IP addresses on an interface using the IP helper library 
 * routine GetIfAddrTable and matching based on the index name. There are
 * more efficient routines but we use GetIfAddrTable because it's avaliable
 * on 98 and NT.
 *
 * Returns the count of addresses, or -1 if error. If no error occurs then
 * netaddrPP will return a list of netaddr structures with the IP addresses.
 */
int enumAddresses_win(JNIEnv *env, netif *netifP, netaddr **netaddrPP) 
{
    MIB_IPADDRTABLE *tableP;
    ULONG size;
    DWORD ret;
    DWORD i;
    netaddr *netaddrP;
    int count = 0;

    /*
     * Use GetIpAddrTable to enumerate the IP Addresses
     */
    size = sizeof(MIB_IPADDRTABLE);
    tableP = (MIB_IPADDRTABLE *)malloc(size);

    ret = (*GetIpAddrTable_fn)(&tableP, &size, FALSE);
    if (ret == ERROR_INSUFFICIENT_BUFFER || ret == ERROR_BUFFER_OVERFLOW) {
	tableP = (MIB_IPADDRTABLE *)realloc(tableP, size);
	ret = (*GetIpAddrTable_fn)(tableP, &size, FALSE);
    }
    if (ret != NO_ERROR) {
	if (tableP) {
	    free(tableP);
	}
	JNU_ThrowByName(env, "java/lang/Error", 
		"IP Helper Library GetIpAddrTable function failed");
	return -1;
    }

    /*
     * Iterate through the table to find the addresses with the
     * matching dwIndex. Ignore 0.0.0.0 addresses.
     */
    count = 0;
    netaddrP = NULL;

    i = 0;
    while (i<tableP->dwNumEntries) {
	if (tableP->table[i].dwIndex == netifP->dwIndex &&
	    tableP->table[i].dwAddr != 0) {

	    netaddr *curr = (netaddr *)malloc(sizeof(netaddr));
	    if (curr == NULL) {
		JNU_ThrowOutOfMemoryError(env, "heap allocation failure");
		free_netaddr(netaddrP);
		free(tableP);
		return -1;
	    }

	    curr->addr = htonl(tableP->table[i].dwAddr);
	    curr->next = netaddrP;
	    netaddrP = curr;
	    count++;
	}
	i++;
    }

    *netaddrPP = netaddrP;
    free(tableP);
    return count;
}
 
/*
 * Class:     java_net_NetworkInterface
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_NetworkInterface_init(JNIEnv *env, jclass cls) 
{
    OSVERSIONINFO ver;
    HANDLE h;

    /* 
     * First check if this is a Windows 9x machine.
     */
    ver.dwOSVersionInfoSize = sizeof(ver);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && ver.dwMajorVersion == 4) {
	isW9x = JNI_TRUE;
    }

    /*
     * Try to load the IP Helper Library and obtain the entry points we
     * require. This will succeed on 98, NT SP4+, 2000 & XP. It will
     * fail on Windows 95 (if IE hasn't been updated) and old versions
     * of NT (IP helper library only appeared at SP4). If it fails on
     * Windows 9x we will use the registry approach, otherwise if it
     * fails we throw an Error indicating that we have an incompatible
     * IP helper library.
     */
    h = LoadLibrary(_T("iphlpapi.dll"));
    if (h != NULL) {
#ifdef UNDER_CE
	GetIpAddrTable_fn = 
	    (int (PASCAL FAR *)())GetProcAddress(h, TEXT("GetIpAddrTable"));
	GetIfTable_fn = 
	    (int (PASCAL FAR *)())GetProcAddress(h, TEXT("GetIfTable"));
	GetFriendlyIfIndex_fn = 
	    (int (PASCAL FAR *)())GetProcAddress(h, TEXT("GetFriendlyIfIndex"));
#else
	GetIpAddrTable_fn = 
	    (int (PASCAL FAR *)())GetProcAddress(h, "GetIpAddrTable");
	GetIfTable_fn = 
	    (int (PASCAL FAR *)())GetProcAddress(h, "GetIfTable");
	GetFriendlyIfIndex_fn = 
	    (int (PASCAL FAR *)())GetProcAddress(h, "GetFriendlyIfIndex");
#endif
    }
    if (GetIpAddrTable_fn == NULL ||
	GetIfTable_fn == NULL ||
	GetFriendlyIfIndex_fn == NULL) {

#ifdef WIN32_9X_SUPPORT
	if (isW9x) {
	    /* Use Windows 9x registry approach which requires initialization */
	    enumInterfaces_fn = enumInterfaces_win9x;
	    enumAddresses_fn = enumAddresses_win9x;
	    init_win9x();
	} else 
#endif
	{
	    JNU_ThrowByName(env, "java/lang/Error", 
		"Incompatible IP helper library (iphlpapi.dll)");
	    return;
	}	
    } else {
	enumInterfaces_fn = enumInterfaces_win;
	enumAddresses_fn = enumAddresses_win;
    }	

    /*
     * Get the various JNI ids that we require
     */
    ni_class = (*env)->NewGlobalRef(env, cls);
    CHECK_NULL(ni_class);
    ni_nameID = (*env)->GetFieldID(env, ni_class, "name", "Ljava/lang/String;");
    CHECK_NULL(ni_nameID);
    ni_displayNameID = (*env)->GetFieldID(env, ni_class, "displayName", "Ljava/lang/String;");
    CHECK_NULL(ni_displayNameID);
    ni_indexID = (*env)->GetFieldID(env, ni_class, "index", "I");
    CHECK_NULL(ni_indexID);
    ni_addrsID = (*env)->GetFieldID(env, ni_class, "addrs", "[Ljava/net/InetAddress;");
    CHECK_NULL(ni_addrsID);
    ni_ctor = (*env)->GetMethodID(env, ni_class, "<init>", "()V");
    CHECK_NULL(ni_ctor);

    /* Should pass non-null 2nd arg, but we know it is ignored. */
    Java_java_net_InetAddress_init(env, 0);
}

/*
 * Create a NetworkInterface object, populate the name and index, and
 * populate the InetAddress array based on the IP addresses for this
 * interface.
 */
jobject createNetworkInterface(JNIEnv *env, netif *ifs, int netaddrCount, netaddr *netaddrP)
{
    jobject netifObj;
    jobject name, displayName;
    jobjectArray addrArr;
    netaddr *addrs;
    jint addr_index;    
    jclass inet4Cls;

    /*
     * Create a NetworkInterface object and populate it
     */
    netifObj = (*env)->NewObject(env, ni_class, ni_ctor);
    name = (*env)->NewStringUTF(env, ifs->name);
    displayName = (*env)->NewStringUTF(env, ifs->displayName);
    if (netifObj == NULL || name == NULL || displayName == NULL) {
	return NULL;
    }
    (*env)->SetObjectField(env, netifObj, ni_nameID, name); 
    (*env)->SetObjectField(env, netifObj, ni_displayNameID, displayName);
    (*env)->SetIntField(env, netifObj, ni_indexID, ifs->index);
            
    /*
     * Get the IP addresses for this interface if necessary
     * Note that 0 is a valid number of addresses.
     */
    if (netaddrCount < 0) {
	netaddrCount = (*enumAddresses_fn)(env, ifs, &netaddrP);
	if ((*env)->ExceptionOccurred(env)) {
	    free_netaddr(netaddrP);
	    return NULL;
	}
    }
    addrArr = (*env)->NewObjectArray(env, netaddrCount, ia_class, NULL);
    if (addrArr == NULL) {
	free_netaddr(netaddrP);
	return NULL;
    }
    addrs = netaddrP;
    addr_index = 0;
    while (addrs != NULL) {
        inet4Cls = (*env)->FindClass(env, "java/net/Inet4Address");
        if (inet4Cls != NULL) {
            jobject iaObj = (*env)->NewObject(env, inet4Cls, ia4_ctrID);
	    if (iaObj == NULL) {
                free_netaddr(netaddrP);
	        return NULL;
            }
            /* default ctor will set family to AF_INET */

            /* already in correct byte order */
            (*env)->SetIntField(env, iaObj, ia_addressID, addrs->addr);

            (*env)->SetObjectArrayElement(env, addrArr, addr_index, iaObj);
            addrs = addrs->next;
            addr_index++;
        }
    }
    (*env)->SetObjectField(env, netifObj, ni_addrsID, addrArr);

    free_netaddr(netaddrP);

    /* return the NetworkInterface */
    return netifObj;
}

/*
 * Class:     java_net_NetworkInterface
 * Method:    getByName0
 * Signature: (Ljava/lang/String;)Ljava/net/NetworkInterface;
 */
JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByName0
    (JNIEnv *env, jclass cls, jstring name)
{
    netif *ifList, *curr;
    jboolean isCopy;
    const char *name_utf; 
    jobject netifObj = NULL;

    /* get the list of interfaces */
    if ((*enumInterfaces_fn)(env, &ifList) < 0) {
	return NULL;
    }

    /* get the name as a C string */
    name_utf = (*env)->GetStringUTFChars(env, name, &isCopy);

    /* Search by name */
    curr = ifList;
    while (curr != NULL) {
	if (strcmp(name_utf, curr->name) == 0) {
	    break;
	}
	curr = curr->next;
    }
 
    /* if found create a NetworkInterface */
    if (curr != NULL) {;
	netifObj = createNetworkInterface(env, curr, -1, NULL);
    }

    /* release the UTF string */
    (*env)->ReleaseStringUTFChars(env, name, name_utf);

    /* release the interface list */
    free_netif(ifList);

    return netifObj;
}

/*
 * Class:     NetworkInterface
 * Method:    getByIndex
 * Signature: (I)LNetworkInterface;
 */
JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByIndex
  (JNIEnv *env, jclass cls, jint index)
{
    netif *ifList, *curr;
    jobject netifObj = NULL;

    /* get the list of interfaces */
    if ((*enumInterfaces_fn)(env, &ifList) < 0) {
	return NULL;
    }

    /* search by index */
    curr = ifList;
    while (curr != NULL) {
	if (index == curr->index) {
	    break;
	}
	curr = curr->next;
    }

    /* if found create a NetworkInterface */ 
    if (curr != NULL) {
	netifObj = createNetworkInterface(env, curr, -1, NULL);
    }

    /* release the interface list */
    free_netif(ifList);

    return netifObj;
}

/*
 * Class:     java_net_NetworkInterface
 * Method:    getByInetAddress0
 * Signature: (Ljava/net/InetAddress;)Ljava/net/NetworkInterface;
 */
JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByInetAddress0
    (JNIEnv *env, jclass cls, jobject iaObj)
{    
    netif *ifList, *curr;
    jint addr = (*env)->GetIntField(env, iaObj, ia_addressID);
    jobject netifObj = NULL;

    /* get the list of interfaces */
    if ((*enumInterfaces_fn)(env, &ifList) < 0) {
	return NULL;
    }

    /* 
     * Enumerate the addresses on each interface until we find a
     * matching address.
     */
    curr = ifList;
    while (curr != NULL) {
	int count;
	netaddr *addrList;
	netaddr *addrP;
	
	/* enumerate the addresses on this interface */
	count = (*enumAddresses_fn)(env, curr, &addrList);
	if (count < 0) {
	    free_netif(ifList);
	    return NULL;
	}
    
	/* iterate through each address */
	addrP = addrList;

	while (addrP != NULL) {	    
	    /* already in correct byte order */
	    if ((unsigned long)addr == addrP->addr) {
		break;
	    }
	    addrP = addrP->next;
	}

	/*
	 * Address matched so create NetworkInterface for this interface
	 * and address list.
	 */
	if (addrP != NULL) {
	    /* createNetworkInterface will free addrList */
	    netifObj = createNetworkInterface(env, curr, count, addrList);	    
	    break;
	}

	/* on next interface */
	curr = curr->next;
    }
    
    /* release the interface list */
    free_netif(ifList);

    return netifObj;
}

/*
 * Class:     java_net_NetworkInterface
 * Method:    getAll
 * Signature: ()[Ljava/net/NetworkInterface;
 */
JNIEXPORT jobjectArray JNICALL Java_java_net_NetworkInterface_getAll
    (JNIEnv *env, jclass cls)
{
    int count;
    netif *ifList, *curr;
    jobjectArray netIFArr;
    jint arr_index;   

    /*
     * Get list of interfaces
     */
    count = (*enumInterfaces_fn)(env, &ifList);
    if (count < 0) {
	return NULL;
    }  

    /* allocate a NetworkInterface array */
    netIFArr = (*env)->NewObjectArray(env, count, cls, NULL);
    if (netIFArr == NULL) {
        return NULL;
    }

    /*
     * Iterate through the interfaces, create a NetworkInterface instance
     * for each array element and populate the object.
     */
    curr = ifList;
    arr_index = 0;
    while (curr != NULL) {
	jobject netifObj;

	netifObj = createNetworkInterface(env, curr, -1, NULL);
	if (netifObj == NULL) {
	    return NULL;
	}

	/* put the NetworkInterface into the array */
	(*env)->SetObjectArrayElement(env, netIFArr, arr_index++, netifObj);	

	curr = curr->next;
    }

    /* release the interface list */
    free_netif(ifList);

    return netIFArr;
}
