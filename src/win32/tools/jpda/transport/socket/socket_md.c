/*
 * @(#)socket_md.c	1.16 06/10/26
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

/* Adapted from JDK1.2 socket_md.c        1.9 98/06/30 */

/*
 * sysBind, sysSetSockOpt added with implementations based on 
 * PlainSocketImpl v1.18 (see below)
 */

#include <windef.h>
#ifdef WINCE
#include <winsock.h>
#else
#include <winsock2.h>
#endif
#include <stdio.h>
#include <tchar.h>

#include "sysSocket.h"
#include "socketTransport.h"
/*
 * Table of Windows Sockets errors, the specific exception we
 * throw for the error, and the error text.
 *
 * Note that this table excludes OS dependent errors.
 */
static struct {
    int errCode;
    const char *errString;
} const winsock_errors[] = {
    { WSAEPROVIDERFAILEDINIT,	"Provider initialization failed (check %SystemRoot%)" },
    { WSAEACCES, 		"Permission denied" },
    { WSAEADDRINUSE, 		"Address already in use" },
    { WSAEADDRNOTAVAIL, 	"Cannot assign requested address" },
    { WSAEAFNOSUPPORT,		"Address family not supported by protocol family" },
    { WSAEALREADY,		"Operation already in progress" },
    { WSAECONNABORTED,		"Software caused connection abort" },
    { WSAECONNREFUSED,		"Connection refused" },
    { WSAECONNRESET,		"Connection reset by peer" },
    { WSAEDESTADDRREQ,		"Destination address required" },
    { WSAEFAULT,		"Bad address" },
    { WSAEHOSTDOWN,		"Host is down" },
    { WSAEHOSTUNREACH,		"No route to host" },
    { WSAEINPROGRESS,		"Operation now in progress" },
    { WSAEINTR,			"Interrupted function call" },
    { WSAEINVAL,		"Invalid argument" },
    { WSAEISCONN,		"Socket is already connected" },
    { WSAEMFILE,		"Too many open files" },
    { WSAEMSGSIZE,		"The message is larger than the maximum supported by the underlying transport" },
    { WSAENETDOWN,		"Network is down" },
    { WSAENETRESET,		"Network dropped connection on reset" },
    { WSAENETUNREACH,		"Network is unreachable" },
    { WSAENOBUFS,		"No buffer space available (maximum connections reached?)" },
    { WSAENOPROTOOPT,		"Bad protocol option" },
    { WSAENOTCONN,		"Socket is not connected" },
    { WSAENOTSOCK,		"Socket operation on nonsocket" },
    { WSAEOPNOTSUPP,		"Operation not supported" },
    { WSAEPFNOSUPPORT,		"Protocol family not supported" },
    { WSAEPROCLIM,		"Too many processes" },
    { WSAEPROTONOSUPPORT,	"Protocol not supported" },
    { WSAEPROTOTYPE,		"Protocol wrong type for socket" },
    { WSAESHUTDOWN,		"Cannot send after socket shutdown" },
    { WSAESOCKTNOSUPPORT,	"Socket type not supported" },
    { WSAETIMEDOUT,		"Connection timed out" },
    { WSATYPE_NOT_FOUND,	"Class type not found" },
    { WSAEWOULDBLOCK,		"Resource temporarily unavailable" },
    { WSAHOST_NOT_FOUND,	"Host not found" },
#ifndef WINCE
    { WSA_NOT_ENOUGH_MEMORY,	"Insufficient memory available" },
    { WSA_OPERATION_ABORTED,	"Overlapped operation aborted" },
#endif
    { WSANOTINITIALISED,	"Successful WSAStartup not yet performed" },
    { WSANO_DATA,		"Valid name, no data record of requested type" },
    { WSANO_RECOVERY,		"This is a nonrecoverable error" },
    { WSASYSNOTREADY,		"Network subsystem is unavailable" },
    { WSATRY_AGAIN,		"Nonauthoritative host not found" },
    { WSAVERNOTSUPPORTED,	"Winsock.dll version out of range" },
    { WSAEDISCON,		"Graceful shutdown in progress" },
};


