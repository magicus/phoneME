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
 * Implementation of pcsl_network.h for platforms that support the BSD sockets 
 * API.
 *
 * For all functions, the "handle" is the Unix file descriptor (an int)
 * cast to void *.  Since BSD sockets is synchronous, the context is always 
 * set to NULL.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef SOLARIS
#include <sys/sockio.h>
#endif

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#include <pcsl_memory.h>

#include <pcsl_network.h>
#include <pcsl_network_na.h>
#include <pcsl_network_notifier.h>

int lastError; /* For pcsl_network_error use. */

/**
 * Returns name of network interface which is meant as a primary
 * way of connecting to this device from an external system.
 * On most linux systems this would be "eth0"
 * 
 * See javax.microedition.io.ServerSocketConnection.getLocalAddress()
 */
static char *pcsl_network_getMainIFName() {
    char* pName = getenv(ENV_VAR_WITH_NETWORK_IF_NAME);
    return pName ? pName : "eth0";
}

/**
 * See pcsl_network.h for definition.
 * 
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_network_gethostbyname_start(
	char *hostname,
	unsigned char *pAddress, 
	int maxLen,
	int *pLen,
	void **pHandle,
	void **pContext)
{
    struct hostent *hp;
    int realLen;

    (void)pHandle;
    (void)pContext;

    hp = gethostbyname(hostname);
    lastError = errno;
    if (NULL == hp) {
        return PCSL_NET_IOERROR;
    }

    if (hp->h_addrtype == AF_INET) {
        realLen = 4;	/* IPv4 */
    } else if (hp->h_addrtype == AF_INET6) {
        realLen = 16;   /* IMPL NOTE - IPv6 not supported, yet. */
    } else {
        return PCSL_NET_IOERROR;
    }

    if (realLen > maxLen) {
        return PCSL_NET_INVALID;
    }

    memcpy(pAddress, hp->h_addr_list[0], realLen);
    *pLen = realLen;
    return PCSL_NET_SUCCESS;
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
    /*
     * DEBUG:
     * printf("pcsl_socket_gethostbyname_finish called!\n");
     */
    (void)handle;
    (void)context;
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_socket.h for definition.
 */
int pcsl_network_getsockopt(
	void *handle,
	int flag,
	int *pOptval)
{
    int level = SOL_SOCKET;
    int optname;
    socklen_t optsize = sizeof(optname);
    struct linger lbuf ;
    void * opttarget = (void *) pOptval ;

    int fd = na_get_fd(handle);

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
    default:
        return PCSL_NET_INVALID;
    }

    if (getsockopt(fd, level,  optname, opttarget, &optsize) == 0 ) {
        lastError = 0;

        if (optname == SO_LINGER) {
            /* If linger is on return the number of seconds. */
            *pOptval = (lbuf.l_onoff == 0 ? 0 : lbuf.l_linger) ;
        }
        /*
         * DEBUG:
         * printf("getSockopt option=%d value=%d size=%d\n", 
         *	  optname, *pOptval, optsize);
         */
        return PCSL_NET_SUCCESS;
    } else {
        lastError = errno;
        /*
         * DEBUG: 
         * printf("getsockopt errno=%d option=%d level=%d\n", 
         *        errno, optname, level );
         */
        return PCSL_NET_IOERROR;
    }
}

/**
 * See pcsl_socket.h for definition.
 */
