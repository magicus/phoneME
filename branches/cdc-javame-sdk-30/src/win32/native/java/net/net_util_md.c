/*
 * @(#)net_util_md.c	1.55 06/10/10 
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

#include "javavm/include/porting/ansi/errno.h"
#ifndef WINCE
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif

#include "jni_util.h"
#include "jvm.h"
#include "net_util.h"
#include <tchar.h>

#ifndef IPTOS_TOS_MASK
#define IPTOS_TOS_MASK 0x1e
#endif
#ifndef IPTOS_PREC_MASK
#define IPTOS_PREC_MASK 0xe0
#endif 

#include "java_net_SocketOptions.h"

#include <string.h>
/* true if SO_RCVTIMEO is supported */
jboolean isRcvTimeoutSupported = JNI_TRUE;

void 
NET_ThrowCurrent(JNIEnv *env, char *msg) 
{
    NET_ThrowNew(env, WSAGetLastError(), msg);
}

/*
  These initializations are now down as part of CVMnetStartup and 
  CVMnetShutdown. Also, using DllMain won't work when cvm is built
  as a standalone .exe instead of a dll.
*/
#if 0
/*
 * Initialize Windows Sockets API support
 */
 BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    WSADATA wsadata;

    switch (reason) { 
	case DLL_PROCESS_ATTACH:
	    if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
		return FALSE;
	    }
	    break;

	case DLL_PROCESS_DETACH:
	    WSACleanup();
	    break;

	default:
	    break;
    }
    return TRUE;
}
#endif /* 0 */

/*
 * Since winsock doesn't have the equivalent of strerror(errno)
 * use table to lookup error text for the error.
 */
JNIEXPORT void JNICALL
NET_ThrowNew(JNIEnv *env, int errorNum, char *msg) 
{
    int i;
    //int table_size = sizeof(winsock_errors) /
    //	     sizeof(winsock_errors[0]);
    char exc[256];
    char fullMsg[256];
    char *excP = NULL;

    /*
     * If exception already throw then don't overwrite it.
     */
    if ((*env)->ExceptionOccurred(env)) {
	return;
    }

    /*
     * Default message text if not provided
     */
    if (!msg) {
	msg = "no further information";
    }
    switch(errorNum) {
    case WSAEPROTOTYPE:
        JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			"Invalid protocol type");
	break;
    case WSAENOPROTOOPT:
    case WSAEPROTONOSUPPORT:
    case WSAESOCKTNOSUPPORT:
    case WSAEOPNOTSUPP:
    case WSAEPFNOSUPPORT:
    case WSAEAFNOSUPPORT:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Option unsupported by protocol: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAENOTSOCK:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Descriptor not a socket: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAECONNRESET:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Connection reset by peer: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAECONNABORTED:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Connection aborted by peer: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAEADDRINUSE:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Address in use: %s", msg);
        JNU_ThrowByName(env, JNU_JAVANETPKG "BindException", fullMsg);
	break;
    case WSAEADDRNOTAVAIL:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Cannot assign requested address: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "BindException", fullMsg);
	break;
    case WSAESHUTDOWN:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Connection shutdown: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAEISCONN:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Already connected: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAENOTCONN:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Not connected: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAETIMEDOUT:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Operation timed out: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "ConnectException", fullMsg);
	break;
    case WSAECONNREFUSED:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Connection refused: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "ConnectException", fullMsg);
	break;
    case WSAEHOSTUNREACH:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Host unreachable: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "NoRouteToHostException", fullMsg);
	break;
    case WSAEHOSTDOWN:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "Host down: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAEINPROGRESS:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "blocking winsock call in progress: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAEFAULT:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "argp parameter not part of user address space: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSAENETDOWN:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "network subsystem has failed: %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    case WSANOTINITIALISED:
	jio_snprintf(fullMsg, sizeof(fullMsg),
		     "net dll not initialized (WSAStartup): %s", msg);
	JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", fullMsg);
	break;
    default: {
	    char fullMsg[2048];
	    if (!msg) {
		msg = "Socket error occurred";
	    }
	    
	    jio_snprintf(fullMsg, sizeof(fullMsg), 
			 "%s (code=%d)", msg, GetLastError());
	    JNU_ThrowByName(env, JNU_JAVANETPKG "SocketException", 
			    fullMsg);
	}
    }
    return;
}


