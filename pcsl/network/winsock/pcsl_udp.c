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
 * Implementation of pcsl_datagram.h for platforms that support the
 * Winsock API.
 *
 * For all functions, the "handle" is the Winsock HANDLE
 * cast to void *.  The read and writes are synchronous,
 * so the context is always set to NULL.
 */

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif

#include <pcsl_network.h>

/* #include <win32app_export.h>, will a pcsl_event header file later. */
extern HANDLE win32app_get_window_handle();
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

/* For use by pcsl_network_error. */
int lastError;

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_open_start(int port, void **pHandle, void **pContext) {
    SOCKET s;
    int truebuf = -1;
    int status;
    struct sockaddr_in addr;
    unsigned long nonblockingFlag = 1;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    lastError = WSAGetLastError();
    if (INVALID_SOCKET == s) {
        return PCSL_NET_IOERROR;
    }

    status = setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                        (char*)&truebuf, sizeof (truebuf));
    lastError = WSAGetLastError();
    if (SOCKET_ERROR == status) {
        (void)closesocket(s);
        return PCSL_NET_IOERROR;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    status = bind(s, (struct sockaddr*)&addr, sizeof (addr));
    lastError = WSAGetLastError();

    if (SOCKET_ERROR == status) {
        (void)closesocket(s);
        return PCSL_NET_IOERROR;
    }

    /* Set the socket to non-blocking. */
    ioctlsocket(s, FIONBIO, &nonblockingFlag);

    *pHandle = (void*)s;
    *pContext = NULL;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_datagram.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_datagram_open_finish(void *handle, void *context) {
    return PCSL_NET_INVALID;
}

/**
 * Common implementation between pcsl_datagram_read_start() 
 * and pcsl_datagram_read_finish().
 */
static int pcsl_datagram_read_common(void *handle, unsigned char *pAddress,
        int *port, char *buffer, int length, int *pBytesRead) {
    SOCKET s = (SOCKET)handle;
    struct sockaddr_in addr;
    int len = sizeof(struct sockaddr_in);
    int status;

    status = recvfrom(s, buffer, length, 0, (struct sockaddr*)&addr,  &len);
    lastError = WSAGetLastError();

    if (SOCKET_ERROR == status) {
        if (WSAEWOULDBLOCK == lastError) {
            /*
             * Win32 only has one notifier per socket so always set both and
             * close, the MIDP event code can handle any extra notifications.
             * Do not cancel the notifications until the socket is closed.
             */
            WSAAsyncSelect(s, win32app_get_window_handle(), WM_NETWORK,
                           FD_READ | FD_WRITE | FD_CLOSE);
            return PCSL_NET_WOULDBLOCK;
        }

        if (WSAECONNRESET == lastError) {
            /* The last call to sendto failed. Just return 0. */
            memset(pAddress, 0, sizeof(addr.sin_addr.s_addr)); 
            *port = 0;
            *pBytesRead = 0;

            return PCSL_NET_SUCCESS;
        }

        if (WSAEINTR == lastError) {
            return PCSL_NET_INTERRUPTED;
        }

        if (WSAEMSGSIZE == lastError) {
            /* The message was bigger than the buffer provided. */
            status = length;
        } else {
            return PCSL_NET_IOERROR;
        }
    }

    memcpy(pAddress, &addr.sin_addr.s_addr, sizeof(addr.sin_addr.s_addr)); 
    *port = ntohs(addr.sin_port);
    *pBytesRead = status;

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_read_start(void *handle, unsigned char *pAddress,
        int *port, char *buffer, int length, int *pBytesRead,
        void **pContext) {
    *pContext = NULL;
    return pcsl_datagram_read_common(handle, pAddress, port,
                                     buffer, length, pBytesRead);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_read_finish(void *handle, unsigned char *pAddress,
        int *port, char *buffer, int length, int *pBytesRead, void *context) {
    (void)context;
    return pcsl_datagram_read_common(handle, pAddress, port,
                                     buffer, length, pBytesRead);
}

/**
 * Common implementation between pcsl_datagram_write_start() 
 * and pcsl_datagram_write_finish().
 */
static int pcsl_datagram_write_common(void *handle, unsigned char *ipBytes,
        int port, char *buffer, int length, int *pBytesWritten) {
    SOCKET s = (SOCKET)handle;
    struct sockaddr_in addr;
    int status;

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((short)port);
    memcpy(&addr.sin_addr.s_addr, ipBytes, sizeof(addr.sin_addr.s_addr));

    status = sendto(s, buffer, length, 0, (struct sockaddr*)&addr,
                    sizeof(addr));
    lastError = WSAGetLastError();

    if (SOCKET_ERROR == status) {
        if (WSAEWOULDBLOCK == lastError) {
            /*
             * Win32 only has one notifier per socket so always set both and
             * close, the MIDP event code can handle any extra notifications.
             * Do not cancel the notifications until the socket is closed.
             */
            WSAAsyncSelect(s, win32app_get_window_handle(), WM_NETWORK,
                           FD_READ | FD_WRITE | FD_CLOSE);
            return PCSL_NET_WOULDBLOCK;
        }

        if (WSAEINTR == lastError) {
            return PCSL_NET_INTERRUPTED;
        }

        return PCSL_NET_IOERROR;
    }

    *pBytesWritten = status;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_write_start(void *handle, unsigned char *ipBytes,
        int port, char *buffer, int length, int *pBytesWritten,
        void **pContext) {
    *pContext = NULL;
    return pcsl_datagram_write_common(handle, ipBytes, port, buffer, 
                                      length, pBytesWritten);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_write_finish(void *handle, unsigned char *ipBytes,
        int port, char *buffer, int length, int *pBytesWritten,
        void *context) {
    (void)context;
    return pcsl_datagram_write_common(handle, ipBytes, port, buffer, 
                                      length, pBytesWritten);
}

/**
 * See pcsl_datagram.h for definition.
 *
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_datagram_close_start(void *handle, void **pContext) {
    SOCKET s = (SOCKET)handle;

    (void)pContext;

    /*
     * Unblock any waiting threads, by send a close event with an interrupt
     * status. Closesocket cancels async notitifications on the socket and
     * does NOT send any messages.
     */
    PostMessage(win32app_get_window_handle(), WM_NETWORK, s,
                WSAMAKESELECTREPLY(FD_CLOSE, WSAEINTR));

    /*
     * There are no documented errors when closing a UDP port,
     * also nothing can be done if a UDP port can't be closed.
     */
    closesocket(s);
    lastError = 0;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_datagram.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_datagram_close_finish(void *handle, void *context) {
    (void)handle;
    (void)context;
    return PCSL_NET_INVALID;
}