/* GetProcAddress takes a Unicode function name parameter on Windows CE.
 * The same function on Windows NT uses ASCII.
 * Windows CE provides an ASCII version of the function, GetProcessAddressA.
 * GetProcessAddressA on Windows CE is equivalent to GetProcessAddress on NT.
 */
#ifdef WINCE
#define GETPROCADDRESS_ASCII GetProcAddressA
#else
#define GETPROCADDRESS_ASCII GetProcAddress
#endif

#ifdef DEBUG
#define sysAssert(expression) {		\
    if (!(expression)) {		\
	    exitTransportWithError \
            ("\"%s\", line %d: assertion failure\n", \
             __FILE__, __DATE__, __LINE__); \
    }					\
}
#else
#define sysAssert(expression) ((void) 0)
#endif

typedef jboolean bool_t;

#include "mutex_md.h"

struct sockaddr;
struct hostent;

#define FN_RECV          0
#define FN_SEND          1
#define FN_LISTEN        2
#define FN_ACCEPT        3
#define FN_RECVFROM      4
#define FN_SENDTO        5
#define FN_SELECT        6
#define FN_CONNECT       7
#define FN_CLOSESOCKET   8
#define FN_SHUTDOWN      9
#define FN_DOSHUTDOWN    10
#define FN_GETHOSTBYNAME 11
#define FN_HTONS         12
#define FN_SOCKET        13
#define FN_WSASENDDISCONNECT 14
#define FN_SOCKETAVAILABLE 15

#define FN_BIND           16         
#define FN_SETSOCKETOPTION 17         
#define FN_GETPROTOBYNAME 18      
#define FN_NTOHS          19
#define FN_HTONL          20
#define FN_GETSOCKNAME    21
#define FN_NTOHL          22

static int (PASCAL FAR *sockfnptrs[])() = 
    {NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL };                   

static bool_t sockfnptrs_initialized = FALSE;
static mutex_t sockFnTableMutex;

/* is Winsock2 loaded? better to be explicit than to rely on sockfnptrs */
static bool_t winsock2Available = FALSE;


/* IMPORTANT: whenever possible, we want to use Winsock2 (ws2_32.dll)
 * instead of Winsock (wsock32.dll). Other than the fact that it is
 * newer, less buggy and faster than Winsock, Winsock2 lets us to work
 * around the following problem:
 *
 * Generally speaking, it is important to shutdown a socket before
 * closing it, since failing to do so can sometimes result in a TCP
 * RST (abortive close) which is disturbing to the peer of the
 * connection.
 * 
 * The Winsock way to shutdown a socket is the Berkeley call
 * shutdown(). We do not want to call it on Win95, since it
 * sporadically leads to an OS crash in IFS_MGR.VXD.  Complete hull
 * breach.  Blue screen.  Ugly.
 *
 * So, in initSockTable we look for Winsock 2, and if we find it we
 * assign wsassendisconnectfn function pointer. When we close, we
 * first check to see if it's bound, and if it is, we call it. Winsock
 * 2 will always be there on NT, and we recommend that win95 user
 * install it.
 *
 * - br 10/11/97
 */

#ifdef WINCE
static struct protoent *
getprotobyname(char *proto)
{
    return NULL;
}
#endif