void
NET_ThrowSocketException(JNIEnv *env, char* msg) 
{
    static jclass cls = NULL;
    if (cls == NULL) {
	cls = (*env)->FindClass(env, "java/net/SocketException");
	if (cls == NULL) {
	    return;
	}
	cls = (*env)->NewGlobalRef(env, cls);
    }
    (*env)->ThrowNew(env, cls, msg);
}

jfieldID
NET_GetFileDescriptorID(JNIEnv *env)
{
    jclass cls = (*env)->FindClass(env, "java/io/FileDescriptor");
    return (*env)->GetFieldID(env, cls, "fd", "I");
}

jint  IPv6_supported()
{
    return JNI_FALSE;
}


/*
 * Return the default TOS value
 */
int NET_GetDefaultTOS() {
    static int default_tos = -1;
    OSVERSIONINFO ver;
    HKEY hKey;
    LONG ret;

    /*
     * If default ToS already determined then return it
     */
    if (default_tos >= 0) {
	return default_tos;
    }

    /*
     * Assume default is "normal service"
     */	
    default_tos = 0;

    /* 
     * Which OS is this?
     */
    ver.dwOSVersionInfoSize = sizeof(ver);
    GetVersionEx(&ver);

    /*
     * If 2000 or greater then no default ToS in registry
     */
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
	if (ver.dwMajorVersion >= 5) {
	    return default_tos;
	}
    }

    /*
     * Query the registry to see if a Default ToS has been set.
     * Different registry entry for NT vs 95/98/ME.
     */
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		           _T("SYSTEM\\CurrentControlSet\\Services\\Tcp\\Parameters"),
		           0, KEY_READ, (PHKEY)&hKey);
    } else {
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		           _T("SYSTEM\\CurrentControlSet\\Services\\VxD\\MSTCP\\Parameters"), 
		           0, KEY_READ, (PHKEY)&hKey);
    }
    if (ret == ERROR_SUCCESS) {	
	DWORD dwLen;
	DWORD dwDefaultTOS;
	ULONG ulType;
	dwLen = sizeof(dwDefaultTOS);

	ret = RegQueryValueEx(hKey, _T("DefaultTOS"),  NULL, &ulType,
			     (LPBYTE)&dwDefaultTOS, &dwLen);
	RegCloseKey(hKey);
	if (ret == ERROR_SUCCESS) {
	    default_tos = (int)dwDefaultTOS;
        }
    }
    return default_tos;
}



/*
 * Map the Java level socket option to the platform specific
 * level and option name. 
 */
JNIEXPORT int JNICALL
NET_MapSocketOption(jint cmd, int *level, int *optname) {
    
    typedef struct {
	jint cmd;
	int level;
	int optname;
    } sockopts;

    static sockopts opts[] = {
	{ java_net_SocketOptions_TCP_NODELAY,	IPPROTO_TCP, 	TCP_NODELAY },
	{ java_net_SocketOptions_SO_OOBINLINE,	SOL_SOCKET,	SO_OOBINLINE },
	{ java_net_SocketOptions_SO_LINGER,	SOL_SOCKET,	SO_LINGER },
	{ java_net_SocketOptions_SO_SNDBUF,	SOL_SOCKET,	SO_SNDBUF },
	{ java_net_SocketOptions_SO_RCVBUF,	SOL_SOCKET,	SO_RCVBUF },
	{ java_net_SocketOptions_SO_KEEPALIVE,	SOL_SOCKET,	SO_KEEPALIVE },
	{ java_net_SocketOptions_SO_REUSEADDR,	SOL_SOCKET,	SO_REUSEADDR },
	{ java_net_SocketOptions_SO_BROADCAST,	SOL_SOCKET,	SO_BROADCAST },
	{ java_net_SocketOptions_IP_MULTICAST_IF,   IPPROTO_IP,	IP_MULTICAST_IF },
	{ java_net_SocketOptions_IP_MULTICAST_LOOP, IPPROTO_IP, IP_MULTICAST_LOOP },
	{ java_net_SocketOptions_IP_TOS,	    IPPROTO_IP,	IP_TOS },

    };


    int i;

    /*
     * Map the Java level option to the native level 
     */
    for (i=0; i<(int)(sizeof(opts) / sizeof(opts[0])); i++) {
	if (cmd == opts[i].cmd) {
	    *level = opts[i].level;
	    *optname = opts[i].optname;
	    return 0;
	}
    }

    /* not found */
    return -1;
}


