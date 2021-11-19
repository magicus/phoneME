/*
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
#include "javacall_datagram.h"
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif



/* #include <midp_msgQueue_md.h>, will a pcsl_event header file later. */
extern HANDLE midpGetWindowHandle();
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

/* For use by pcsl_network_error. */
int lastError;


/**
 * Opens a datagram socket
 *
 * @param port The local port to attach to
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns JAVACALL_OK.
 *
 * @return JAVACALL_OK if the function completes successfully
 *         JAVACALL_FAIL if there was an IO error and IOException needs to be thrown;
 */
javacall_result javacall_datagram_open(
    int port,
  javacall_handle *pHandle) {

    SOCKET s;
    int truebuf = -1;
    int status;
    struct sockaddr_in addr;
    unsigned long nonblockingFlag = 1;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    lastError = WSAGetLastError();
    if (INVALID_SOCKET == s) {
        return JAVACALL_OK;
    }

    status = setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                        (char*)&truebuf, sizeof (truebuf));
    lastError = WSAGetLastError();
    if (SOCKET_ERROR == status) {
        (void)closesocket(s);
        return JAVACALL_FAIL;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    status = bind(s, (struct sockaddr*)&addr, sizeof (addr));
    lastError = WSAGetLastError();
    if (SOCKET_ERROR == status) {
        (void)closesocket(s);
        return JAVACALL_FAIL;
    }

    /* Set the socket to non-blocking. */
    ioctlsocket(s, FIONBIO, &nonblockingFlag);
     /*
       * Win32 only has one notifier per socket so always set both and
       * close, the MIDP event code can handle any extra notifications.
       * Do not cancel the notifications until the socket is closed.
       */
        WSAAsyncSelect(s, midpGetWindowHandle(), WM_NETWORK,
                           FD_READ | FD_WRITE | FD_CLOSE);

    *pHandle = (void*)s;

    return JAVACALL_OK;
  }

/**
 * Common implementation between pcsl_datagram_read_start()
 * and pcsl_datagram_read_finish().
 */
static int javacall_datagram_read_common(void *handle, unsigned char *pAddress,
        int *port, char *buffer, int length, int *pBytesRead)
{
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
            WSAAsyncSelect(s, midpGetWindowHandle(), WM_NETWORK,
                           FD_READ | FD_WRITE | FD_CLOSE);
            return JAVACALL_WOULD_BLOCK;
        }

        if (WSAECONNRESET == lastError) {
            /* The last call to sendto failed. Just return 0. */
            memset(pAddress, 0, sizeof(addr.sin_addr.s_addr));
            *port = 0;
            *pBytesRead = 0;

            return JAVACALL_OK;
        }

        if (WSAEINTR == lastError) {
            return JAVACALL_INTERRUPTED;
        }

        if (WSAEMSGSIZE == lastError) {
            /* The message was bigger than the buffer provided. */
            status = length;
        } else {
            return JAVACALL_FAIL;
        }
    }

    memcpy(pAddress, &addr.sin_addr.s_addr, sizeof(addr.sin_addr.s_addr));
    *port = ntohs(addr.sin_port);
    *pBytesRead = status;

    return JAVACALL_OK;
}

/**
 * Initiates a read from a platform-specific datagram.
 *
 * <p>
 * <b>NOTE:</b> The parameter <tt>buffer</tt> must be pre-allocated
 * prior to calling this function.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 * @param port pointer to the port number of the remote location
 *             that sent the datagram. <tt>port</tt> is set by
 *             this function.
 * @param buffer data received from the remote location. The contents
 *               of <tt>buffer</tt> are set by this function.
 * @param length the length of the buffer
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 * @param pContext address of pointer variable to receive the context;
 *        it is set only when this function returns JAVACALL_WOULD_BLOCK
 *
 * @return JAVACALL_OK for successful read operation
 *         JAVACALL_WOULD_BLOCK if the operation would block
 *         JAVACALL_INTERRUPTED for an Interrupted IO Exception
 *         JAVACALL_FAIL for all other errors
 */
int javacall_datagram_recvfrom_start(
    javacall_handle handle,
    unsigned char *pAddress,
    int *port,
    char *buffer,
    int length,
    int *pBytesRead,
    void **pContext) {

    *pContext = NULL;
       return javacall_datagram_read_common(handle, pAddress, port,
                                     buffer, length, pBytesRead);
    }

