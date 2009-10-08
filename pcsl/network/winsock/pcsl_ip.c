/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 *
 * Implementation of IP functions in pcsl_network.h for platforms that
 * support the winsock API.
 *
 */

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#include <pcsl_memory.h>
#include <pcsl_network.h>
#include <pcsl_network_notifier.h>
#include <pcsl_socket.h>

/* #include <win32app_export.h>, will a pcsl_event header file later. */
extern HANDLE win32app_get_window_handle();
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

typedef struct _AddrContext {
    struct _AddrContext* next;
    HANDLE asyncHandle;
    /* host must be last because it is variable length. */
    struct hostent host; 
} AddrContext;

/* For use by pcsl_network_error. */
int lastError;

/*
 * To reduce complexity, a whole struct is used a root instead of a just a
 * pointer. The root host field never has any real data.
 */
static AddrContext rootAddrContext;

static void addAddrContext(AddrContext* pContext) {
    pContext->next = rootAddrContext.next;
    rootAddrContext.next = pContext;
}

static AddrContext* removeAddrContext(HANDLE asyncHandle) {
    AddrContext* prev = &rootAddrContext;
    AddrContext* current = rootAddrContext.next;

    // Start after the root, because the root struct has no data.
    while (NULL != current) {
        if (current->asyncHandle == asyncHandle) {
            prev->next = current->next;
            return current;
        }

        prev = current;
        current = current->next;
    }

    return NULL;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_gethostbyname_start(
	char *hostname,
	unsigned char *pAddress, 
	int maxLen,
	int *pLen,
        void **pHandle,
	void **pContext)
{
    AddrContext* pAddrContext;
    HANDLE asyncHandle;

    (void)pAddress;
    (void)pLen;
    (void)maxLen;

    pAddrContext = (AddrContext*)
        pcsl_mem_malloc((sizeof (AddrContext) + MAXGETHOSTSTRUCT));
    if (NULL == pAddrContext) {
        return PCSL_NET_IOERROR;
    }

    asyncHandle = WSAAsyncGetHostByName(win32app_get_window_handle(),
                     WM_HOST_RESOLVED, hostname, (char*)(&pAddrContext->host),
                                        MAXGETHOSTSTRUCT);
    lastError = WSAGetLastError();

    if (0 == asyncHandle) {
        pcsl_mem_free(pAddrContext);
        return PCSL_NET_IOERROR;
    }

    *pHandle = (void*)asyncHandle;
    pAddrContext->asyncHandle = asyncHandle;
    addAddrContext(pAddrContext);
    return PCSL_NET_WOULDBLOCK;
}


/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_network_gethostbyname_finish(
	unsigned char *pAddress,
	int maxLen,
	int *pLen,
        void *handle,
	void *context)
{
    AddrContext* pAddrContext = removeAddrContext((HANDLE)handle);
    int addrLen;

    lastError = (int)context;
    if (NULL == pAddrContext) {
        return PCSL_NET_IOERROR;
    }

    if ((0 != lastError) ||
        (AF_INET != pAddrContext->host.h_addrtype &&
         AF_INET6 != pAddrContext->host.h_addrtype)) {
        pcsl_mem_free(pAddrContext);
        return PCSL_NET_IOERROR;
    }

    addrLen = pAddrContext->host.h_length;
    if (addrLen > maxLen) {
        pcsl_mem_free(pAddrContext);
        return PCSL_NET_INVALID;
    }

    *pLen = addrLen;
    memcpy(pAddress, pAddrContext->host.h_addr, addrLen);
    pcsl_mem_free(pAddrContext);
    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getsockopt(
	void *handle,
	int flag,
	int *pOptval)
{
    SOCKET s = (SOCKET)handle;
    int level = SOL_SOCKET;
    int optname;
    int optsize = sizeof(optname);
    struct linger lbuf ;
    void * opttarget = (void *) pOptval ;

    switch (flag) {
    case 0: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        break;

    case 1: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
        break;

    case 2: /* KEEPALIVE */
        optname = SO_KEEPALIVE;
        break;

    case 3: /* RCVBUF */
        optname = SO_RCVBUF;
        break;

    case 4: /* SNDBUF */
        optname = SO_SNDBUF;
        break;

    default: /* Illegal Argument */
        return PCSL_NET_INVALID;
    }

    if (0 == getsockopt(s, level,  optname, opttarget, &optsize)) {
        if (SO_LINGER == optname) {
            /* If linger is on return the number of seconds. */
            *pOptval = (lbuf.l_onoff == 0 ? 0 : lbuf.l_linger) ;
        }

        lastError = 0;
        return 0;
    }

    lastError = WSAGetLastError();
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */

int pcsl_network_setsockopt(
	void *handle,
	int flag,
	int optval)
{
    SOCKET s = (SOCKET)handle;
    int    level = SOL_SOCKET;
    int    optsize =  sizeof(optval); 
    int    optname;
    struct linger lbuf ;
    void * opttarget = (void *) & optval ;

    switch (flag) {
    case 0: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        break;

    case 1: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
	
        lbuf.l_linger = (unsigned short) optval;
        if (flag <= 0) {
            lbuf.l_onoff = 0;	/* lbuf.l_linger would be ignored */
        } else {
            lbuf.l_onoff = 1;
        }
	
        break;

    case 2: /* KEEPALIVE */
        optname = SO_KEEPALIVE;
        break;

    case 3: /* RCVBUF */
        optname = SO_RCVBUF;
        break;

    case 4: /* SNDBUF */
        optname = SO_SNDBUF;
        /*
         * Linux sets the SO_SND buffer size in TCP to 2048 even if 
         * there is an explicite request to set it to 0 through 
         * setsockopt()	system call. This is because TCP reserves a minimum
         * size buffer for administrative purposes and internal kernel
         * structures.
         * However, in windows, it has been found that buffer size is actually 
         * set to 0 upon setsockopt() with value = 0. It causes one of the TCK
         * tests, viz, SecureConnection0011, to block indefinitely. A 
         * workaround for this problem is to set the SO_SND buffer size to
         * a minimum value like 2048 bytes as in linux. Ideally, it is not
         * advisable to set the buffer sizes to 0 as it causes calling 
         * application process to sleep if there is a need to send TCP data 
         * from this socket.
         */
        if (optval == 0) {
            optval = 2048;
            opttarget = (void *) & optval ;
        }
		
        break;

    default: /* Illegal Argument */
        return PCSL_NET_INVALID;
    }

    if (0 == setsockopt(s, level,  optname, opttarget, optsize)){
        lastError = 0;
		return PCSL_NET_SUCCESS;
    }

    lastError = WSAGetLastError();
    return PCSL_NET_INVALID;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_getlocaladdr(
	void *handle,
	char *pAddress)
{
    SOCKET s = (SOCKET)handle;
    int status;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    sa.sin_family = AF_INET;
    status = getsockname(s, (struct sockaddr*)&sa, &saLen);
    lastError = WSAGetLastError();
    if (SOCKET_ERROR == status) {
        return PCSL_NET_IOERROR;
    }

    /*
     * The string returned from inet_ntoa is only valid until the next
     * winsock API call. The string should not be freed.
     */
    strcpy(pAddress, inet_ntoa(sa.sin_addr));

    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_getremoteaddr(
	void *handle,
	char *pAddress)
{
    SOCKET s = (SOCKET)handle;
    int status;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    sa.sin_family = AF_INET;
    status = getpeername(s, (struct sockaddr*)&sa, &saLen);
    if (SOCKET_ERROR == status) {
        return PCSL_NET_IOERROR;
    }

    /*
     * The string returned from inet_ntoa is only valid until the next
     * winsock API call. The string should not be freed.
     */
    strcpy(pAddress, inet_ntoa(sa.sin_addr));

    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getlocalport(
	void *handle,
	int *pPortNumber)
{
    SOCKET s = (SOCKET)handle;
    int status;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    sa.sin_family = AF_INET;
    status = getsockname(s, (struct sockaddr*)&sa, &saLen);
    if (SOCKET_ERROR == status) {
        return PCSL_NET_IOERROR;
    }

    *pPortNumber = ntohs(sa.sin_port);

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getremoteport(
	void *handle,
	int *pPortNumber)
{
    SOCKET s = (SOCKET)handle;
    int status;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);

    sa.sin_family = AF_INET;
    status = getpeername(s, (struct sockaddr*)&sa, &saLen);
    if (SOCKET_ERROR == status) {
        return PCSL_NET_IOERROR;
    }

    *pPortNumber = ntohs(sa.sin_port);

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_init(void) {
	return pcsl_network_init_start(NULL);
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_init_start(PCSL_NET_CALLBACK pcsl_network_callback) {
    static int netinit = 0;
    static WSADATA wsaData;

    (void)pcsl_network_callback;

    if (!netinit) {
        lastError = 0;
        rootAddrContext.asyncHandle = 0;
        rootAddrContext.next = NULL;

        if (0 != WSAStartup(0x0101, &wsaData )) {
            return PCSL_NET_IOERROR;
        }

        netinit = 1;
    }

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_init_finish(void) {
	return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_finalize_start(PCSL_NET_CALLBACK pcsl_network_callback) {
    (void)pcsl_network_callback;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_finalize_finish(void) {
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_error(
	void *handle)
{
    (void)handle;
    return lastError;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getLocalHostName(
    char *pLocalHost)
{
    if (SOCKET_ERROR == gethostname(pLocalHost, MAX_HOST_LENGTH)) {
        return PCSL_NET_IOERROR;
    }

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getLocalIPAddressAsString(
	char *pLocalIPAddress)
{
    static char hostname[MAX_HOST_LENGTH];
    struct hostent *pHost;
    struct in_addr* pAddr;

    if (SOCKET_ERROR == gethostname(hostname, sizeof (hostname))) {
        lastError = WSAGetLastError();
        return PCSL_NET_IOERROR;
    }

    pHost = gethostbyname(hostname);
    lastError = WSAGetLastError();

    if (NULL == pHost) {
        return PCSL_NET_IOERROR;
    }

    pAddr = (struct in_addr*)(pHost->h_addr);
    strcpy(pLocalIPAddress, inet_ntoa(*pAddr));
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getHostByAddr_start(int ipn, char* host, void **pHandle,
        void **pContext) {
    int h_len;
    unsigned long addr = (unsigned long)ipn;
    struct hostent *hp;

    (void)pContext;

    hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);
    lastError = WSAGetLastError();

    if (hp == NULL) {
        /* No DNS name for this address; convert it to a dotted-quad */
        struct in_addr ipAddress;

        ipAddress.s_addr = ipn;
        strcpy(host, inet_ntoa(ipAddress));
    } else {
        h_len = strlen(hp->h_name);
        if (h_len > MAX_HOST_LENGTH - 1) {
            h_len = MAX_HOST_LENGTH - 1;
        }

        (void)memmove(host, hp->h_name, h_len);
        host[h_len] = 0;
    }

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getHostByAddr_finish(int ipn, char* host, void **pHandle,
        void *context) {
    (void)ipn;
    (void)host;
    (void)pHandle;
    (void)context;
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_addrToString(unsigned char *ipBytes,
        unsigned short** pResult, int* pResultLen) {
    struct in_addr addr;
    char temp[80];
    int tempLen;
    int i;
    
    memcpy(&addr.s_addr, ipBytes, sizeof(addr.s_addr));

    strcpy(temp, inet_ntoa(addr));
    tempLen = strlen(temp);

    *pResult = (unsigned short*)pcsl_mem_malloc(tempLen *
                                               sizeof (unsigned short));
    if (NULL == *pResult) {
        return PCSL_NET_IOERROR;
    }

    for (i = 0; i < tempLen; i++) {
        (*pResult)[i] = temp[i];
    }

    *pResultLen = tempLen;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
void pcsl_add_network_notifier(
    void *handle,
    int event) {
    long lEvent;
    switch (event) {
    case PCSL_NET_CHECK_ACCEPT:
        lEvent = FD_ACCEPT;
        break;
    case PCSL_NET_CHECK_READ:
        lEvent = FD_READ;
        break;
    case PCSL_NET_CHECK_WRITE:
        lEvent = FD_WRITE;
        break;
    case PCSL_NET_CHECK_EXCEPTION:
    default:
        /* need revisit */
        return;
    }
    WSAAsyncSelect((SOCKET)handle, win32app_get_window_handle(),
                   WM_NETWORK, lEvent);
}

/**
 * See pcsl_network.h for definition.
 */
void pcsl_remove_network_notifier(
    void *handle,
    int event) {
    switch (event) {
    case PCSL_NET_CHECK_ACCEPT:
        break;
    case PCSL_NET_CHECK_READ:
        break;
    case PCSL_NET_CHECK_WRITE:
        break;
    case PCSL_NET_CHECK_EXCEPTION:
    default:
        /* need revisit */
        return;
    }
    WSAAsyncSelect((SOCKET)handle, win32app_get_window_handle(),
                   0, 0);
}

/**
 * See pcsl_network.h for definition.
 */
unsigned int pcsl_network_htonl(
    unsigned int value) {
    return htonl(value);
}

/**
 * See pcsl_network.h for definition.
 */
unsigned int pcsl_network_ntohl(
    unsigned int value) {
    return ntohl(value);
}
    
/**
 * See pcsl_network.h for definition.
 */
unsigned short pcsl_network_htons(
    unsigned short value) {
    return htons(value);
}

/**
 * See pcsl_network.h for definition.
 */
unsigned short pcsl_network_ntohs(
    unsigned short value) {
    return ntohs(value);
}
