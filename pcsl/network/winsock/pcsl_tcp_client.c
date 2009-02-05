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
 * Implementation of pcsl_network.h for platforms that support the winsock 
 * API.
 *
 * For all functions, the "handle" is the winsock handle (an int)
 * cast to void *.  Since winsock reads and writes to sockets are synchronous,
 * the context for reading and writing is always set to NULL.
 */

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#include <pcsl_network.h>

/* #include <win32app_export.h>, will a pcsl_event header file later. */
extern HANDLE win32app_get_window_handle();
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

/* For use by pcsl_network_error. */
int lastError;

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_open_start(
	unsigned char *ipBytes,
	int port,
	void **pHandle,
	void **pContext)
{
    SOCKET s;
    int falsebuf = 0;
    int status;
    struct sockaddr_in addr;
    unsigned long blockingFlag = 1;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == s) {
        return PCSL_NET_IOERROR;
    }

    status = setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                        (char*)&falsebuf, sizeof (falsebuf));
    lastError = WSAGetLastError();
    if (SOCKET_ERROR == status) {
        (void)closesocket(s);
        return PCSL_NET_IOERROR;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);

    memcpy(&addr.sin_addr.s_addr, ipBytes, sizeof(addr.sin_addr.s_addr));

    /* Note: WSAAsyncSelect sets the socket to non-blocking mode. */
    WSAAsyncSelect(s, win32app_get_window_handle(), WM_NETWORK, FD_CONNECT);

    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    lastError = WSAGetLastError();

    *pHandle = (void*)s;
    *pContext = NULL;

    if (0 == status) {
        /*
         * connect() returned 0, this because the server is on the same host
         * and the loopback address was used to connect. However there is
         * a connect notification waiting in the message queue to unblock the
         * caller, so tell the caller to block.
         */
        return PCSL_NET_WOULDBLOCK;
    }

    if (WSAEWOULDBLOCK == lastError) {
        return PCSL_NET_WOULDBLOCK;
    }

    closesocket(s);

    return PCSL_NET_CONNECTION_NOTFOUND;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_open_finish(
	void *handle,
	void *context)
{
    SOCKET s = (SOCKET)handle;
    int status = (int)context;

    /* Reset Window's message notification for this connection. */
    WSAAsyncSelect(s, win32app_get_window_handle(), 0, 0);

    lastError = status;
    if (0 == status) {
        return PCSL_NET_SUCCESS;
    }

    closesocket(s);
    return PCSL_NET_IOERROR;
}
      

/**
 * Common implementation between read_start() and read_finish().
 */
static int winsock_read_common(
	void *handle,
	unsigned char *pData,
	int len,  
	int *pBytesRead)
{
    SOCKET s = (SOCKET)handle;
    int bytesRead;

    bytesRead = recv(s, (char*)pData, len, 0);
    lastError = WSAGetLastError();

    if (SOCKET_ERROR != bytesRead) {
        *pBytesRead = bytesRead;
        return PCSL_NET_SUCCESS;
    }

    if (WSAEWOULDBLOCK == lastError) {
        /*
         * Win32 only has one notifier per socket so always set both and close,
         * the MIDP event code can handle any extra notifications.
         * Do not cancel the notifications until the socket is closed.
         */
        WSAAsyncSelect(s, win32app_get_window_handle(), WM_NETWORK,
                       FD_READ | FD_WRITE | FD_CLOSE);
        return PCSL_NET_WOULDBLOCK;
    }

    if (WSAEINTR == lastError ||  WSAENOTSOCK == lastError) {
        return PCSL_NET_INTERRUPTED;
    }

    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_read_start(
	void *handle,
	unsigned char *pData,
	int len,  
	int *pBytesRead,
	void **pContext)
{
    *pContext = NULL;
    return winsock_read_common(handle, pData, len, pBytesRead);
}


/**
 * See pcsl_network.h for definition.
 */
extern int pcsl_socket_read_finish(
	void *handle,
	unsigned char *pData,
	int len,
	int *pBytesRead,
	void *context)
{
    (void)context;
    return winsock_read_common(handle, pData, len, pBytesRead);
}


/**
 * Common implementation between write_start() and write_finish().
 */
static int winsock_write_common(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten)
{
    SOCKET s = (SOCKET)handle;
    int bytesSent;

    bytesSent = send(s, pData, len, 0);
    lastError = WSAGetLastError();

    if (SOCKET_ERROR != bytesSent) {
        *pBytesWritten = bytesSent;
        return PCSL_NET_SUCCESS;
    }

    if (WSAEWOULDBLOCK == lastError) {
        /*
         * Win32 only has one notifier per socket so always set both and close,
         * the MIDP event code can handle any extra notifications.
         * Do not cancel the notifications until the socket is closed.
         */
        WSAAsyncSelect(s, win32app_get_window_handle(), WM_NETWORK,
                       FD_READ | FD_WRITE | FD_CLOSE);
        return PCSL_NET_WOULDBLOCK;
    }

    if (WSAEINTR == lastError ||  WSAENOTSOCK == lastError) {
        return PCSL_NET_INTERRUPTED;
    }

    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_write_start(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void **pContext)
{
    *pContext = NULL;
    return winsock_write_common(handle, pData, len, pBytesWritten);
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_write_finish(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void *context)
{
    (void)context;
    return winsock_write_common(handle, pData, len, pBytesWritten);
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_available(void *handle, int *pBytesAvailable)
{
    SOCKET s = (SOCKET)handle;
    unsigned long len = 0;
    int status;

    status = ioctlsocket(s, FIONREAD, &len);
    lastError = WSAGetLastError();
    if (SOCKET_ERROR != status) {
        *pBytesAvailable = (int)len;
        return PCSL_NET_SUCCESS;
    }

    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_shutdown_output(void *handle) {
    SOCKET s = (SOCKET)handle;
    int status;

    status = shutdown(s, SD_SEND);
    lastError = WSAGetLastError();
    if (SOCKET_ERROR != status) {
        return PCSL_NET_SUCCESS;
    }

    if (WSAENOTSOCK == lastError) {
        return PCSL_NET_INTERRUPTED;
    }

    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 *
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_socket_close_start(
    void *handle,
    void **pContext)
{
    SOCKET s = (SOCKET)handle;
    int status;

    (void)pContext;

    /*
     * Unblock any waiting threads, by send a close event with an interrupt
     * status. Closesocket cancels async notitifications on the socket and
     * does NOT send any messages.
     */
    PostMessage(win32app_get_window_handle(), WM_NETWORK, s,
                WSAMAKESELECTREPLY(FD_CLOSE, WSAEINTR));

    status = closesocket(s);

    lastError = WSAGetLastError();
    if (SOCKET_ERROR != status) {
        return PCSL_NET_SUCCESS;
    }        

    if (lastError == WSAEWOULDBLOCK) {
        /*
         * Call closesocket again, this will cause the socket to close
         * in the background in a system thread, see doc for closesocket.
         */
        closesocket(s);
        lastError = 0;
        return PCSL_NET_SUCCESS;
    }

    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_socket_close_finish(
	void *handle,
	void *context)
{
    (void)handle;
    (void)context;
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_network.h for definition.
 */
char * pcsl_inet_ntoa (void *ipBytes) {
    return inet_ntoa(*((struct in_addr*)ipBytes));
}
