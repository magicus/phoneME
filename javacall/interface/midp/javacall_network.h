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

#ifndef __JAVACALL_NETWORK_H
#define __JAVACALL_NETWORK_H

/**
 * @file javacall_network.h
 * @ingroup Network
 * @brief Javacall interfaces for networking
 */

/**
 * @defgroup Network Network API
 * @ingroup JTWI
 * 
 * This is the API for networking.  Currently, the APIs support only
 * client-side TCP sockets.
 *
 * The API is structured to facilitate implementation on a platform that uses
 * asynchronous I/O. Certain I/O operations have two phases: the first to
 * initiate the operation and the second to retrieve the results. To this
 * end, these operations are represented by two functions: the first suffixed
 * with "start" and the second suffixed with "finish".  The operations for
 * which an asynchronous implementation is supported are open, read, write,
 * close, and gethostbyname.
 *
 * Each such I/O operation starts when the start-function is called.  If the
 * operation can complete immediately (e.g., for a read operation, there is
 * sufficient data from the network already available) the start-function can
 * return its results directly. If this occurs, the operation is complete, and
 * the finish-function will not be called.
 *
 * If the operation cannot complete immediately, and assuming there is no
 * error, the start-function should return an indication of
 * JAVACALL__NET_WOULDBLOCK. This indicates to the caller that it must call the
 * finish-function sometime later in order to retrieve the results. Instead of
 * returning data, in this case the start-function returns a handle (if
 * necessary) and a context. The handle and the pointer will be stored
 * by the caller and will be passed to the finish-function when it's called.
 *
 * If for some reason the operation still is incomplete at the time the finish
 * function is called (for example, it is a multi-step operation) the finish
 * function can also return JAVACALL__NET_WOULDBLOCK. As before, this indicates to
 * the caller that it must call the finish-function again later in order to
 * retrieve the results. The implementation can keep track of intermediate
 * state between finish calls by updating information in its context.
 *
 * The handle is an opaque datum (a void*) that serves both to represent an
 * open connection and also as the name for a pending I/O operation.  The
 * handle is used by the calling system to keep track of threads blocked
 * waiting for this I/O operation to complete.  The handle must be unique;
 * that is, it must not be possible for any two currently pending I/O
 * operations to return the same handle.
 *
 * It is recommended that implementations allocate a context structure on the
 * heap in the start-function and return a pointer to the caller. This context
 * structure can contain any platform-specific information necessary to keep
 * track of the pending I/O operation, such as a return data buffer and status
 * codes. The context structure should be freed when the finish-function
 * completes.  If the implementation of a particular I/O operation has no need
 * for a context, it must set the context pointer to NULL in the
 * start-function.
 *
 * This structure in support of asynchronous I/O makes it somewhat
 * inconvenient for platforms that primarily support synchronous I/O. For
 * example, on Unix, the read() call is synchronous in that it either
 * completes successfully or returns an indication (EWOULDBLOCK) that it
 * cannot perform any I/O; in either case, no pending I/O operation results.
 * Nonetheless, if the start-function for reading issues a read() call that
 * returns EWOULDBLOCK, it must return JAVACALL__NET_WOULDBLOCK and wait for a
 * subsequent call to the finish-function.  When the finish-function is
 * called, a read() call must be issued that is similar -- if not identical --
 * to the read() call issued by the start-function. In this case having
 * separate start-function and finish-functions isn't useful. However, they
 * must be implemented on all platforms, even those that use synchronous I/O,
 * to preserve the uniformity of appearance of JAVACALL_ on all platforms.
 *
 * The only case where a finish-function need not be implemented is if the
 * platform can guarantee that its corresponding start-function never returns
 * PSCL_IO_WOULDBLOCK.  In this case a finish-function must be supplied in
 * order to avoid linkage errors, but it should be empty.  (Ideally, in debug
 * mode, it should trigger an assertion failure.)
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "javacall_defs.h"

/**
 * @defgroup MandatoryNetwork Mandatory Networking API
 * @ingroup Network
 * @{
 */