/**
 * Finishes a pending read operation.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 * @param port pointer to the port number of the remote location
 *             that sent the datagram. <tt>port</tt> is set by
 *             this function.
 * @param buffer data received from the remote location. The contents
 *               of <tt>buffer</tt> are set by this function.
 * @param length the length of the buffer
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 * @param context the context returned by javacall_datagram_recvfrom_start
 *
 * @return JAVACALL_OK for successful read operation;
 *         JAVACALL_WOULD_BLOCK if the caller must call the finish function again to complete the operation;
 *         JAVACALL_INTERRUPTED for an Interrupted IO Exception
 *         JAVACALL_FAIL for all other errors
 */
int javacall_datagram_recvfrom_finish(
      javacall_handle handle,
    unsigned char *pAddress,
    int *port,
    char *buffer,
    int length,
    int *pBytesRead,
    void *context) {

    (void)context;
      return javacall_datagram_read_common(handle, pAddress, port,
                                     buffer, length, pBytesRead);
    }


/**
 * Common implementation between pcsl_datagram_write_start()
 * and pcsl_datagram_write_finish().
 */
static int javacall_datagram_write_common(void *handle, unsigned char *ipBytes,
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
            WSAAsyncSelect(s, midpGetWindowHandle(), WM_NETWORK,
                           FD_READ | FD_WRITE | FD_CLOSE);
            return  JAVACALL_WOULD_BLOCK ;
        }

        if (WSAEINTR == lastError) {
            return JAVACALL_INTERRUPTED;
        }

        return JAVACALL_FAIL;
    }

    *pBytesWritten = status;
    return JAVACALL_OK;
}


/**
 * Initiates a write to a platform-specific datagram
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 * @param port port number of the remote location to send the datagram
 * @param buffer data to send to the remote location
 * @param length amount of data, in bytes, to send to the remote
 *               location
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        JAVACALL_OK
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns JAVACALL_WOULD_BLOCK
 *
 * @return JAVACALL_OK for successful write operation;
 *         JAVACALL_WOULD_BLOCK if the operation would block,
 *         JAVACALL_INTERRUPTED for an Interrupted IO Exception
 *         JAVACALL_FAIL for all other errors
 */
int javacall_datagram_sendto_start(
    javacall_handle handle,
    unsigned char *pAddress,
    int port,
    char *buffer,
    int length,
    int *pBytesWritten,
    void **pContext) {

    *pContext = NULL;
       return javacall_datagram_write_common(handle, pAddress, port, buffer,
                                       length, pBytesWritten);
    }

/**
 * Finishes a pending write operation.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 * @param port port number of the remote location to send the datagram
 * @param buffer data to send to the remote location
 * @param length amount of data, in bytes, to send to the remote
 *               location
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        JAVACALL_OK
 * @param context the context returned by javacall_datagram_sendto_start
 *
 * @return JAVACALL_OK for successful write operation;
 *         JAVACALL_WOULD_BLOCK if the caller must call the finish function again to complete the operation;
 *         JAVACALL_INTERRUPTED for an Interrupted IO Exception
 *         JAVACALL_FAIL for all other errors
 */
int javacall_datagram_sendto_finish(
      javacall_handle handle,
    unsigned char *pAddress,
    int port,
    char *buffer,
    int length,
    int *pBytesWritten,
    void *context) {

    (void)context;
       return javacall_datagram_write_common(handle, pAddress, port, buffer,
                                      length, pBytesWritten);
    }


/**
 * Initiates the closing of a platform-specific datagram socket.
 *
 * @param handle handle of an open connection
 *
 * @return JAVACALL_OK upon success
 *         JAVACALL_FAIL for an error
 */
int javacall_datagram_close(
  javacall_handle handle) {

    SOCKET s = (SOCKET)handle;

    WSAAsyncSelect(s, midpGetWindowHandle(), 0, 0);
    /*
     * Unblock any waiting threads, by send a close event with an interrupt
     * status. Closesocket cancels async notitifications on the socket and
     * does NOT send any messages.
     */
    PostMessage(midpGetWindowHandle(), WM_NETWORK, s,
                WSAMAKESELECTREPLY(FD_CLOSE, WSAEINTR));

    /*
     * There are no documented errors when closing a UDP port,
     * also nothing can be done if a UDP port can't be closed.
     */
    closesocket(s);
    lastError = 0;
    return JAVACALL_OK;
}