static void
initSockFnTable() {
    int (PASCAL FAR* WSAStartupPtr)(WORD, LPWSADATA); 
    WSADATA wsadata;

    mutexInit(&sockFnTableMutex);
    mutexLock(&sockFnTableMutex);
    if (sockfnptrs_initialized == FALSE) {
        HANDLE hWinsock;

        /* try to load Winsock2, and if that fails, load Winsock */
        hWinsock = LoadLibrary(_T("ws2_32.dll"));
        if (hWinsock == NULL) {
#ifdef WINCE
            hWinsock = LoadLibrary(_T("winsock"));
#else
            hWinsock = LoadLibrary(_T("wsock32.dll"));
#endif
            winsock2Available = FALSE;
        } else {
            winsock2Available = TRUE;
        }

        if (hWinsock == NULL) {
            fprintf(stderr, "Could not load Winsock 1 or 2 (error: %d)\n", 
                        GetLastError());
        }

        /* If we loaded a DLL, then we might as well initialize it.  */
        WSAStartupPtr = (int (PASCAL FAR *)(WORD, LPWSADATA))
                            GETPROCADDRESS_ASCII(hWinsock, "WSAStartup");
        if (WSAStartupPtr(MAKEWORD(1,1), &wsadata) != 0) {
            fprintf(stderr, "Could not initialize Winsock\n"); 
        }

        sockfnptrs[FN_RECV]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "recv");
        sockfnptrs[FN_SEND]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "send");
        sockfnptrs[FN_LISTEN]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "listen");
        sockfnptrs[FN_ACCEPT]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "accept");
        sockfnptrs[FN_RECVFROM]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "recvfrom");
        sockfnptrs[FN_SENDTO]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "sendto");
        sockfnptrs[FN_SELECT]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "select");
        sockfnptrs[FN_CONNECT]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "connect");
        sockfnptrs[FN_CLOSESOCKET]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "closesocket");
        /* we don't use this */
        sockfnptrs[FN_SHUTDOWN]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "shutdown");
        sockfnptrs[FN_GETHOSTBYNAME]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock,
						   "gethostbyname");
        sockfnptrs[FN_HTONS]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "htons");
        sockfnptrs[FN_SOCKET]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "socket");
        /* in winsock 1, this will simply be 0 */
        sockfnptrs[FN_WSASENDDISCONNECT]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock,
                                                   "WSASendDisconnect");
        sockfnptrs[FN_SOCKETAVAILABLE]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock,
                                                   "ioctlsocket");
        sockfnptrs[FN_BIND]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "bind");
        sockfnptrs[FN_SETSOCKETOPTION]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "setsockopt");
        sockfnptrs[FN_GETPROTOBYNAME]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, 
                                                   "getprotobyname");
        sockfnptrs[FN_NTOHS]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "ntohs");
        sockfnptrs[FN_HTONL]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "htonl");
        sockfnptrs[FN_GETSOCKNAME]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "getsockname");
        sockfnptrs[FN_NTOHL]
            = (int (PASCAL FAR *)())GETPROCADDRESS_ASCII(hWinsock, "ntohl");
    }            

    sysAssert(sockfnptrs[FN_RECV] != NULL);
    sysAssert(sockfnptrs[FN_SEND] != NULL);
    sysAssert(sockfnptrs[FN_LISTEN] != NULL);
    sysAssert(sockfnptrs[FN_ACCEPT] != NULL);
    sysAssert(sockfnptrs[FN_RECVFROM] != NULL);
    sysAssert(sockfnptrs[FN_SENDTO] != NULL);
    sysAssert(sockfnptrs[FN_SELECT] != NULL);
    sysAssert(sockfnptrs[FN_CONNECT] != NULL);
    sysAssert(sockfnptrs[FN_CLOSESOCKET] != NULL);
    sysAssert(sockfnptrs[FN_SHUTDOWN] != NULL);
    sysAssert(sockfnptrs[FN_GETHOSTBYNAME] != NULL);
    sysAssert(sockfnptrs[FN_HTONS] != NULL);
    sysAssert(sockfnptrs[FN_SOCKET] != NULL);
    
    if (winsock2Available) {
        sysAssert(sockfnptrs[FN_WSASENDDISCONNECT] != NULL);
    }

    sysAssert(sockfnptrs[FN_SOCKETAVAILABLE] != NULL);
    sysAssert(sockfnptrs[FN_BIND] != NULL);
    sysAssert(sockfnptrs[FN_SETSOCKETOPTION] != NULL);
