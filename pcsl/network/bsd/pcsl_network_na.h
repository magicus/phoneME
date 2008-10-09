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

#ifndef _PCSL_NETWORK_NA_H
#define _PCSL_NETWORK_NA_H

#include <pcsl_network.h>

/**
 * @file
 * @defgroup bsd BSD Notification Adapter Interface
 * @ingroup network_low
 * @brief Low level interface using Notification Adapter \n
 * ##include <>
 * @{
 *
 * Notification adapter is an abstraction layer that implements certain
 * platform specific functions which are used internally by the generic BSD
 * implementation of PCSL networking interfaces. This layer is specifically
 * created to isolate the platform specific implementation from a generic
 * BSD implementation. So, for all BSD compatible platforms, only interfaces
 * defined in this file need to be ported without modifying the generic BSD
 * implementation of PCSL networking interfaces. Note that this abstraction
 * layer is transparent for generic BSD layer. Notification adapter object
 * is uniquely identified by a socket descriptor. A "handle" will actually 
 * be a reference to this notification adapter object.
 *
 * This file declares notification adapter functions which should be 
 * implemented for a particular platform. These functions are used by 
 * pcsl_network.c.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a notification adapter object for this fd.
 * @param fd BSD Socket descriptor
 *
 * @return Platform specific handle which is actually
 *        a reference pointer to this notification adapter object
 */
void* na_create(int fd);

/**
 * Extracts a file descriptor from a handle
 * @param handle : Platform specific handle that represents an 
 *          open connection
 *
 * @return fd BSD Socket descriptor
 */
int na_get_fd(void *handle);

/**
 * Requests a callback when pcsl_socket_read_start()  occurs on this 
 * handle AND it returns with PCSL_NET_WOULDBLOCK
 * @param handle : Platform specific handle that represents an 
 *          open connection
 */
void na_register_for_read(void *handle);

/**
 * Requests a callback when pcsl_socket_write_start()  occurs on this 
 * handle AND it returns with PCSL_NET_WOULDBLOCK
 * @param handle : Platform specific handle that represents an 
 *          open connection
 */
void na_register_for_write(void *handle);

/**
 * Cancels a callback requested for when a blocked read
 * activity is finished
 * @param handle : Platform specific handle that represents an 
 *          open connection
 */
void na_unregister_for_read(void *handle);

/**
 * Cancels a callback requested for when a blocked write
 * activity is finished
 * @param handle : Platform specific handle that represents an 
 *          open connection
 */
void na_unregister_for_write(void *handle);

/**
 * Checks the status of a notification adapter object for any pending 
 * read or write operations
 * @param handle : Platform specific handle that represents an 
 *          open connection
 *
 * @return PCSL_NET_SUCCESS if operations are still allowed on 
 *                          this resource or
 *         PCSL_NET_INTERRUPTED if the resource has been closed.
 */
int na_get_status(void *handle);

/**
 * Destroys the notification adapter for this handle. There is no explicit
 * need to unregister the handle before destroying it
 * @param handle : Platform specific handle that represents an 
 *          open connection
 */
void na_destroy(void *handle);

/** @} */ //End of group Low Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_NETWORK_NA_H */