/*
 * Wrapper for setsockopt dealing with Windows specific issues :-
 *
 * IP_TOS and IP_MUTICAST_LOOP can't be set on some Windows
 * editions. 
 * 
 * The value for the type-of-service (TOS) needs to be masked
 * to get consistent behaviour with other operating systems.
 */
JNIEXPORT int JNICALL
NET_SetSockOpt(int s, int level, int optname, const void *optval,
	       int optlen)
{   
    int rv;

    if (level == IPPROTO_IP && optname == IP_TOS) {
	int *tos = (int *)optval;
	*tos &= (IPTOS_TOS_MASK | IPTOS_PREC_MASK);
    }

    rv = setsockopt(s, level, optname, optval, optlen);

    if (rv == SOCKET_ERROR) {
	/*
	 * IP_TOS & IP_MULTICAST_LOOP can't be set on some versions
	 * of Windows.
	 */
	if ((WSAGetLastError() == WSAENOPROTOOPT) &&
	    (level == IPPROTO_IP) &&
	    (optname == IP_TOS || optname == IP_MULTICAST_LOOP)) {
	    rv = 0;
	}

	/*
	 * IP_TOS can't be set on unbound UDP sockets.
	 */
	if ((WSAGetLastError() == WSAEINVAL) && 
	    (level == IPPROTO_IP) &&
	    (optname == IP_TOS)) {
	    rv = 0;
	}
    }

    return rv;
}

/*
 * Wrapper for setsockopt dealing with Windows specific issues :-
 *
 * IP_TOS is not supported on some versions of Windows so 
 * instead return the default value for the OS.
 */
JNIEXPORT int JNICALL
NET_GetSockOpt(int s, int level, int optname, void *optval,
	       int *optlen)
{
    int rv;

    rv = getsockopt(s, level, optname, optval, optlen);


    /*
     * IPPROTO_IP/IP_TOS is not supported on some Windows
     * editions so return the default type-of-service
     * value.
     */
    if (rv == SOCKET_ERROR) {

	if (WSAGetLastError() == WSAENOPROTOOPT &&
	    level == IPPROTO_IP && optname == IP_TOS) {

	    int *tos;
	    tos = (int *)optval;
	    *tos = NET_GetDefaultTOS();

	    rv = 0;
	}
    }

    return rv;
}

/*
 * Wrapper for bind winsock call - transparent converts an 
 * error related to binding to a port that has exclusive access
 * into an error indicating the port is in use (facilitates
 * better error reporting).
 */
JNIEXPORT int JNICALL
NET_Bind(int s, struct sockaddr *him, int len)
{
    int rv = bind(s, him, len);

    if (rv == SOCKET_ERROR) {
	/*
	 * If bind fails with WSAEACCES it means that a privileged
	 * process has done an exclusive bind (NT SP4/2000/XP only).
	 */
	if (WSAGetLastError() == WSAEACCES) {
	    WSASetLastError(WSAEADDRINUSE);
	}
    }

    return rv;
}

JNIEXPORT int JNICALL
NET_SocketClose(int fd) {
    int ret;
#ifndef WINCE
    struct linger l;
    int len = sizeof (l);
    if (getsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&l, &len) == 0) {
	if (l.l_onoff == 0) {
	    WSASendDisconnect(fd, NULL);
	}
    }
#endif
    ret = closesocket (fd);
    return ret;
}

JNIEXPORT int JNICALL
NET_Timeout(int fd, long timeout) {
    int ret; 
    fd_set tbl; 
    struct timeval t; 
    t.tv_sec = timeout / 1000; 
    t.tv_usec = (timeout % 1000) * 1000; 
    FD_ZERO(&tbl); 
    FD_SET(fd, &tbl); 
    ret = select (fd + 1, &tbl, 0, 0, &t); 
    return ret;
}