/**
 * Performs platform-specific initialization of the networking system.
 * Will be called ONCE during VM startup before opening a network connection.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_WOULD_BLOCK caller must call the xxx_init_finish function to
 * 			complete the operation
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_init_start(void);

/**
 * Finishes platform-specific initialization of the networking system.
 * The function is invoked be the JVM after receiving JAVACALL_NETWORK_UP
 * notification from the platform.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_init_finish(void);

/**
 * Performs platform-specific finalization of the networking system.
 * Will be called ONCE during VM shutdown.
 *
 * @retval JAVACALL_WOULD_BLOCK caller must call xxx_finalize_finish
 *         function to complete the operation
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize_start(void);

/**
 * Finishes platform-specific finalize of the networking system.
 * The function is invoked be the JVM after receiving JAVACALL_NETWORK_DOWN
 * notification from the platform.
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize_finish(void);

/**
 * Initiates lookup of the given host name to find its IP address.
 *
 * @param hostname the host name for which an ip address is needed,
 *        a null-terminated string
 * @param pAddress base of byte array to receive the address (the IP
 *        address should be stored in network byte order).
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an
 *        IPv4 address, 16 if it is an IPv6 address
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULD_BLOCK.
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if there is a network error
 * @retval JAVACALL_INVALID_ARGUMENT    if maxLen is too small to receive the address
 * @retval JAVACALL_WOULD_BLOCK         if the caller must call the finish function
 *                                      again to complete the operation
 */
javacall_result javacall_network_gethostbyname_start(char *hostname,
                                        unsigned char *pAddress,
                                        int maxLen,
                                        /*OUT*/ int *pLen,
                                        /*OUT*/ javacall_handle* pHandle,
                                        /*OUT*/ void **pContext);

/**
 * Finishes a pending host name lookup operation.
 *
 * @param pAddress base of byte array to receive the address (the IP
 *        address should be stored in network byte order).
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an
 *        IPv4 address, 16 if it is an IPv6 address
 * @param handle the handle returned by the gethostbyname_start function
 * @param context the context returned by the gethostbyname_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there is a network error
 */
javacall_result javacall_network_gethostbyname_finish(
                                        unsigned char *pAddress,
                                        int maxLen,
                                        int *pLen,
                                        javacall_handle handle,
                                        void *context);
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
 * @defgroup NotificationNetwork Notification API for Networking
 * @ingroup Network
 * @{
 */

/**
 * @enum javacall_network_event
 * @brief Network notification event type
 */
typedef enum {
    /** Network is down */
    JAVACALL_NETWORK_DOWN             =50,
    /** Network is up */
    JAVACALL_NETWORK_UP               =51,
    /** Network down request */
    JAVACALL_NETWORK_DOWN_REQUEST     =52
} javacall_network_event;

/**
 * @enum javacall_socket_option
 * @brief socket option of the values defined in javax.microedition.io.SocketConnection
 */ 
typedef enum {
    /** The small buffer writing delay. Set to zero to disable 
     * Nagle algorithm for small buffer operations. */
    JAVACALL_SOCK_DELAY         = 0,
    /** The linger time to wait in seconds before closing a connection 
     * with pending data output. Set to zero to disable the feature. */
    JAVACALL_SOCK_LINGER        = 1,
    /** The keep alive feature. Set to zero to disalbe the feature. */
    JAVACALL_SOCK_KEEPALIVE     = 2,
    /** The size of the receiving buffer. */
    JAVACALL_SOCK_RCVBUF        = 3,
    /** The size of the sending buffer. */
    JAVACALL_SOCK_SNDBUF        = 4,
    /** The abort(RST) to be sent when the connection is closed, 
     * instead of normal FIN. Set zero value to diable and positive value 
     * to enable */
    JAVACALL_SOCK_ABORT         = 5
} javacall_socket_option;

/**
 * A callback function to be called for notification of network
 * conenction related events, such as network going down or up.
 * The platform will invoke the call back in platform context.
 * @param event the type of network-related event that occured
 *              JAVACALL_NETWORK_DOWN if the network became unavailable
 *              JAVACALL_NETWORK_UP if the network is now available
 *
 */
void javanotify_network_event(javacall_network_event event);


/** @} */

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/

/**
 * @defgroup OptionalNetwork Optional Networking API
 * @ingroup Network
 *
 * @{
 */

/**
 * Gets a platform-specific error code for the previous operation on an open
 * connection.  This is mainly useful for adding detail information to
 * debugging and diagnostic messages.
 *
 * @param handle handle of an open connection
 *
 * @return 0 if there is no error;\n
 * a non-zero, platform-specific value if there was an error
 */
int /*OPTIONAL*/ javacall_network_error(javacall_handle handle);

/**
 * Gets the name of the local device from the system. This method is
 * called when the <tt>microedition.hostname</tt> system property
 * is retrieved.
 *
 * @param pLocalHost base of char array to receive the host name, Size
 *        of the pLocalHost should be MAX_HOST_LENGTH
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_get_local_host_name(/*OUT*/ char *pLocalHost);

