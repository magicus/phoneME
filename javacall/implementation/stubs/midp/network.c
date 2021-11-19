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
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * Stub implementation of pcsl_network.h for platforms that support the winsock 
 * API.
 *
 * For all functions, the "handle" is the winsock handle (an int)
 * cast to void *.  Since winsock reads and writes to sockets are synchronous,
 * the context for reading and writing is always set to NULL.
 */
   
#include "javacall_network.h" 
    
/**
 * Performs platform-specific initialization of the networking system.
 * Will be called ONCE during VM startup before opening a network connection.
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_WOULD_BLOCK caller must call the finish function to 
 * 			complete the operation
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_init_start(void) {
    return JAVACALL_FAIL;                                        
}


/**
 * Finishes platform-specific initialization of the networking system.
 * The function is invoked be the JVM after receiving JAVACALL_NETWORK_UP
 * notification from the platform.
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_WOULD_BLOCK caller must call the finish function to 
 * 			complete the operation
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_init_finish(void) {
    return JAVACALL_FAIL;                                        
}  


/**
 * Performs platform-specific finalization of the networking system.
 * Will be called ONCE during VM shutdown.
 *
 * @retval JAVACALL_WOULD_BLOCK caller must call xxx_finalize_finish
 *         function to complete the operation
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize_start(void){
    return JAVACALL_FAIL;
}

/**
 * Finishes platform-specific finalize of the networking system.
 * The function is invoked be the JVM after receiving JAVACALL_NETWORK_DOWN
 * notification from the platform.
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize_finish(void)
{
    return JAVACALL_FAIL;
}

  
/**
 * Performs platform-specific finalizaiton of the networking system.
 * Will be called ONCE during VM shutdown!
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize(void) {
    return JAVACALL_FAIL;                                        
}  
    
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
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULD_BLOCK.
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if there is a network error
 * @retval JAVACALL_INVALID_ARGUMENTS   if maxLen is too small to receive the address
 */
javacall_result javacall_network_gethostbyname_start(char *hostname,
    unsigned char *pAddress, int maxLen, /*OUT*/ int *pLen, /*OUT*/ void **pHandle, /*OUT*/ void **pContext) {
    return JAVACALL_FAIL;                                        
}  
    
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
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function again to complete the operation;\n
 * @retval JAVACALL_FAIL        if there is a network error
 */
javacall_result javacall_network_gethostbyname_finish(unsigned char *pAddress,
    int maxLen,int *pLen,void *handle,void *context) {
    return JAVACALL_FAIL;                                        
}  

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
javacall_result /*OPTIONAL*/ javacall_network_getsockopt(javacall_handle handle,javacall_socket_option flag, /*OUT*/ int *pOptval) {
    return JAVACALL_FAIL;
}
    
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
 * @retval JAVACALL_INVALID_ARGUMENTS   if the platform did not accept the value for the option changed
 */
javacall_result /*OPTIONAL*/ javacall_network_setsockopt(javacall_handle handle, javacall_socket_option flag, int optval) {
    return JAVACALL_FAIL;
}

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
    char *hostname, /*OUT*/ javacall_handle* pHandle, void **pContext) {

    /*
     * Note there is no spec requirement to look up the the address to
     * to get a hostname. It is not a good idea to perform the lookup,
     * since this function is called EVERY time
     * a datagram is received and on a cell phone network this lookup
     * can take some time and really effect performance for receiving
     * datagrams.
     */

    return JAVACALL_FAIL;
}
    
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
    char *hostname, /*OUT*/ javacall_handle* pHandle, void *context) {
    return JAVACALL_FAIL;
}

/**
 * Gets the IP address of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getlocaladdr(
    javacall_handle handle,
    char *pAddress) {
    return JAVACALL_FAIL;
}
/**
 * Gets the IP address of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getremoteaddr(
    void *handle,
    char *pAddress)
{
    return JAVACALL_FAIL;
}
   
/**
 * Gets the port number of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the local port number
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getlocalport(javacall_handle handle,int *pPortNumber) {
    return JAVACALL_FAIL;
}
/**
 * Gets the port number of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the local port number
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result javacall_socket_getremoteport(
    void *handle,
    int *pPortNumber)
{
    return JAVACALL_FAIL;
}

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
javacall_result /*OPTIONAL*/ javacall_network_get_local_ip_address_as_string(/*OUT*/ char *pLocalIPAddress) {
    return JAVACALL_FAIL;
}

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
javacall_result /*OPTIONAL*/ javacall_network_get_http_proxy(/*OUT*/ char *pHttpProxy, /*OUT*/ char *pHttpsProxy) {
    return JAVACALL_FAIL;
}

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
int /*OPTIONAL*/ javacall_network_error(javacall_handle handle) {
    return 0;
}

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
javacall_result /*OPTIONAL*/ javacall_network_get_local_host_name(/*OUT*/ char *pLocalHost) {
    return JAVACALL_FAIL;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_server_socket_set_notifier(javacall_handle handle, javacall_bool set) {
    return JAVACALL_FAIL;
}

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
char* javacall_inet_ntoa(void *address) {
    return NULL;
}

/** @} */
#ifdef __cplusplus
}
#endif

