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

#ifndef _PCSL_DATAGRAM_H
#define _PCSL_DATAGRAM_H

/**
 * @defgroup datagram Datagram Interface
 * @ingroup network_high
 */
   
/**
 * @file
 * @ingroup datagram
 * @brief PCSL networking interfaces for datagrams \n
 * ##include <pcsl_datagram.h>
 * @{
 *
 */ 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a datagram socket
 *
 * @param port The local port to attach to
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns PCSL_NET_SUCCESS.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns PCSL_NET_WOULDBLOCK.
 *
 * @return PCSL_NET_SUCCESS if the function completes successfully;\n
 * PCSL_NET_IOERROR if there was an IO error and IOException needs 
 * to be thrown;
 */
extern int pcsl_datagram_open_start(
	int port,
	void **pHandle,
	void **pContext);


/**
 * Finishes a pending open operation.
 *
 * @param handle the handle returned by the open_start function
 * @param context the context returned by the open_start function
 *
 * @return PCSL_NET_SUCCESS if the operation completed successfully;
 * PCSL_NET_IOERROR if an error occurred; PCSL_NET_INVALID if not 
 * applicable
 */
extern int pcsl_datagram_open_finish(
	void *handle,
	void *context);


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
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param pContext address of pointer variable to receive the context;
 *        it is set only when this function returns PCSL_NET_WOULDBLOCK
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_datagram_read_start(
    void *handle,
    unsigned char *pAddress,
    int *port, 
    char *buffer,
    int length,
    int *pBytesRead,
    void **pContext);

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
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param context the context returned by read_start
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_datagram_read_finish(
	void *handle,
    unsigned char *pAddress,
    int *port, 
    char *buffer,
    int length,
    int *pBytesRead,
	void *context);


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
 *        PCSL_NET_SUCCESS
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns PCSL_NET_WOULDBLOCK
 *
 * @return PCSL_NET_SUCCESS for successful write operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_datagram_write_start(
    void *handle,
    unsigned char *pAddress,
    int port,
    char *buffer,
    int length,
    int *pBytesWritten,
    void **pContext);

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
 *        PCSL_NET_SUCCESS
 * @param context the context returned by write_start
 *
 * @return PCSL_NET_SUCCESS for successful write operation;\n 
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
extern int pcsl_datagram_write_finish(
	void *handle,
    unsigned char *pAddress,
    int port,
    char *buffer,
    int length,
    int *pBytesWritten,
	void *context); 


/**
 * Initiates the closing of a platform-specific datagram socket.
 *
 * @param handle handle of an open connection
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns PCSL_NET_WOULDBLOCK
 *
 * @return PCSL_NET_SUCCESS upon success,\n 
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n PCSL_NET_IOERROR for an error
 */
extern int pcsl_datagram_close_start(
	void *handle,
	void **pContext);


/**
 * Initiates the closing of a platform-specific datagram socket.
 *
 * @param handle handle of an open connection
 * @param context the context returned by close_start
 *
 * @return PCSL_NET_SUCCESS upon success;\n
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 * PCSL_NET_IOERROR for an error
 */
extern int pcsl_datagram_close_finish(
	void *handle,
	void *context);


/** @} */   //End of group High Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_DATAGRAM_H */



