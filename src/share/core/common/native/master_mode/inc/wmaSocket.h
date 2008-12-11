/*
 *  
 *
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

#ifndef WMA_SOCKET_H
#define WMA_SOCKET_H

#include <jsr120_types.h>
#include <stdlib.h>
#include <stdio.h>

#include <java_types.h>
#include <midpServices.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * This file defines the WMASocket class and methods to associate
 * network events with the platform event loop.
 */

/**
 * Creates a platform-specific handle.
 *
 * @param fd platform-specific file descriptor to be associated with
 *		the new handle
 *
 * @return the platform-specific handle; NULL if there was an error
 */
void *wmaCreateSocketHandle(WMA_PROTOCOLS protocol, int fd);

/**
 * Gets the platform-specific file descriptor associated with the
 * given handle.
 *
 * @param handle platform-specific handle to the open connection
 *
 * @return the associated platform-specific file descriptor; a negative
 *	   number if there was an error
 */
int wmaGetRawSocketFD(void *handle);

/**
 * Destroys a platform-specific handle and releases any resources used
 * by the handle.
 *
 * @param handle platform-specific handle to destroy
 *
 * @return 0 if successful; a non-zero value if there was an error
 */
void wmaDestroySocketHandle(void *handle);

#ifdef __cplusplus
}
#endif

#endif
