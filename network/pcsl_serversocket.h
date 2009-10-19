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

#ifndef _PCSL_SERVERSOCKET_H
#define _PCSL_SERVERSOCKET_H

/**
 * @defgroup serversocket Server Socket Interface
 * @ingroup network_high
 */
   
/**
 * @file
 * @ingroup socket
 * @brief PCSL networking interfaces for server TCP sockets \n
 * ##include <pcsl_serversocket.h>
 * @{
 *
 */ 

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_SERVER_SOCKET
    
/**
  * The backlog parameter defines the maximum length the queue of pending
  * connections may grow to. If a connection request arrives with the queue
  * full the client may receive an error or, if the underlying protocol
  * supports retransmission, the request may be ignored so that retries succeed.
  */
#define SERVERSOCKET_BACKLOG 3

/**
 * Creates a server socket.
 *
 * @param port number of the port to open
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns PCSL_NET_SUCCESS.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_IOERROR if there was an IO error and IOException needs to be thrown.
 */
extern
int pcsl_serversocket_open(
    int port,
    void **pHandle);


/**
 * Creates a server socket.
 *
 * @param port number of the port to open
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns PCSL_NET_SUCCESS.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_IOERROR if there was an IO error and IOException needs to be thrown.
 */
extern
int pcsl_server_socket_open_start(
    int port,
    void **pHandle,
    void **pContext);


/**
 * Creates a server socket.
 *
 * @param port number of the port to open
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns PCSL_NET_SUCCESS.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_IOERROR if there was an IO error and IOException needs to be thrown.
 */
extern
int pcsl_server_socket_open_finish(
    void **pHandle,
    void **pContext);

/**
 * Initiates accept and open of connection associated with the
 * platform-specific handle. 
 *
 * @param handle platform-specific handle for the server connection
 * @param pConnectionHandle address of variable to receive the client socket
 *        handle; this is set only when this function returns PCSL_NET_SUCCESS.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_WOULDBLOCK if caller must call this function again to finish the
 * operation;\n PCSL_NET_IOERROR if there was an IO error and IOException needs to be thrown.
 */
extern int
pcsl_serversocket_accept_start(
    void *handle,
    void **pConnectionHandle,
    void **pContext);

/**
 * Finishes accept and open of connection associated with the
 * platform-specific handle. 
 *
 * @param handle platform-specific handle for the server connection
 * @param pConnectionHandle address of variable to receive the client socket
 *        handle; this is set only when this function returns PCSL_NET_SUCCESS.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_WOULDBLOCK if caller must call this function again to finish the
 * operation;\n PCSL_NET_IOERROR if there was an IO error and IOException needs to be thrown.
 */
extern int
pcsl_serversocket_accept_finish(
    void *handle,
    void **pConnectionHandle,
    void **pContext);

/**
 * Initiates the closing of a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns PCSL_NET_WOULDBLOCK
 *
 * @return PCSL_NET_SUCCESS upon success,\n 
 *         PCSL_NET_IOERROR for an error
 */
extern
int pcsl_serversocket_close_start(
	void *handle,
	void **pContext);


/**
 * Initiates the closing of a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param context the context returned by close_start
 *
 * @return PCSL_NET_SUCCESS upon success;\n
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 * PCSL_NET_IOERROR for an error
 */
extern
int pcsl_serversocket_close_finish(
	void *handle,
	void *context);

#endif /* ifdef ENABLE_SERVER_SOCKET */

   
/** @} */   //End of group High Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_SERVERSOCKET_H */
