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
 * Implementation of pcsl_serversocket.h for platforms that support the BSD
 * sockets API.
 *
 * For all functions, the "handle" is the Unix file descriptor (an int)
 * cast to void *.  Since BSD sockets is synchronous, the context is always 
 * set to NULL.
 */

#ifdef ENABLE_SERVER_SOCKET

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
#include <pcsl_serversocket.h>

extern int lastError; /* For pcsl_network_error use. */

#define INVALID_SOCKET       (-1)

#define SOCKET_ERROR         (-1)

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_open(
	int port,
	void **pHandle)
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
        /* printf("setsockopt failed errno=%d\n", errno); */
        (void)close(fd);
        return PCSL_NET_IOERROR;
    }

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    status = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    lastError = errno;

    if (status == -1) {
        /* printf("bind failed errno=%d\n", errno); */
        (void)close(fd);
        return PCSL_NET_IOERROR;
    }


    status = listen(fd, SERVERSOCKET_BACKLOG);

    if (status == 0) {
        *pHandle = na_create(fd);
        return PCSL_NET_SUCCESS;
    }

    /* printf("listen failed errno=%d\n", errno); */
    (void)close(fd);
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_serversocket.h for definition.
 */
int
pcsl_server_socket_open_start(int port, void **pHandle, void **pContext) {
    (void)pContext;
    return pcsl_serversocket_open(port, pHandle);
}


/**
 * See pcsl_serversocket.h for definition.
 */
int
pcsl_server_socket_open_finish(void **handle, void **context) {
    (void)handle;
    (void)context;
    return PCSL_NET_IOERROR;
}

/**
 * Common functionality for accept start and finish
 */
static int pcsl_serversocket_accept_common(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
    int connfd;
    int flags;
    struct sockaddr_in sa;
    int saLen = sizeof (sa);
    int listenfd = na_get_fd(handle);

    connfd = accept(listenfd, (struct sockaddr *)&sa, (socklen_t *)&saLen);

    if (connfd == SOCKET_ERROR) {
        if (errno == EWOULDBLOCK || errno == ECONNABORTED) {
            /*
             * The "listenfd" is marked as non-blocking and no connections 
             * are present to be accepted.
             */
            na_register_for_read((void *)handle);
            *pContext = NULL;
            return PCSL_NET_WOULDBLOCK;
        } else {
            na_unregister_for_read((void *)handle);
            return PCSL_NET_IOERROR;
        }
    } else {
        na_unregister_for_read((void *)handle);
        /*
         * Linux accept does _not_ inherit socket flags like O_NONBLOCK.
         * So, irrespective of the blocking or non-blockimg mode of listenfd,
         * connfd, must be set to non-blocking mode; otherwise VM will 
         * be blocked for any I/O operation on this new socket descriptor.
         */
        flags = fcntl(connfd, F_GETFL, 0);
        fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

        *pConnectionHandle = na_create(connfd);

        return PCSL_NET_SUCCESS;
    }
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_start(
    void *handle,
    void **pConnectionHandle,
    void **pContext) {
    return pcsl_serversocket_accept_common(handle, pConnectionHandle, pContext);
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_finish(
    void *handle,
    void **pConnectionHandle,
    void **pContext) {
    return pcsl_serversocket_accept_common(handle, pConnectionHandle, pContext);
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_sereversocket_close_start(
    void *handle,
    void **pContext)
{
    return pcsl_socket_close_start(handle, pContext);
}


/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_close_finish(
	void *handle,
	void *context)
{
    return pcsl_socket_close_finish(handle, context);
}

#endif /* ifdef ENABLE_SERVER_SOCKET */
