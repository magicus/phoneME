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
 * Implementation of pcsl_serversocket.h for platforms that support the winsock
 * API.
 *
 * For all functions, the "handle" is the winsock handle (an int)
 * cast to void *.  Since winsock reads and writes to sockets are synchronous,
 * the context for reading and writing is always set to NULL.
 */

#ifdef ENABLE_SERVER_SOCKET

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#include <pcsl_network.h>
#include <pcsl_socket.h>
#include <pcsl_serversocket.h>

/* #include <win32app_export.h>, will a pcsl_event header file later. */
extern HANDLE win32app_get_window_handle();
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

/* For use by pcsl_network_error. */
int lastError;

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_open(
	int port,
	void **pHandle)
{
    int falsebuf  = FALSE;
    struct sockaddr_in addr;
    int fd = -1, res;

    /*
      fprintf(stderr, "server open(%d)\n", port);
    */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) {
        lastError = WSAGetLastError();
        return PCSL_NET_IOERROR;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&falsebuf, sizeof (int));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    res = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (res == SOCKET_ERROR) {
        lastError = WSAGetLastError();
        closesocket(fd);
        return PCSL_NET_IOERROR;
    } else {
        res = listen(fd, SERVERSOCKET_BACKLOG);
        /*
          fprintf(stderr, "listen(%d) = %d\n", fd, res);
        */
        if (res == SOCKET_ERROR) {
            lastError = WSAGetLastError();
            closesocket(fd);
            return PCSL_NET_IOERROR;
        } else {
            unsigned long blockingFlag = 1;

            /* success, set the socket to not block before returning */
            ioctlsocket(fd, FIONBIO, &blockingFlag);
            *pHandle = (void*)fd;
            return PCSL_NET_SUCCESS;
        }
    }

    closesocket(fd);
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
    SOCKET sock;
    struct sockaddr sa;
    int saLen = sizeof (sa);

    sock = accept((SOCKET)handle, &sa, &saLen);
    /*
      fprintf(stderr, "accept(%d) = %d\n", handle, sock);
    */
    if (sock == SOCKET_ERROR) {
        lastError = WSAGetLastError();

        if (lastError == WSAEWOULDBLOCK) {
            WSAAsyncSelect((SOCKET)handle, win32app_get_window_handle(), WM_NETWORK, FD_ACCEPT);
            *pContext = NULL;
            return PCSL_NET_WOULDBLOCK;
        } else {
            sock = PCSL_NET_IOERROR;
        }
    } else {
        unsigned long blockingFlag = 1;

        ioctlsocket(sock, FIONBIO, &blockingFlag);
        *pConnectionHandle = (void*)sock;
    }

    WSAAsyncSelect((SOCKET)handle, win32app_get_window_handle(), 0, 0);
    return PCSL_NET_SUCCESS;
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

#endif