int pcsl_network_setsockopt(
	void *handle,
	int flag,
	int optval)
{
    int    level = SOL_SOCKET;
    int    optsize =  sizeof(optval);
    int    optname;
    struct linger lbuf ;
    void * opttarget = (void *) & optval ;

    int fd = na_get_fd(handle);

    switch (flag) { 
    case 0: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        break;
    case 1: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
        if (optval == 0) {
            lbuf.l_onoff = 0;
            lbuf.l_linger = 0;
        } else {
            lbuf.l_onoff = 1;
            lbuf.l_linger = optval;
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
        break;
    default:
        return PCSL_NET_INVALID;
    }

    /*
     * DEBUG: 
     * printf("setSockopt option=%d value=%d\n", optname, optval);
     */

    if (setsockopt(fd, level,  optname, opttarget, optsize) != 0) {
        lastError = errno;
        /*
         * DEBUG: 
         * printf("setsockopt errno=%d\n", errno);
         */
        return PCSL_NET_INVALID;
    }

    lastError = 0;
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
    (void)pcsl_network_callback;
    lastError = 0;
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
    return lastError;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getLocalHostName(
    char *pLocalHost)
{
    if (gethostname(pLocalHost, MAX_HOST_LENGTH) == 0) {
        lastError = 0;
        return PCSL_NET_SUCCESS;
    }

    lastError = errno;
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
#ifdef SOLARIS
int pcsl_network_getLocalIPAddressAsString(
        char *pLocalIPAddress)
{
    int sfd; 
    int i; 
    struct lifnum ifnum;
    struct lifconf ifconf;
    struct lifreq *ifr;
    struct sockaddr_in *sin;

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        lastError = errno; 
        return PCSL_NET_IOERROR;
    }

    /* use AF_INET for IPv4 only, switch to AF_UNSPEC to enable IPv6 */
    ifnum.lifn_family = AF_INET;
    ifnum.lifn_flags = 0;
    if (ioctl(sfd, SIOCGLIFNUM, &ifnum) != 0) { 
        lastError = errno; 
        return PCSL_NET_IOERROR;
    }

    memset(&ifconf, 0, sizeof(ifconf)); 
 
    ifconf.lifc_family = AF_INET;
    ifconf.lifc_len = sizeof(struct lifreq) * ifnum.lifn_count;
    ifconf.lifc_buf = calloc(ifnum.lifn_count, sizeof(struct lifreq));

    if (ioctl(sfd, SIOCGLIFCONF, &ifconf) != 0) { 
        lastError = errno; 
        return PCSL_NET_IOERROR;
    }

    for (i = 0; i < ifnum.lifn_count; i++) {
        ifr = ifconf.lifc_req + i;
        sin = (struct sockaddr_in *)&ifr->lifr_addr; 
        if (strcmp("lo0", ifr->lifr_name)) {
            strcpy(pLocalIPAddress, inet_ntoa(sin->sin_addr)); 
            break;
        }
    }

    free(ifconf.lifc_buf);
    return PCSL_NET_SUCCESS;
}
#else
int pcsl_network_getLocalIPAddressAsString(
        char *pLocalIPAddress)
{
    int sfd; 
    int i; 
    struct ifreq ifr; 
    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr; 

    memset(&ifr, 0, sizeof ifr); 
 
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        lastError = errno; 
        return PCSL_NET_IOERROR;
    }

    strcpy(ifr.ifr_name, pcsl_network_getMainIFName()); 
    sin->sin_family = AF_INET; 

    if (ioctl(sfd, SIOCGIFADDR, &ifr) != 0) { 
        lastError = errno; 
        return PCSL_NET_IOERROR;
    }

    strcpy(pLocalIPAddress, inet_ntoa(sin->sin_addr)); 

    return PCSL_NET_SUCCESS;
}
#endif

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getHostByAddr_start(
    int ipn, 
    char* host,
    void **pHandle,
    void **pContext)
{
    int h_len;
    unsigned long addr = (unsigned long)ipn;
    struct hostent *hp;

    hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);
    lastError = errno;

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

	/*
         * DEBUG: 
         * printf("getHostByAddr ipaddr='%lx' host='%s'\n", ipn, host);
         */

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getHostByAddr_finish(
    int ipn, 
    char* host,
    void **pHandle,
    void *context)
{
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getRawIpNumber(
    unsigned char* ipBytes) {
    return ((struct in_addr*)ipBytes)->s_addr;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getlocalport(
	void *handle,
	int *pPortNumber)
{
    int status;
    int localPort;
    struct sockaddr_in sa;
    socklen_t saLen = sizeof (sa);
    int fd = na_get_fd(handle);

    status = getsockname(fd, (struct sockaddr*)&sa, &saLen);
    lastError = errno;

    if (status < 0) {
        return PCSL_NET_IOERROR;
    }

    /*
     * DEBUG: 
     * printf("get ip number family=%x port=%d addr=%s\n",  
     *        sa.sin_family, sa.sin_port, inet_ntoa(sa.sin_addr) );
     */

    localPort = ntohs(sa.sin_port);
    *pPortNumber = localPort;

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getremoteport(
	void *handle,
	int *pPortNumber)
{
    int status;
    int peerPort;
    struct sockaddr_in sa;
    socklen_t saLen = sizeof (sa);
    int fd = na_get_fd(handle);

    status = getpeername(fd, (struct sockaddr*)&sa, &saLen);
    lastError = errno;

    if (status < 0) {
        return PCSL_NET_IOERROR;
    }

    /*
     * DEBUG: 
     * printf("get ip number family=%x port=%d addr=%s\n",  
     *        sa.sin_family, sa.sin_port, inet_ntoa(sa.sin_addr) );
     */

    peerPort =  (int) ntohs(sa.sin_port);
    *pPortNumber = peerPort;

    return PCSL_NET_SUCCESS;
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
    switch (event) {
    case PCSL_NET_CHECK_READ:
    case PCSL_NET_CHECK_ACCEPT:
        na_register_for_read(handle);
        break;
    case PCSL_NET_CHECK_WRITE:
        na_register_for_write(handle);
        break;
    case PCSL_NET_CHECK_EXCEPTION:
        /* need revisit */
        break;
    }
}

/**
 * See pcsl_network.h for definition.
 */
void pcsl_remove_network_notifier(
    void *handle,
    int event) {
    switch (event) {
    case PCSL_NET_CHECK_READ:
    case PCSL_NET_CHECK_ACCEPT:
        na_unregister_for_read(handle);
        break;
    case PCSL_NET_CHECK_WRITE:
        na_unregister_for_write(handle);
        break;
    case PCSL_NET_CHECK_EXCEPTION:
        /* need revisit */
        break;
    }
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

/**
 * See pcsl_network.h for definition.
 */
char * pcsl_inet_ntoa (void *ipBytes) {
    return inet_ntoa(*((struct in_addr*)ipBytes));
}
