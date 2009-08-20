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

#ifndef __JAVACALL_SOCKET_H
#define __JAVACALL_SOCKET_H

/**
 * @file javacall_socket.h
 * @ingroup Socket
 * @brief Javacall interfaces for socket
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

/**
 * @defgroup Socket Socket API
 * @ingroup JTWI
 * 
 * Networking functionality is not mandatory for JTWI conformance. J2ME networking defines three,
 * support client TCP sockets, Datagram UDP sockets and Server TCP sockets. The functions needed to
 * support the client TCP sockets and UDP sockets are described below. \n
 * Networking requirements is similar to a very large extent to BSD sockets. Networking functions can
 * be broadly classified into the following groups,
 * - Client TCP socket functions:
 *    - socketopen
 *    - socketclose
 *    - socket read
 *    - socket write
 *    - get local address
 *    - get local port
 *    - get remote address
 *    - get remote port
 *    - getsockopt
 *    - setsockopt
 * - Datagram functions:
 *    - datagram open
 *    - datagram close
 *    - datagram read
 *    - datagram write
 * - Misc networking functions:
 *    - Network initialization
 *    - get Last error code
 *    - get Host by Name
 *    - get Local Host Name
 *    - get Local IP Address
 *    - get Host By Address
 * 
 * \par
 * 
 * The following functions support asynchronous operation:
 * - socket open
 * - socket close
 * - socket read
 * - socket write
 * - datagram open
 * - datagram close
 * - datagram read
 * - datagram write
 * - get Host By Address
 * - get Host by Name
 *
 * \par
 * 
 * In the async case, the functions returns JAVACALL_WOULD_BLOCK instead of blocking and an
 * event is required to be sent when the operation has completed. See section on event for related
 * information.
 * 
 * @{
 */

/**
 * @defgroup MandatoryTcpSocket Mandatory Client Socket API
 * @ingroup Socket
 * @{
 */

/**
 * Initiates the connection of a client socket.
 *
 * @param ipBytes The IP address of the remote device in the form of byte array.
 * @param port number of the port to open
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK or JAVACALL_OK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULDBLOCK.
 *
 * Socket operation:
 * 1.open a socket.
 * 2.set it to non-blocking mode.
 * 3.connect to remote ip address. 
 * 
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an IO error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function to complete the operation
 * @retval JAVACALL_CONNECTION_NOT_FOUND when there was some other error (Connection not found exception case)
 */
javacall_result javacall_socket_open_start(unsigned char *ipBytes,int port,
                                           javacall_handle* pHandle, void **pContext);
    
/**
 * Finishes a pending open operation.
 *
 * @param handle the handle returned by the open_start function
 * @param context the context returned by the open_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if an error occurred  
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result javacall_socket_open_finish(javacall_handle handle, void *context);
    
/**
 * Initiates a read from a platform-specific TCP socket.
 *  
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 * @param pContext address of pointer variable to receive the context;
 *        it is set only when this function returns JAVACALL_WOULDBLOCK
 * 
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_read_start(javacall_handle handle,unsigned char *pData,int len, 
                                           int *pBytesRead, void **pContext); 
    
/**
 * Finishes a pending read operation.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 * @param context the context returned by read_start
 * 
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_read_finish(javacall_handle handle,unsigned char *pData,int len,int *pBytesRead,void *context);
    
/**
 * Initiates a write to a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        JAVACALL_OK
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns JAVACALL_WOULDBLOCK
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_write_start(javacall_handle handle,char *pData,int len,int *pBytesWritten,void **pContext); 
    
/**
 * Finishes a pending write operation.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        JAVACALL_OK
 * @param context the context returned by write_start
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_write_finish(javacall_handle handle,char *pData,int len,int *pBytesWritten,void *context); 
    
/**
 * Initiates the closing of a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns JAVACALL_WOULDBLOCK
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block 
 */
javacall_result javacall_socket_close_start(javacall_handle handle,void **pContext);
    
/**
 * Initiates the closing of a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param context the context returned by close_start
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result javacall_socket_close_finish(javacall_handle handle,void *context);

/**
 * Gets the number of bytes available to be read from the platform-specific
 * socket without causing the system to block. If it is not possible to find
 * out the actual number of available bytes then the resulting number is 0.
 * <b>IMPORTANT</b>: Some features of the product (not convered by spec but still essential,
 * for example java debugging) might not work if this function does not return
 * actual value.
 *
 * @param handle handle of an open connection
 * @param pBytesAvailable returns the number of available bytes
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error 
 */