#ifdef WINCE
    if (sockfnptrs[FN_GETPROTOBYNAME] == NULL) {
	sockfnptrs[FN_GETPROTOBYNAME] = (int (PASCAL FAR *)())getprotobyname;
    }
#else
    sysAssert(sockfnptrs[FN_GETPROTOBYNAME] != NULL);
#endif
    sysAssert(sockfnptrs[FN_NTOHS] != NULL);
    sysAssert(sockfnptrs[FN_HTONL] != NULL);
    sysAssert(sockfnptrs[FN_GETSOCKNAME] != NULL);
    sysAssert(sockfnptrs[FN_NTOHL] != NULL);

    sockfnptrs_initialized = TRUE;
    mutexUnlock(&sockFnTableMutex);
}

/*
 * If we get a nonnull function pointer it might still be the case
 * that some other thread is in the process of initializing the socket
 * function pointer table, but our pointer should still be good.
 */
int
dbgsysListen(int fd, SOCKINT32 count) {
    int (PASCAL FAR *listenfn)();
    if ((listenfn = sockfnptrs[FN_LISTEN]) == NULL) {
        initSockFnTable();
        listenfn = sockfnptrs[FN_LISTEN];
    }
    sysAssert(sockfnptrs_initialized == TRUE && listenfn != NULL);
    return (*listenfn)(fd, count);
}

int
dbgsysConnect(int fd, struct sockaddr *name, int namelen) {
    int (PASCAL FAR *connectfn)();
    if ((connectfn = sockfnptrs[FN_CONNECT]) == NULL) {
        initSockFnTable();
        connectfn = sockfnptrs[FN_CONNECT];
    }
    sysAssert(sockfnptrs_initialized == TRUE);
    sysAssert(connectfn != NULL);
    return (*connectfn)(fd, name, namelen);
}

int dbgsysFinishConnect(int fd, long timeout) {
    int rv;
    struct timeval t;
    fd_set wr, ex;

    t.tv_sec = timeout / 1000;
    t.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&wr);
    FD_ZERO(&ex);
    FD_SET((unsigned int)fd, &wr);
    FD_SET((unsigned int)fd, &ex);

    rv = select(fd+1, 0, &wr, &ex, &t);
    if (rv == 0) {
	return SYS_ERR;	    /* timeout */
    }

    /*
     * Check if there was an error - this is preferable to check if
     * the socket is writable because some versions of Windows don't
     * report a connected socket as being writable.
     */
    if (!FD_ISSET(fd, &ex)) {
	return SYS_OK;		   
    }

    /*
     * Unable to establish connection - to get the reason we must
     * call getsockopt.
     */
    return SYS_ERR;
}

int
dbgsysAccept(int fd, struct sockaddr *name, int *namelen) {
    int (PASCAL FAR *acceptfn)();
    if ((acceptfn = sockfnptrs[FN_ACCEPT]) == NULL) {
        initSockFnTable();
        acceptfn = sockfnptrs[FN_ACCEPT];
    }
    sysAssert(sockfnptrs_initialized == TRUE && acceptfn != NULL);
    {
	int r = (*acceptfn)(fd, name, namelen);
	if (r == SOCKET_ERROR) {
#ifndef WINCE
	    errno = WSAGetLastError();
#endif
	}
	return r;
    }
}

int
dbgsysRecvFrom(int fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen) {
    int (PASCAL FAR *recvfromfn)();
    if ((recvfromfn = sockfnptrs[FN_RECVFROM]) == NULL) {
        initSockFnTable();
        recvfromfn = sockfnptrs[FN_RECVFROM];
    }
    sysAssert(sockfnptrs_initialized == TRUE && recvfromfn != NULL);
    return (*recvfromfn)(fd, buf, nBytes, flags, from, fromlen);
}

