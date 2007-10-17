/*
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

#ifndef __JAVACALL_DATAGRAM_H
#define __JAVACALL_DATAGRAM_H

#include "javacall_defs.h"

/**
 * @file
 * @ingroup Socket
 * @brief javacall interfaces for datagrams
 */ 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup OptionalDatagram Optional Datagram API
 * @ingroup Socket
 * @{
 */

/**
 * Opens a datagram socket
 *
 * @param port The local port to attach to. If the port is 0 then the platform
 *             will allocate an available port.
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns JAVACALL_OK.
 *
 * @return JAVACALL_OK if the function completes successfully
 *         JAVACALL_FAIL if there was an IO error and IOException needs to be thrown;
 */
javacall_result javacall_datagram_open(
	int port,
	javacall_handle *pHandle);



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
	  void *context); 


/**
 * Initiates the closing of a platform-specific datagram socket.
 *
 * @param handle handle of an open connection
 *
 * @return JAVACALL_OK upon success
 *         JAVACALL_FAIL for an error
 */
int javacall_datagram_close(
	javacall_handle handle);


/** @} */


/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup NotificationDatagram Notification API for Datagram 
 * @ingroup Socket
 * @{
 */
 
/**
 * @enum javacall_datagram_callback_type
 * @brief Notification callback type
 */
typedef enum {
    /** Datagram receive completed */ 
    JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED         =1000,
    /** Datagram send completed */
    JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED           =1001    
} javacall_datagram_callback_type;


/**
 * A callback function to be called for notification of non-blocking 
 * client/server socket related events, such as a socket completing opening or , 
 * closing socket remotely, disconnected by peer or data arrived on 
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *     - JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED
 *     - JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED
 * @param handle handle of datagram related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_datagram_event(
                             javacall_datagram_callback_type type, 
                             javacall_handle handle,
                             javacall_result operation_result);



/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __JAVACALL_DATAGRAM_H */



