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
 * Implementation of pcsl_socket.h for platforms that support the BSD sockets 
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

#ifdef SOLARIS
#include <sys/filio.h>
#endif

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
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_open_start(
	unsigned char *ipBytes,
	int port,
	void **pHandle,
	void **pContext)
{
    int truebuf = 1;
    int fd = -1;
    struct sockaddr_in addr;
    int status;
    int flags;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    lastError = errno;
    if (fd == INVALID_SOCKET) {
        return PCSL_NET_IOERROR;
    }

    status =
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &truebuf, sizeof(truebuf));
    lastError = errno;

    if (status == -1) {
        /*
         * DEBUG:
         * printf("setsockopt failed errno=%d\n", errno);
         */
        (void)close(fd);
        return PCSL_NET_IOERROR;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);

    memcpy(&addr.sin_addr.s_addr, ipBytes, sizeof(addr.sin_addr.s_addr));

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    status = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    lastError = errno;

    /*
       connect() may return 0 in case of non-blocking sockets if
       server is on the same host as client. No need to create a 
       a blocked handle if connection is successfully established
    */

    if (status == 0) {
        *pHandle = na_create(fd);
        return PCSL_NET_SUCCESS;
    }

    if ((status == SOCKET_ERROR) && 
        (errno == EINPROGRESS || errno == EALREADY)) {
        *pContext = NULL;
        *pHandle = na_create(fd);
        na_register_for_write(*pHandle);
        return PCSL_NET_WOULDBLOCK;
    }

    close(fd);
    return PCSL_NET_CONNECTION_NOTFOUND;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_open_finish(
	void *handle,
	void *context)
{
    int err;
    socklen_t err_size = sizeof(err);

    int fd = na_get_fd(handle);

    int status = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_size);
    /*
     * DEBUG:
     * printf( "pcsl_socket_open_finish::handle=%d err=%d\n", fd, err);
     */

    lastError = err;

    if (err == 0) {
        na_unregister_for_write(handle);
        /*
         * DEBUG:
         * printf("opened fd: %d\n", fd);
         */
        return PCSL_NET_SUCCESS;
    } else {
        na_destroy(handle);
        close(fd);
        return PCSL_NET_IOERROR;
    }
}
      
/**
 * Common implementation between pcsl_socket_read_start() 
 * and pcsl_socket_read_finish().
 */
static int pcsl_socket_read_common(
	void *handle,
	unsigned char *pData,
	int len,  
	int *pBytesRead)
{
    int status;
    int fd;

    if (na_get_status(handle) == PCSL_NET_INTERRUPTED) {
        /*
         * VMSocket status need not be set to any value at this point
         * as the VMSocket is deleted and corresponding BSD socket is 
         * also closed after emitting an interrupted IO exception
         */
        return PCSL_NET_INTERRUPTED;
    }
 
    fd = na_get_fd(handle);

    status = recv(fd, pData, len, 0);
    lastError = errno;


    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            /*
             * DEBUG:
             * printf("read_start from : fd :%d\n", fd);
             */
            return PCSL_NET_WOULDBLOCK;
        } else if (EINTR == errno) {
            return PCSL_NET_INTERRUPTED;
        } else {
            return PCSL_NET_IOERROR;
        }
    }

    if (status == 0) {
        /*
         * DEBUG:
         * printf("Bytes read are 0\n");
         */
    }

    /*
     * DEBUG:
     * printf("read_finish from : fd :%d\n", fd);
     */
    *pBytesRead = status;
    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_read_start(
	void *handle,
	unsigned char *pData,
	int len,  
	int *pBytesRead,
	void **pContext)
{
    int status;

    status = pcsl_socket_read_common(handle, pData, len, pBytesRead);

    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_read(handle);
        *pContext = NULL;
    }

    return status;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_read_finish(
	void *handle,
	unsigned char *pData,
	int len,
	int *pBytesRead,
	void *context)
{
    int status;
  
    status = pcsl_socket_read_common(handle, pData, len, pBytesRead);
    
    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_read(handle);
    } else {        
        na_unregister_for_read(handle);
    }

    return status;
}