int
dbgsysSendTo(int fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen) {
    int (PASCAL FAR *sendtofn)();
    if ((sendtofn = sockfnptrs[FN_SENDTO]) == NULL) {
        initSockFnTable();
        sendtofn = sockfnptrs[FN_SENDTO];
    }
    sysAssert(sockfnptrs_initialized == TRUE && sendtofn != NULL);
    return (*sendtofn)(fd, buf, len, flags, to, tolen);
}

int
dbgsysRecv(int fd, char *buf, int nBytes, int flags) {
    int (PASCAL FAR *recvfn)();
    if ((recvfn = sockfnptrs[FN_RECV]) == NULL) {
        initSockFnTable();
        recvfn = sockfnptrs[FN_RECV];
    }
    sysAssert(sockfnptrs_initialized == TRUE && recvfn != NULL);
    return (*recvfn)(fd, buf, nBytes, flags);
}

int
dbgsysSend(int fd, char *buf, int nBytes, int flags) {
    int (PASCAL FAR *sendfn)();
    if ((sendfn = sockfnptrs[FN_SEND]) == NULL) {
        initSockFnTable();
        sendfn = sockfnptrs[FN_SEND];
    }
    sysAssert(sockfnptrs_initialized == TRUE && sendfn != NULL);
    return (*sendfn)(fd, buf, nBytes, flags);
}

struct hostent *
dbgsysGetHostByName(char *hostname) {
    int (PASCAL FAR *sendfn)();
    if ((sendfn = sockfnptrs[FN_GETHOSTBYNAME]) == NULL) {
        initSockFnTable();
        sendfn = sockfnptrs[FN_GETHOSTBYNAME];
    }
    sysAssert(sockfnptrs_initialized == TRUE && sendfn != NULL);
    return (struct hostent *)
        (*(struct hostent *(PASCAL FAR *)(char *))sendfn)(hostname);
}

unsigned short
dbgsysHostToNetworkShort(unsigned short hostshort) {
    int (PASCAL FAR *sendfn)();
    if ((sendfn = sockfnptrs[FN_HTONS]) == NULL) {
        initSockFnTable();
        sendfn = sockfnptrs[FN_HTONS];
    }
    sysAssert(sockfnptrs_initialized == TRUE && sendfn != NULL);
    return (*sendfn)((int)hostshort);
}

int
dbgsysSocket(int domain, int type, int protocol) {
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_SOCKET]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_SOCKET];
    }
    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(domain, type, protocol);
}

/*
 * This function is carefully designed to work around a bug in Windows
 * 95's networking winsock. Please see the beginning of this file for
 * a complete description of the problem.
 */
int dbgsysSocketClose(int fd) {

    if (fd > 0) {
        int (PASCAL FAR *closesocketfn)();
        int (PASCAL FAR *wsasenddisconnectfn)();
        int dynamic_ref = -1;

        if ((closesocketfn = sockfnptrs[FN_CLOSESOCKET]) == NULL) {
            initSockFnTable();
        }
        /* At this point we are guaranteed the sockfnptrs are initialized */
            sysAssert(sockfnptrs_initialized == TRUE);

        closesocketfn = sockfnptrs[FN_CLOSESOCKET];
        sysAssert(closesocketfn != NULL);

        if (winsock2Available) {
            wsasenddisconnectfn = sockfnptrs[FN_WSASENDDISCONNECT];
            (void) (*wsasenddisconnectfn)(fd, NULL);
        }
        (void) (*closesocketfn)(fd);
    }
    return TRUE;
}

/*
 * Poll the fd for reading for timeout ms.  Returns 1 if something's
 * ready, 0 if it timed out, -1 on error, -2 if interrupted (although
 * interruption isn't implemented yet).  Timeout in milliseconds.  */