javacall_result javacall_socket_available(javacall_handle handle,
                                          int *pBytesAvailable);

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
 * @defgroup NotificationSocket Notification API for Socket 
 * @ingroup Socket
 * @{
 */
 
/**
 * @enum javacall_socket_callback_type
 * @brief Client socket notification event type
 */
typedef enum {
    /** Socket open completed */
    JAVACALL_EVENT_SOCKET_OPEN_COMPLETED            =1000,
    /** Socket close completed */
    JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED           =1001,
    /** Data receive completed */
    JAVACALL_EVENT_SOCKET_RECEIVE                   =1002,
    /** Data send completed */
    JAVACALL_EVENT_SOCKET_SEND                      =1003,
    /** Socket was disconnected from remote side */
    JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED       =1004,
    /** Get host by name completed */
    JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED  =1005
} javacall_socket_callback_type;


/**
 * A callback function to be called for notification of non-blocking 
 * client/server socket related events, such as a socket completing opening or , 
 * closing socket remotely, disconnected by peer or data arrived on 
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *          - JAVACALL_EVENT_SOCKET_OPEN_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_RECEIVE
 *          - JAVACALL_EVENT_SOCKET_SEND
 *          - JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED
 *          - JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED  
 * @param socket_handle handle of socket related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_socket_event(
                             javacall_socket_callback_type type, 
                             javacall_handle socket_handle,
                             javacall_result operation_result);

/** @} */
    


/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup OptionalTcpSocket Optional Server Socket API
 * @ingroup Socket
 * @{
 */

/**
 * Shuts down the output side of a platform-specific TCP socket.
 * Further writes to this socket are disallowed.
 *
 * Note: a function to shut down the input side of a socket is
 * explicitly not provided.
 *
 * @param handle handle of an open connection
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_shutdown_output(javacall_handle handle);
    
    
/**
 * Gets the IP address of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getlocaladdr(javacall_handle handle,char *pAddress);
    
    
/**
 * Gets the IP address of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getremoteaddr(javacall_handle handle,char *pAddress);
    
/**
 * Gets the port number of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the local port number
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getlocalport(javacall_handle handle,int *pPortNumber);
    
/** 
 * Gets the port number of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the remote port number
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getremoteport(javacall_handle handle,int *pPortNumber);


/**
 * Initiates the connection of a client/server socket.
 *
 * @param port number of the port to open (0 means that the platform will
 *             allocate an available port)
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK or JAVACALL_OK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULDBLOCK.
 *
 * Server Socket operation:
 * 1.open a socket.
 * 2.set it to non-blocking mode.
 * 3.configure the socket to reuse the port number.
 * 4.bind the socket to the given port.
 * 5.start listen on the socket and set the backlog to at least 3 concurrent connection.

 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an IO error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function to complete the operation
 * @retval JAVACALL_CONNECTION_NOT_FOUND when there was some other error (Connection not found exception case)
 */

javacall_result /*OPTIONAL*/ javacall_server_socket_open_start(int port, void **pHandle, void **pContext);

/**
 * Finishes a pending server socket open operation.
 *
 * @param handle the handle returned by the open_start function
 * @param context the context returned by the open_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if an error occurred  
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/javacall_server_socket_open_finish(void *handle,void *context);
    
/**
 * Initiate to accept handle to wait for incoming TCP connection for the given server socket.
 * 
 * @param handle handle of an open connection
 * @param newhandle a pointer to a handle which will receive the accpeted socket handle;
 *	                 it is set only when this function returns JAVACALL_OK
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block  
 */ 
javacall_result /*OPTIONAL*/ javacall_server_socket_accept_start(javacall_handle handle, javacall_handle *newhandle);

/**
 * Finish pending accept operation.
 * 
 * Note: This function will only be called if the javanotify_server_socket_event passes 0 as
 *       the new_socket_handle. Otherwise, the handle provided in the new_socket_handle will 
 *       be used.
 *
 * @param handle handle of an open connection
 * @param newhandle a pointer to a handle which will receive the accpeted socket handle;
 *	                 it is set only when this function returns JAVACALL_OK
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block  
 */ 
javacall_result /*OPTIONAL*/ javacall_server_socket_accept_finish(javacall_handle handle, javacall_handle *newhandle);
    
/** @} */

/**
 * @defgroup TcpServerSocketNotification Notification API for Server Socket
 * @ingroup Socket
 * @{
 */

/**
 * @enum javacall_server_socket_callback_type
 * @brief Server socket notification event type
 */
typedef enum {
    /** Accept completed */
    JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED   =2000
} javacall_server_socket_callback_type;

/**
 * A callback function to be called for notification of non-blocking 
 * server socket only related events, such as a accept completion.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *          JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED
 * @param socket_handle handle of socket related to the notification.
 *                          If the platform is not able to provide the socket 
 *                          handle in the callback, it should pass 0 as the new_socket_handle
 *                          and the implementation will call javacall_server_socket_accept_finish
 *                          to retrieve the handle for the accepted connection.
 * @param new_socket_handle in case of accept the socket handle for the 
 *                          newly created connection
 *               
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void /* OPTIONAL*/ javanotify_server_socket_event(
                             javacall_server_socket_callback_type type, 
                             javacall_handle socket_handle,
                             javacall_handle new_socket_handle,
                             javacall_result operation_result);
/** @} */

/** @} */
    
#ifdef __cplusplus
}
#endif

#endif 