/**
 * Common implementation between pcsl_socket_write_start() 
 * and pcsl_socket_write_finish().
 */
static int pcsl_socket_write_common(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten)
{
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

    status = send(fd, pData, len, 0);
    lastError = errno;

    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            /*
             * DEBUG:
             * printf("write_start from : fd :%d\n", fd);
             */
            return PCSL_NET_WOULDBLOCK;
        } else if (EINTR == errno) {
            return PCSL_NET_INTERRUPTED;
        } else {
            return PCSL_NET_IOERROR;
        }
    }

    /*
     * DEBUG:
     * printf("writing : %s : to fd :%d\n", pData, fd);
     * printf("write_finish from : fd :%d\n", fd);
     */

    *pBytesWritten = status;
    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_write_start(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void **pContext)
{
    int status;

    status = pcsl_socket_write_common(handle, pData, len, pBytesWritten);

    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_write(handle);
        *pContext = NULL;
    }

    return status;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_write_finish(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void *context)
{
    int status;

    status = pcsl_socket_write_common(handle, pData, len, pBytesWritten);
    
    if (status == PCSL_NET_WOULDBLOCK) {
        na_register_for_write(handle);
    } else {        
        na_unregister_for_write(handle);
    }
    return status;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_available(void *handle, int *pBytesAvailable)
{
    int fd = na_get_fd(handle);
    int status;

    status = ioctl(fd, FIONREAD, pBytesAvailable);
    lastError = errno;

    if (status < 0) {
        return PCSL_NET_IOERROR;
    } else {
        return PCSL_NET_SUCCESS;
    }
}

/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_shutdown_output(void *handle) {
    int fd = na_get_fd(handle);
    int status;

    status = shutdown(fd, 1);
    lastError = errno;
 
    /* Return value of shutdown() need not be checked */

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_socket.h for definition.
 *
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_socket_close_start(
    void *handle,
    void **pContext)
{
    int status;
    int fd = na_get_fd(handle);
    /*
     * DEBUG:
     * printf("closing fd: %d\n", fd);
     */

    status = close(fd);
    lastError = errno;

    /*
     * If PCSL_NET_WOULDBLOCk is detected, the notification adapter must not be
     * destroyed.
     */
    na_destroy(handle);

    if (status == 0) {
        return PCSL_NET_SUCCESS;
    } 

    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_socket.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_socket_close_finish(
	void *handle,
	void *context)
{
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_getlocaladdr(
	void *handle,
	char *pAddress)
{
    int status;
    struct sockaddr_in sa;
    socklen_t saLen = sizeof (sa);
    int fd = na_get_fd(handle);

    sa.sin_family = AF_INET;

    status = getsockname(fd, (struct sockaddr*)&sa, &saLen);
    lastError = errno;

    if (status < 0) {
        return PCSL_NET_IOERROR;
    }

    /*
     * DEBUG:
     * printf("get ip number port=%d addr=%s\n",  
     *        sa.sin_port, inet_ntoa(sa.sin_addr) );
     */

    strcpy(pAddress, (char *)inet_ntoa(sa.sin_addr));

    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_socket.h for definition.
 */
int pcsl_socket_getremoteaddr(
	void *handle,
	char *pAddress)
{
    int status;
    struct sockaddr_in sa;
    socklen_t saLen = sizeof (sa);
    int fd = na_get_fd(handle);

    sa.sin_family = AF_INET;

    status = getpeername(fd, (struct sockaddr*)&sa, &saLen);
    lastError = errno;

    if (status < 0) {
        return PCSL_NET_IOERROR;
    }

    /*
     * DEBUG:
     * printf("get ip number port=%d addr=%s\n",  
     *        sa.sin_port, inet_ntoa(sa.sin_addr) );
     */

    strcpy(pAddress, (char *)inet_ntoa(sa.sin_addr));

    return PCSL_NET_SUCCESS;
}
