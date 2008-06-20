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

#ifndef _PCSL_SOCKET_H
#define _PCSL_SOCKET_H

/**
 * @defgroup socket Socket Interface
 * @ingroup network_high
 */
   
/**
 * @file
 * @ingroup socket
 * @brief PCSL networking interfaces for client TCP sockets \n
 * ##include <pcsl_socket.h>
 * @{
 *
 */ 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initiates the connection of a client socket.
 *
 * @param ipBytes IP address of the local device in the form of byte array
 * @param port number of the port to open
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns PCSL_NET_WOULDBLOCK or
 *        PCSL_NET_SUCCESS.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns PCSL_NET_WOULDBLOCK.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function to
 * complete the operation;\n PCSL_NET_IOERROR if there was an IO error and
 * IOException needs to be thrown;\n PCSL_NET_CONNECTION_NOTFOUND when 
 * there was some other error and ConnectionNotFound exception needs 
 * to be thrown 
 */
extern int pcsl_socket_open_start(
	unsigned char *ipBytes,
	int port,
	void **pHandle,
	void **pContext);


/**
 * Finishes a pending open operation.
 *
 * @param handle the handle returned by the open_start function
 * @param context the context returned by the open_start function
 *
 * @return PCSL_NET_SUCCESS if the operation completed successfully;\n
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 * PCSL_NET_IOERROR if an error occurred
 */
extern int pcsl_socket_open_finish(
	void *handle,
	void *context);


/**
 * Initiates a read from a platform-specific TCP socket.
 *  
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param pContext address of pointer variable to receive the context;
 *        it is set only when this function returns PCSL_NET_WOULDBLOCK
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_socket_read_start(
	void *handle,
	unsigned char *pData,
	int len,  
	int *pBytesRead,
	void **pContext); 

/**
 * Finishes a pending read operation.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param context the context returned by read_start
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_socket_read_finish(
	void *handle,
	unsigned char *pData,
	int len,
	int *pBytesRead,
	void *context);


/**
 * Initiates a write to a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        PCSL_NET_SUCCESS
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns PCSL_NET_WOULDBLOCK
 *
 * @return PCSL_NET_SUCCESS for successful write operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_socket_write_start(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void **pContext); 


/**
 * Finishes a pending write operation.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        PCSL_NET_SUCCESS
 * @param context the context returned by write_start
 *
 * @return PCSL_NET_SUCCESS for successful write operation;\n 
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_socket_write_finish(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void *context); 


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
extern int pcsl_socket_close_start(
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
extern int pcsl_socket_close_finish(
	void *handle,
	void *context);


/**
 * Gets the number of bytes available to be read from the platform-specific
 * socket without causing the system to block.
 *
 * @param handle handle of an open connection
 * @param pBytesAvailable returns the number of available bytes
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_socket_available(
	void *handle,
	int *pBytesAvailable);


/**
 * Shuts down the output side of a platform-specific TCP socket.
 * Further writes to this socket are disallowed.
 *
 * Note: a function to shut down the input side of a socket is
 * explicitly not provided.
 *
 * @param handle handle of an open connection
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_socket_shutdown_output(
	void *handle);


/**
 * Gets the IP address of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_socket_getlocaladdr(
	void *handle,
	char *pAddress);


/**
 * Gets the IP address of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_socket_getremoteaddr(
	void *handle,
	char *pAddress);

/** @} */   //End of group High Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_SOCKET_H */



