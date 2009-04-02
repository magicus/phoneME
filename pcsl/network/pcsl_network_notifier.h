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
#ifndef _PCSL_NETWORK_NOTIFIER_H
#define _PCSL_NETWORK_NOTIFIER_H

/**
 * @defgroup networknotifier Network Notification Interface
 * @ingroup network_high
 */

/**
 * @file
 * @ingroup network
 * @brief PCSL networking interfaces for network events notification \n
 * ##include <>
 * @{
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Network notifier event type for read operation.
 */
#define PCSL_NET_CHECK_READ 0x01

/**
 * Network notifier event type for write operation.
 */
#define PCSL_NET_CHECK_WRITE 0x02

/**
 * Network notifier event type for accept operation on serversocket.
 */
#define PCSL_NET_CHECK_ACCEPT 0x04

/**
 * Network notifier event type for case for problems with network operation,
 * e.g. socket has been closed during operation in progress.
 * (need revisit)
 */
#define PCSL_NET_CHECK_EXCEPTION 0x08

/**
 * Registers a notification event for open network connection with the platform-specific
 * event loop. Whenever network activity occurs in the given direction for the connection
 * the platform-specific mechanism for event processing will be triggered.
 *
 * @param handle platform-specific handle to the open connection
 * @param event one of PCSL_CHECK_READ, PCSL_CHECK_WRITE or PCSL_CHECK_EXCEPTION
 * to signal if handle ready to be read from, written to or some problem
 * has happened respectively.
 */
extern void pcsl_add_network_notifier(
    void *handle,
    int event);

/**
 * Unregisters previously registered notification event for open network connection
 * from the platform-specific event loop. Whenever network activity occurs for the
 * connection in the given direction no event processing will be triggered.
 *
 * @param handle platform-specific handle to the open connection
 * @param event one of PCSL_CHECK_READ, PCSL_CHECK_WRITE or PCSL_CHECK_EXCEPTION
 * for not to signal if handle ready to be read from, written to or some problem
 * has happened respectively.
 */
extern void pcsl_remove_network_notifier(
    void *handle,
    int event);

/** @} */   //End of group High Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_NETWORK_NOTIFIER_H */
