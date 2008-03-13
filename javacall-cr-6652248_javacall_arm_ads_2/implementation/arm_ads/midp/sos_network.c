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

/** @file
*
* This file implements all the necessary PCSL interfaces for SoS client.
* Please note that SoS Proxy server must be started before SoS client. SoS
* client and proxy server communication is based on RPC mechanism.
* 
* This version supports multiple sockets.
* 
*
* for SoS network, replace javacall_impl\<your platform>\src\network.c by this file.
*/

#include "javacall_network.h"
#include "javacall_socket.h"

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_open_start(unsigned char *ipBytes,
                                           int port,
                                           void **pHandle,
                                           void **pContext) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_socket_open_finish(void *handle,
                                            void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_read_start(void *handle,
                                           unsigned char* buf,
                                           int numbytes,
                                           int *pBytesRead,
                                           void **pContext) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_socket_read_finish(void *handle,
                                            unsigned char *pData,
                                            int len,
                                            int *pBytesRead,
                                            void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_write_start(void *handle,
                                            char* buf,
                                            int numbytes,
                                            int *pBytesWritten,
                                            void **pContext) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_socket_write_finish(void *handle,
                                             char *pData,
                                             int len,
                                             int *pBytesWritten,
                                             void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_available(void *handle, int *pBytesAvailable) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_close_start(void *handle, void **pContext) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_close_finish(void *handle,
                                             void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_shutdown_output(void *handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_network_gethostbyname_start(char *hostname,
                                                     unsigned char *pAddress,
                                                     int maxLen,
                                                     int *pLen,
                                                     void **pHandle,
                                                     void **pContext) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_network_gethostbyname_finish(unsigned char *pAddress,
                                                      int maxLen,
                                                      int *pLen,
                                                      void *handle,
                                                      void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_socket_getsockopt(void *handle,
                                           int flag,
                                           int *pOptval) { 
    return JAVACALL_NOT_IMPLEMENTED;
}   

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_setsockopt(void *handle,
                                           int flag,
                                           int optval) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_getlocaladdr(void *handle,
                                             char *pAddress) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_getremoteaddr(void *handle,
                                              char *pAddress) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_getlocalport(void *handle,
                                             int *pPortNumber) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_socket_getremoteport(void *handle,
                                              int *pPortNumber) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_network_init(void) {
    return JAVACALL_NOT_IMPLEMENTED;
} 

/**
 * See javacall_network.h for definition.
 */ 
int javacall_network_error(void *handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_network_getLocalHostName(char *pLocalHost) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */ 
javacall_result javacall_network_getLocalIPAddressAsString(char *pLocalIPAddress) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_network_getlocalport(void *handle,
                                              int *pPortNumber) {
    return JAVACALL_NOT_IMPLEMENTED;
}


javacall_result javacall_network_close(void) {
    return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_network_getremoteport(void *handle,
                                               int *pPortNumber) {
    return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_network_getsockopt(javacall_handle handle,
                                            javacall_socket_option flag,
                                            int *pOptval) {
    return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_network_setsockopt(javacall_handle handle,
                                            javacall_socket_option flag,
                                            int optval) {
    return JAVACALL_NOT_IMPLEMENTED;
}