int
dbgsysTimeout(int fd, SOCKINT32 timeout) {
    int res;
    fd_set tbl;
    struct timeval t;
    int (PASCAL FAR *selectfn)();

    t.tv_sec = timeout / 1000;
    t.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&tbl);
    FD_SET((unsigned int)fd, &tbl);

    if ((selectfn = sockfnptrs[FN_SELECT]) == NULL) {
        initSockFnTable();
        selectfn = sockfnptrs[FN_SELECT];
    }
    sysAssert(sockfnptrs_initialized == TRUE && selectfn != NULL); 
    res = (*selectfn)(fd + 1, &tbl, 0, 0, &t);
    return res;
}

SOCKINT32
dbgsysSocketAvailable(int fd, SOCKINT32 *pbytes)
{
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_SOCKETAVAILABLE]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_SOCKETAVAILABLE];
    }
    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(fd, FIONREAD, pbytes);
}

/* Additions to original follow */

int
dbgsysBind(int fd, struct sockaddr *name, int namelen) {
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_BIND]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_BIND];
    }

    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(fd, name, namelen);
}

U_SOCKINT32 
dbgsysInetAddr(const char* cp) {
    return (UINT32)inet_addr(cp);
}

U_SOCKINT32
dbgsysHostToNetworkLong(U_SOCKINT32 hostlong) {
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_HTONL]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_HTONL];
    }

    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(hostlong);
}

unsigned short
dbgsysNetworkToHostShort(unsigned short netshort) {
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_NTOHS]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_NTOHS];
    }

    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(netshort);
}

int
dbgsysGetSocketName(int fd, struct sockaddr *name, int *namelen) {
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_GETSOCKNAME]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_GETSOCKNAME];
    }
    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(fd, name, namelen);
}

U_SOCKINT32
dbgsysNetworkToHostLong(U_SOCKINT32 netlong) {
    int (PASCAL FAR *socketfn)();
    if ((socketfn = sockfnptrs[FN_NTOHL]) == NULL) {
        initSockFnTable();
        socketfn = sockfnptrs[FN_NTOHL];
    }

    sysAssert(sockfnptrs_initialized == TRUE && socketfn != NULL);
    return (*socketfn)(netlong);
}

/*
 * Below Adapted from PlainSocketImpl.c, win32 version 1.18. Changed exception
 * throws to returns of SYS_ERR; we should improve the error codes
 * eventually. Changed java objects to values the debugger back end can
 * more easily deal with. 
 */

int
dbgsysSetSocketOption(int fd, jint cmd, jboolean on, jvalue value) 
{
    int (PASCAL FAR *setsockoptfn)();
    int (PASCAL FAR *getprotofn)();
    if ((setsockoptfn = sockfnptrs[FN_SETSOCKETOPTION]) == NULL) {
        initSockFnTable();
        setsockoptfn = sockfnptrs[FN_SETSOCKETOPTION];
    }
    getprotofn = sockfnptrs[FN_GETPROTOBYNAME];
    sysAssert(sockfnptrs_initialized == TRUE && 
              setsockoptfn != NULL && getprotofn != NULL);

    if (cmd == TCP_NODELAY) {
        struct protoent *proto = (struct protoent *)
            (*(struct protoent *(PASCAL FAR *)(char *))getprotofn)("TCP");
        int tcp_level = (proto == 0 ? IPPROTO_TCP: proto->p_proto);
        long onl = (long)on;

        if ((*setsockoptfn)(fd, tcp_level, TCP_NODELAY,
                       (char *)&onl, sizeof(long)) < 0) {
                return SYS_ERR;
        }
    } else if (cmd == SO_LINGER) {
        struct linger arg;
        arg.l_onoff = on;

        if(on) {
            arg.l_linger = (unsigned short)value.i;
            if((*setsockoptfn)(fd, SOL_SOCKET, SO_LINGER,
                          (char*)&arg, sizeof(arg)) < 0) {
                return SYS_ERR;
            }
        } else {
            if ((*setsockoptfn)(fd, SOL_SOCKET, SO_LINGER,
                           (char*)&arg, sizeof(arg)) < 0) {
                return SYS_ERR;
            }
        }
    } else if (cmd == SO_SNDBUF) {
        jint buflen = value.i;
        if ((*setsockoptfn)(fd, SOL_SOCKET, SO_SNDBUF,
                       (char *)&buflen, sizeof(buflen)) < 0) {
            return SYS_ERR;
        }
    } else if (cmd == SO_REUSEADDR) {
        int oni = (int)on;
        if ((*setsockoptfn)(fd, SOL_SOCKET, SO_REUSEADDR,
                       (char *)&oni, sizeof(oni)) < 0) {
            return SYS_ERR;

        }
    } else {
        return SYS_ERR;
    }
    return SYS_OK;
}

