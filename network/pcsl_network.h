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
#ifndef _PCSL_NETWORK_H
#define _PCSL_NETWORK_H

#include <pcsl_socket.h>
#include <pcsl_datagram.h>
#include <pcsl_network_md.h>

/**
 * @defgroup network Network Interfaces
 */

/**
 * @defgroup network_high  High Level Interface
 * @ingroup network
 */

/**
 * @defgroup network_low  Low Level Interface
 * @ingroup network
 */

/**
 * @file
 * @ingroup network
 * @brief PCSL networking interfaces for client TCP sockets \n
 * ##include <>
 * @{
 *
 * This is the PCSL API for networking.  Currently, the APIs support only 
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
 * PCSL_NET_WOULDBLOCK. This indicates to the caller that it must call the
 * finish-function sometime later in order to retrieve the results. Instead of
 * returning data, in this case the start-function returns a handle (if
 * necessary) and a context. The handle and the pointer will be stored
 * by the caller and will be passed to the finish-function when it's called.
 *
 * If for some reason the operation still is incomplete at the time the finish 
 * function is called (for example, it is a multi-step operation) the finish 
 * function can also return PCSL_NET_WOULDBLOCK. As before, this indicates to 
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
 * returns EWOULDBLOCK, it must return PCSL_NET_WOULDBLOCK and wait for a
 * subsequent call to the finish-function.  When the finish-function is
 * called, a read() call must be issued that is similar -- if not identical --
 * to the read() call issued by the start-function. In this case having
 * separate start-function and finish-functions isn't useful. However, they
 * must be implemented on all platforms, even those that use synchronous I/O,
 * to preserve the uniformity of appearance of PCSL on all platforms.
 *
 * The only case where a finish-function need not be implemented is if the
 * platform can guarantee that its corresponding start-function never returns
 * PSCL_IO_WOULDBLOCK.  In this case a finish-function must be supplied in 
 * order to avoid linkage errors, but it should be empty.  (Ideally, in debug 
 * mode, it should trigger an assertion failure.)
 *
 * Following are some of the known issues in PCSL networking implementations:
 *
 * -# support for IPv6
 * -# get{local,remote}addr should return addr in binary, not as a \n
 *        character string; possibly have separate function like inet_ntop
 * -# better name for pcsl_socket_getLocalIPAddressAsString
 * -# better name for pcsl_socket_getLocalHostName
 * -# need to refine the uniqueness requirements for the handle
 */ 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return value indicating that the operation was completed successfully.
 */
#define PCSL_NET_SUCCESS 0

/**
 * Return value indicating that the finish-function will need to
 * be called later in order to complete the operation.
 */
#define PCSL_NET_WOULDBLOCK -1

/**
 * Return value indicating that an I/O error of some sort has occurred.
 */
#define PCSL_NET_IOERROR -2

/** 
 * Return value indicating that the operation was terminated by an interrupt.
 * This typically causes InterruptedIOException to be thrown in calling Java
 * thread.
 */
#define PCSL_NET_INTERRUPTED -3

/** 
 * Return value indicating that there was an error and
 * ConnectionNotFoundException needs to be thrwon to the calling Java thread
 */
#define PCSL_NET_CONNECTION_NOTFOUND -4

/**
 * Return value indicating that a function parameter had an invalid value.
 */
#define PCSL_NET_INVALID -5

/**
 * A value that is guaranteed to be different from any
 * possible PCSL_NET_* return value.
 * No function will ever return it as a status code.
 */
#define PCSL_NET_NOSTATUS -6

/**
 * The value which valid handle returned by pcsl network functions cannot have
 */
#define INVALID_HANDLE INVALID_HANDLE_MD

/**
 * Maximum length of array of bytes sufficient to hold IP address
 */
#define MAX_ADDR_LENGTH MAX_ADDR_LENGTH_MD

/**
 * Maximum host name length
 */
#define MAX_HOST_LENGTH MAX_HOST_LENGTH_MD

