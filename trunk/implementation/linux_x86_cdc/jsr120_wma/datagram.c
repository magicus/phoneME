/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

//#include "javacall_datagram.h"
#include "javacall_defs.h"

/* DEBUG: for debugging purpose */
#include <stdio.h>
#define javacall_print(x)   do {\
    fprintf(stderr, "%s: %s\n", __FUNCTION__, (x)); \
    fflush(stderr); \
} while (0)

int tmp_main() {
    javacall_handle handle;
    javacall_result ok = javacall_datagram_open(11, &handle);


    struct hostent *pHost = gethostbyname("localhost");
    char *pAddress = pHost->h_addr_list[0];
    int port = 33300;
    char *buffer = "test message";
    int length = strlen(buffer);
    int pBytesWritten;
    void *pContext;

    if (JAVACALL_WOULD_BLOCK ==
        javacall_datagram_recvfrom_start(handle,
		                     pAddress, &port, buffer, length, &pBytesWritten, &pContext)) {
        while (JAVACALL_WOULD_BLOCK ==
	   javacall_datagram_recvfrom_finish(handle,
                 pAddress, &port, buffer, length, &pBytesWritten, &pContext));
    }

    javacall_datagram_close(handle);
    return 1;
}

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
javacall_result javacall_datagram_open(int port, javacall_handle *pHandle)
{
    int sockfd;
    int yes;
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
	return JAVACALL_FAIL;
    }

    *pHandle = (javacall_handle)sockfd;
		
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
        void **pContext)
{
    int sockfd = (int)handle;
    int flags = 0;
    int numbytes;
    int ok;
		
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(*port);      // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    ok = recv(sockfd, buffer, length, flags);

    if (errno == EWOULDBLOCK) {
        printf("recv : EWOULDBLOCK\n"); fflush(stdout);
        return JAVACALL_WOULD_BLOCK;
    }
    //printf("javacall_datagram_recvfrom_start: recv returned %i\n", ok);
    return JAVACALL_FAIL;
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
        void *context)
{
    int sockfd = (int)handle;
    int flags = 0;
	
    int ok = recv(sockfd, buffer, length, flags);
    if (errno == EWOULDBLOCK) {
        printf("recv : EWOULDBLOCK\n");
	return JAVACALL_WOULD_BLOCK;
    }
	
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
        void **pContext)
{
    int sockfd = (int)handle;    
    int flags = 0;
    int numbytes;
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    memcpy(&my_addr.sin_addr.s_addr, pAddress, sizeof(my_addr.sin_addr.s_addr));

    numbytes = sendto(sockfd, buffer, length, flags,
                (struct sockaddr *)&my_addr, sizeof(struct sockaddr));

    printf("javacall_datagram_sendto_start : bytes send = %i\n", numbytes); fflush(stdout);
    
    return JAVACALL_OK;
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
        void *context)
{
    return JAVACALL_FAIL;
}



/**
 * Initiates the closing of a platform-specific datagram socket.
 *
 * @param handle handle of an open connection
 *
 * @return JAVACALL_OK upon success
 *         JAVACALL_FAIL for an error
 */
int javacall_datagram_close(javacall_handle handle)
{
    int sockfd = (int)handle;
    close(sockfd);

    return JAVACALL_OK;
}


#include <sys/select.h>
#include <netinet/in.h>

int addUDPSocket(fd_set* fds, int* maxPort, int port) {
    
    int sock;
    struct sockaddr_in6 peer;
    int rc;

    bzero(&peer, sizeof(peer));
    peer.sin6_family = AF_INET6;
    peer.sin6_addr = in6addr_any;
    peer.sin6_port = htons(port);

    *maxPort = (*maxPort > peer.sin6_port) ? *maxPort : peer.sin6_port;

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
         perror("opening datagram socket");
         return -1;
    }

    rc = bind(sock, (struct sockaddr *)&peer, sizeof(peer));
    if (rc == -1) {
        perror("binding datagram socket");
	return -1;
    } else {
        printf("javacall: wma: binding datagram socket: %i, OK\n", port);
    }
    //for TCP only
    //rc = listen(sock, 3);
    //if (rc == -1) {
    //    perror("listening datagram socket");
    //    exit(1);
    //}

    return sock;
}

extern int smsDatagramSocketHandle, cbsDatagramSocketHandle;

#if ENABLE_JSR_205
extern int mmsDatagramSocketHandle;
#endif

void listen_sockets() {

    struct sockaddr_in6 peer;
    int rc;
    int fd = 0;
    int host_port1 = 11100;
    int host_port2 = 22200;
#if ENABLE_JSR_205
    int host_port3 = 33300;
#endif
    int max = 0;
    int buf[1024];

    fd_set fds;
    FD_ZERO(&fds); 

    smsDatagramSocketHandle = addUDPSocket(&fds, &max, host_port1);
    cbsDatagramSocketHandle = addUDPSocket(&fds, &max, host_port2);
#if ENABLE_JSR_205
    mmsDatagramSocketHandle = addUDPSocket(&fds, &max, host_port3);
#endif

    while (1) {
        FD_SET(smsDatagramSocketHandle, &fds); //need to reset each time
        FD_SET(cbsDatagramSocketHandle, &fds);
#if ENABLE_JSR_205
        FD_SET(mmsDatagramSocketHandle, &fds);
#endif

        printf("waiting...\n");

        select(max + 1, &fds, NULL, NULL, NULL);

        fd = (FD_ISSET(smsDatagramSocketHandle, &fds)) ? smsDatagramSocketHandle :
             (FD_ISSET(cbsDatagramSocketHandle, &fds)) ? cbsDatagramSocketHandle : -1;
#if ENABLE_JSR_205
        if (fd == -1) {
            fd = FD_ISSET(mmsDatagramSocketHandle, &fds) ? 
                                                  mmsDatagramSocketHandle : -1;
        }
#endif

        try_process_wma_emulator(fd);
    }

    //close(smsDatagramSocketHandle);
    //close(cbsDatagramSocketHandle);
#if ENABLE_JSR_205
    //close(mmsDatagramSocketHandle);
#endif
}