int dbgsysConfigureBlocking(int fd, jboolean blocking) {
    u_long argp;
    int result = 0;

    if (blocking == JNI_FALSE) {
        argp = 1;
    } else {
        argp = 0;
    }
    result = ioctlsocket(fd, FIONBIO, &argp);
    if (result == SOCKET_ERROR) {
	return SYS_ERR;
    } else {
	return SYS_OK;
    }
}

int 
dbgsysPoll(int fd, jboolean rd, jboolean wr, long timeout) {
    int rv;
    struct timeval t;
    fd_set rd_tbl, wr_tbl;

    t.tv_sec = timeout / 1000;
    t.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&rd_tbl);
    if (rd) {
	FD_SET((unsigned int)fd, &rd_tbl);
    }

    FD_ZERO(&wr_tbl);
    if (wr) {
	FD_SET((unsigned int)fd, &wr_tbl);
    }

    rv = select(fd+1, &rd_tbl, &wr_tbl, 0, &t);
    if (rv >= 0) {
	rv = 0;
	if (FD_ISSET(fd, &rd_tbl)) {
	    rv |= DBG_POLLIN;
	}
	if (FD_ISSET(fd, &wr_tbl)) {
	    rv |= DBG_POLLOUT;
	}
    }
    return rv;
}

int 
dbgsysGetLastIOError(char *buf, jint size) {
    int table_size = sizeof(winsock_errors) /
		     sizeof(winsock_errors[0]);
    int i;
    int error = WSAGetLastError();

    /*
     * Check table for known winsock errors
     */
    i=0;
    while (i < table_size) {
	if (error == winsock_errors[i].errCode) {
	    break;
        }
	i++;
    }

    if (i < table_size) {
	strcpy(buf, winsock_errors[i].errString);
    } else {
	sprintf(buf, "winsock error %d", error);
    }
    return 0;
}

int 
dbgsysTlsAlloc() {
    return TlsAlloc();
}

void
dbgsysTlsFree(int index) {
    TlsFree(index);
}

void 
dbgsysTlsPut(int index, void *value) {
    TlsSetValue(index, value);
}

void *
dbgsysTlsGet(int index) {
    return TlsGetValue(index);
}

#define FT2INT64(ft) \
        ((CVMInt64)(ft).dwHighDateTime << 32 | (CVMInt64)(ft).dwLowDateTime)

long
dbgsysCurrentTimeMillis() {
    static long fileTime_1_1_70 = 0;	/* midnight 1/1/70 */
    SYSTEMTIME st0;
    FILETIME   ft0;

    /* initialize on first usage */
    if (fileTime_1_1_70 == 0) {
        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);
    }

    GetSystemTime(&st0);
    SystemTimeToFileTime(&st0, &ft0);

    return (FT2INT64(ft0) - fileTime_1_1_70) / 10000;
}

int
dbgsysInit(JavaVM *jvm)
{
    return JNI_OK;
}