/**
 * Type of a function that will be called when the network initialization
 * or finalization is completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
typedef void (*PCSL_NET_CALLBACK)(int isInit, int status);

/**
 * Performs platform-specific initialization of the networking system.
 * 
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_init(void);

/**
 * Starts platform-specific initialization of the networking system.
 *
 * @param pcsl_network_callback pointer to a function that must be called after
 *                              completion of the network initialization;
 *                              can be NULL.
 *
 * @return PCSL_NET_SUCCESS upon success;
 *         PCSL_NET_IOERROR if there is a network error;
 *         PCSL_NET_WOULDBLOCK if the initialization is started, but if can't be
 *                             completed right now; if pcsl_network_callback
 *                             is not null, it will be called when the
 *                             initialization is finished.
 * Note that Winsock and BSD sockets implementations
 * never return PCSL_NET_WOULDBLOCK. 
 */
extern int pcsl_network_init_start(PCSL_NET_CALLBACK pcsl_network_callback);

/**
 * Finalize platform-specific initialization of the networking system.
 *
 *
 * @return PCSL_NET_SUCCESS upon success; 
 *         PCSL_NET_IOERROR if there is a network error;
 *         PCSL_NET_WOULDBLOCK if the initialization is not finished yet
 * Note that Winsock and BSD sockets implementations
 * never return PCSL_NET_WOULDBLOCK.
 */
extern int pcsl_network_init_finish(void);


/**
 * Starts platform-specific finalization of the networking system.
 *
 * @param pcsl_network_callback pointer to a function that must be called after
 *                              completion of the network finalization;
 *                              can be NULL.
 *
 * @return PCSL_NET_SUCCESS upon success;
 *         PCSL_NET_IOERROR if there is a network error;
 *         PCSL_NET_WOULDBLOCK if the finalization is started, but if can't be
 *                             completed right now; if pcsl_network_callback
 *                             is not null, it will be called when the
 *                             initialization is finished.
 * Note that Winsock and BSD sockets implementations
 * never return PCSL_NET_WOULDBLOCK.
 */
extern int pcsl_network_finalize_start(PCSL_NET_CALLBACK pcsl_network_callback);

/**
 * Finalize platform-specific finalization of the networking system.
 *
 *
 * @return PCSL_NET_SUCCESS upon success; 
 *         PCSL_NET_IOERROR if there is a network error;
 *         PCSL_NET_WOULDBLOCK if the finalization is not finished yet
 * Note that Winsock and BSD sockets implementations
 * never return PCSL_NET_WOULDBLOCK.
 */
extern int pcsl_network_finalize_finish(void);

/**
 * Initiates lookup of the given host name to find its IP address.
 *
 * @param hostname the host name for which an ip address is needed,
 *        a null-terminated string
 * @param pAddress base of byte array to receive the address
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an 
 *        IPv4 address, 16 if it is an IPv6 address
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns PCSL_NET_WOULDBLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns PCSL_NET_WOULDBLOCK.
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 * PCSL_NET_IOERROR if there is a network error;\n 
 * PCSL_NET_INVALID if maxLen is too small to receive the address
 */
extern int pcsl_network_gethostbyname_start(
	char *hostname,
	unsigned char *pAddress, 
	int maxLen,
	int *pLen,
	void **pHandle,
	void **pContext);


/**
 * Finishes a pending host name lookup operation.
 * 
 * @param pAddress base of byte array to receive the address
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an 
 *        IPv4 address, 16 if it is an IPv6 address
 * @param handle the handle returned by the gethostbyname_start function
 * @param context the context returned by the gethostbyname_start function
 *
 * @return PCSL_NET_SUCCESS upon success;\n
 * PCSL_NET_WOULDBLOCK if the caller must call the finish function again to 
 * complete the operation;\n
 * PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_gethostbyname_finish(
	unsigned char *pAddress,
	int maxLen,
	int *pLen,
	void *handle,
	void *context);

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
extern int pcsl_network_error(
	void *handle);

/**
 * Gets the name of the local device from the system. This method is
 * called when the <tt>microedition.hostname</tt> system property
 * is retrieved.
 *
 * @param pLocalHost base of char array to receive the host name, Size
 *        of the pLocalHost should be MAX_HOST_LENGTH
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_getLocalHostName(
	char *pLocalHost);

/**
 * Gets the string representation of the local device's IP address.
 * This function returns dotted quad IP address as a string in the 
 * output parameter and not the host name.
 *
 * @param pLocalIPAddress base of char array to receive the local
 *        device's IP address
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_getLocalIPAddressAsString(
	char *pLocalIPAddress);

/**
 * Translates the given IP address into a host name. 
 *
 * @param ipn Raw IP address to translate
 * @param hostname the host name. The value of <tt>host</tt> is set by
 *             this function.
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns PCSL_NET_WOULDBLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns PCSL_NET_WOULDBLOCK.
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 * PCSL_NET_IOERROR if there is a network error;\n 
 */