/**
 * Gets the string representation of the local device's IP address.
 * This function returns dotted quad IP address as a string in the
 * output parameter and not the host name.
 *
 * @param pLocalIPAddress base of char array to receive the local
 *        device's IP address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_get_local_ip_address_as_string(/*OUT*/ char *pLocalIPAddress);

/**
 * Gets the http / https proxy address. This method is
 * called when the <tt>com.sun.midp.io.http.proxy</tt> or
 <tt>com.sun.midp.io.https.proxy</tt> internal property
 * is retrieved.
 *
 * @param pHttpProxy base of char array to receive the hostname followed
 *          by ':' and port. - ex) webcache.thecompany.com:8080.
 *          Size of the pHttpProxy should be (MAX_HOST_LENGTH + 6).
 * @param pHttpsProxy base of char array to receive the hostname followed
 *          by ':' and port. - ex) webcache.thecompany.com:8080.
 *          Size of the pHttpsProxy should be (MAX_HOST_LENGTH + 6).
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_get_http_proxy(/*OUT*/ char *pHttpProxy, /*OUT*/ char *pHttpsProxy);

/**
 * Translates the given IP address into a host name.
 *
 * @param ipn Raw IP address to translate
 * @param hostname the host name. The value of <tt>host</tt> is set by
 *             this function.
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULD_BLOCK.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there is a network error
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_network_gethostbyaddr_start(int ipn,
    char *hostname, /*OUT*/ javacall_handle* pHandle, void **pContext);

/**
 * Finishes a pending host name lookup operation.
 *
 * @param ipn Raw IP address to translate
 * @param hostname the host name. The value of <tt>host</tt> is set by
 *             this function.
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param context the context returned by the getHostByAddr_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there is a network error
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_network_gethostbyaddr_finish(int ipn,
    char *hostname, /*OUT*/ javacall_handle* pHandle, void *context);

/**
 * Gets an option's value for a platform-specific TCP socket or datagram
 *
 * @param handle handle of an open connection
 * @param flag socket option to get. Must be one of the values defined in
 *             <tt>javax.microedition.io.SocketConnection</tt>
 * @param pOptval returns the option's value
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_getsockopt(javacall_handle handle,javacall_socket_option flag, /*OUT*/ int *pOptval);

/**
 * Sets an option's value for a platform-specific TCP socket or datagram
 *
 * @param handle handle of an open connection
 * @param flag socket option to set. Must be one of the values defined in
 *             <tt>javax.microedition.io.SocketConnection</tt>
 * @param optval the value to be set for this option
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if there is a network error
 * @retval JAVACALL_INVALID_ARGUMENT   if the platform did not accept the value for the option changed
 */
javacall_result /*OPTIONAL*/ javacall_network_setsockopt(javacall_handle handle, javacall_socket_option flag, int optval);

/**
 * A javacall equivelent of BSD inet_ntoa () function.
 * The inet_ntoa() function converts the Internet host address to a string 
 * in standard numbers-and-dots notation. The string is returned in
 * a statically allocated buffer, which subsequent calls will overwrite.
 * 
 * @param address the IP address of the remote device in the form of byte array
 *
 * @return converted address
 */
char * javacall_inet_ntoa(void *address);

/**
 * Enables or disables ACCEPT notifications for the given server socket
 *
 * @param handle handle of an opened server socket
 * @param set the flag which sais whether to set or remove notification
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if there is a network error
 */
javacall_result javacall_server_socket_set_notifier(javacall_handle handle, javacall_bool set);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _JAVACALL__NETWORK_H */


