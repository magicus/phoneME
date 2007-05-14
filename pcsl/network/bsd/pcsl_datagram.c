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
 * Implementation of pcsl_datagram.h for platforms that support the BSD sockets 
 * API.
 *
 * For all functions, the "handle" is the Unix file descriptor (an int)
 * cast to void *.  Since BSD sockets is synchronous, the context is always 
 * set to NULL.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <pcsl_network.h>
#include <pcsl_network_na.h>

extern int lastError; /* For pcsl_network_error use. */

#define INVALID_SOCKET (-1)

#define SOCKET_ERROR   (-1)

/**
 * See pcsl_datagram.h for definition.
 */
int
pcsl_datagram_open_start(
    int port, 
    void **pHandle, 
    void **pContext) 
{
    struct sockaddr_in addr;
    int fd;
    int res;
    int i;
	int flags;
    int result; 

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    lastError = errno;
    if (fd == INVALID_SOCKET) {
        return PCSL_NET_IOERROR;
    }

    i = port;

   /*
    * Here is a total mystery to me. If I give solaris a port number of zero
    * then it allocated one somewhere above 50000. The problem is that when I
    * do a recvfrom() on a socket that is bound to a port > 32767 it never sees
    * any data.
    *
    * Temporary solution (seems to work all the time):
    * Start at port 6000 and just go upwards until a free port is found
    */
    if (i <= 0) {
        i = 6000;
    }

    res = EADDRINUSE;
    do {
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons((short)i++);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        res = bind(fd, (struct sockaddr*)&addr, sizeof( addr ));
        lastError = errno;
    } while ((res == SOCKET_ERROR) && (errno == EADDRINUSE) && 
             (port == 0) && (i < 32767));

    if (res != SOCKET_ERROR) {
        *pHandle = na_create(fd);
       
        /* 
        printf("creating SMS datagram handle, handle = %d, fd = %d, 
                port = %d\n", (int)(*pHandle), fd, (i-1));
        */
 
        /* Set the datagram socket to non-blocking mode */
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        return PCSL_NET_SUCCESS;
    }

    close(fd);
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_datagram.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_datagram_open_finish(
    void *handle,
    void *context)
{
    return PCSL_NET_INVALID;
}

/**
 * Common implementation between pcsl_datagram_read_start() 
 * and pcsl_datagram_read_finish().
 */
static int pcsl_datagram_read_common(
    void *handle, 
    unsigned char *pAddress,
    int *port, 
    char *buffer, 
    int length, 
    int *pBytesRead)
{
    struct sockaddr_in addr;
    int len = sizeof(struct sockaddr_in);
    int status;
    int fd;

    if (na_get_status(handle) == PCSL_NET_INTERRUPTED) {
        /*
         * VMSocket status need not be set to any value at this point
         * as the VMSocket is deleted and corresponding BSD socket is 
         * also closed
         */
        return PCSL_NET_INTERRUPTED;
    }

    fd = na_get_fd(handle);

    status = recvfrom(fd, buffer, length, 0, (struct sockaddr*)&addr, 
                   (socklen_t *)&len);
    lastError = errno;

    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            /* printf("datagram read_start from : fd :%d\n", fd); */
            return PCSL_NET_WOULDBLOCK;
        } else if (EINTR == errno) {
            return PCSL_NET_INTERRUPTED;
        } else {
            return PCSL_NET_IOERROR;
        }
    }

    memcpy(pAddress, &addr.sin_addr.s_addr, sizeof(pAddress)); 
    //*ipnumber = (long)addr.sin_addr.s_addr;
    *port     = ntohs(addr.sin_port);
    *pBytesRead = status;

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_read_start(
    void *handle, 
    unsigned char *pAddress,
    int *port, 
    char *buffer, 
    int length, 
    int *pBytesRead,
    void **pContext)
{
    int status;

    status = pcsl_datagram_read_common(handle, pAddress, port,
                buffer, length, pBytesRead);

    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_read(handle);
        *pContext = NULL;
    }

    return status;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_read_finish(
    void *handle, 
    unsigned char *pAddress,
    int *port, 
    char *buffer, 
    int length, 
    int *pBytesRead,
	void *context)
{
    int status;
  
    status = pcsl_datagram_read_common(handle, pAddress, port,
                buffer, length, pBytesRead);
    
    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_read(handle);
    } else {        
        na_unregister_for_read(handle);
    }

    return status;
}

/**
 * Common implementation between pcsl_datagram_write_start() 
 * and pcsl_datagram_write_finish().
 */
static int pcsl_datagram_write_common(
    void *handle,
    unsigned char *ipBytes,
    int port, 
    char *buffer, 
    int length, 
	int *pBytesWritten)
{
    struct sockaddr_in addr;
    int status;
    int fd;

    if (na_get_status(handle) == PCSL_NET_INTERRUPTED) {
        /*
         * VMSocket status need not be set to any value at this point
         * as the VMSocket is deleted and corresponding BSD socket is 
         * also closed
         */
        return PCSL_NET_INTERRUPTED;
    }

    fd = na_get_fd(handle);

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((short)port);
    memcpy(&addr.sin_addr.s_addr, ipBytes, sizeof(addr.sin_addr.s_addr));

    status = sendto(fd, buffer, length, 0, (struct sockaddr*)&addr,
                    sizeof(addr));
    lastError = errno;

    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            /* printf("write_start from : fd :%d\n", fd); */
            return PCSL_NET_WOULDBLOCK;
        } else if (EINTR == errno) {
            return PCSL_NET_INTERRUPTED;
        } else {
            return PCSL_NET_IOERROR;
        }
    }

    *pBytesWritten = status;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_write_start(
	void *handle,
    unsigned char *ipBytes,
	int port,
    char *buffer,
    int length,
	int *pBytesWritten,
	void **pContext)
{
    int status;

    status = pcsl_datagram_write_common(handle, ipBytes, port, buffer, 
                length, pBytesWritten);

    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_write(handle);
        *pContext = NULL;
    }

    return status;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_write_finish(
	void *handle,
    unsigned char *ipBytes,
	int port,
    char *buffer,
    int length,
	int *pBytesWritten,
	void *context)
{
    int status;

    status = pcsl_datagram_write_common(handle, ipBytes, port, buffer, 
                length, pBytesWritten);
    
    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_write(handle);
    } else {        
        na_unregister_for_write(handle);
    }
    return status;
}

/**
 * See pcsl_datagram.h for definition.
 *
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_datagram_close_start(
    void *handle,
    void **pContext)
{
    int status;
    int fd = na_get_fd(handle);
    /* printf("closing fd: %d\n", fd); */

    (void)pContext;
    na_destroy(handle);
    status = close(fd);
    lastError = errno;

    if (status == 0) {
        return PCSL_NET_SUCCESS;
    } 

    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_datagram.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_datagram_close_finish(
	void *handle,
	void *context)
{
    (void)handle;
    (void)context;
    return PCSL_NET_INVALID;
}