extern int pcsl_network_getHostByAddr_start(
    int ipn,
    char *hostname, 
	void **pHandle,
	void **pContext);

/**
 * Finishes a pending host name lookup operation.
 * 
 * @param ipn Raw IP address to translate
 * @param hostname the host name. The value of <tt>host</tt> is set by
 *             this function.
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns PCSL_NET_WOULDBLOCK.
 * @param context the context returned by the getHostByAddr_start function
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 * PCSL_NET_IOERROR if there is a network error;\n 
 */
extern int pcsl_network_getHostByAddr_finish(
    int ipn,
    char *hostname, 
	void **pHandle,
	void *context);

/**
 * Converts a binary IP address to Unicode dotted string representation.
 *
 * @param pResult address of where to put the address of the result string,
 *                free the result with pcsl_mem_free when done
 * @param pResultLen length of the result string
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 * PCSL_NET_IOERROR if there is not enough memory to allocate the result;\n 
 */
extern int pcsl_network_addrToString(
   unsigned char *ipBytes,
   unsigned short** pResult,
   int* pResultLen);

/**
 * Gets the port number of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the local port number
 * 
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_getlocalport(
	void *handle,
	int *pPortNumber);

/** 
 * Gets the port number of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the remote port number
 * 
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_getremoteport(
	void *handle,
	int *pPortNumber);

/**
 * Gets an option's value for a platform-specific TCP socket or datagram
 *
 * @param handle handle of an open connection
 * @param flag socket option to get. Must be one of the values defined in
 *             <tt>javax.microedition.io.SocketConnection</tt>
 * @param pOptval returns the option's value
 *
 * @return PCSL_NET_SUCCESS upon success;\n 
 *         PCSL_NET_IOERROR for an error
 */
extern int pcsl_network_getsockopt(
	void *handle,
	int flag,
	int *pOptval);


/**
 * Sets an option's value for a platform-specific TCP socket or datagram
 *
 * @param handle handle of an open connection
 * @param flag socket option to set. Must be one of the values defined in
 *             <tt>javax.microedition.io.SocketConnection</tt>
 * @param optval the value to be set for this option
 *
 * @return PCSL_NET_SUCCESS upon success;\n
 *         PCSL_NET_IOERROR is returned if the option is not supported \n
 *         PCSL_NET_INVALID is returned if the platform did not accept the
 *         value for the option changed.
 */
extern int pcsl_network_setsockopt(
	void *handle,
	int flag,
	int optval);
	
/**
 * Converts a uint32 value from host format to the network format
 *
 * @param value value to be converted
 *
 * @return converted value
 */
extern unsigned int pcsl_network_htonl(
    unsigned int value);

/**
 * Converts a uint32 value from network format to the host format
 *
 * @param value value to be converted
 *
 * @return converted value
 */
extern unsigned int pcsl_network_ntohl(
    unsigned int value);
    
/**
 * Converts a uint16 value from host format to the network format
 *
 * @param value value to be converted
 *
 * @return converted value
 */
extern unsigned short pcsl_network_htons(
    unsigned short value);

/**
 * Converts a uint16 value from network format to the host format
 *
 * @param value value to be converted
 *
 * @return converted value
 */
extern unsigned short pcsl_network_ntohs(
    unsigned short value);


/**
 * A pcsl equivalent of BSD inet_ntoa () function.
 * The inet_ntoa() function converts the Internet host address to a string 
 * in standard numbers-and-dots notation. The string is returned in
 * a statically allocated buffer, which subsequent calls will overwrite.
 * 
 * @param ipBytes the IP address of the remote device in the form of byte array
 *
 * @return converted address
 */
extern char * pcsl_inet_ntoa (void *ipBytes);


/** @} */   //End of group High Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_NETWORK_H */
