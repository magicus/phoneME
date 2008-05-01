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

#ifndef _MASTERMODE_HANDLE_SIGNAL_H_
#define _MASTERMODE_HANDLE_SIGNAL_H_

/**
 * @file
 *
 * Utility functions to handle received system signals.
 */

#include <timer_queue.h>
#include <pcsl_network_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepare read/write/exception descriptor sets with data from
 * socket list for suceeded select() query
 *
 * @param socketsList list of sockets registered for read/write notifications
 * @param pRead_fds set of descriptors to check for read signals
 * @param pWrite_fds set of descriptors to check for write signals
 * @param pExcept_fds set of descriptors to check for exception signals
 * @param pNum_fds upper bound of checked descriptor values
 */
void setSockets(const SocketHandle* socketsList,
        /*OUT*/ fd_set* pRead_fds, /*OUT*/ fd_set* pWrite_fds,
        /*OUT*/ fd_set* pExcept_fds, /*OUT*/ int* pNum_fds);

/**
 * Handle received socket signal and prepare reentry data
 * to unblock a thread waiting for the signal.
 *
 * @param socketsList list of sockets registered for read/write notifications
 * @param pRead_fds set of descriptors to check for read signals
 * @param pWrite_fds set of descriptors to check for write signals
 * @param pExcept_fds set of descriptors to check for exception signals
 * @param pNewSignal reentry data to unblock a threads waiting for a socket signal
 */
void handleSockets(const SocketHandle* socketsList,
        fd_set* pRead_fds, fd_set* pWrite_fds, fd_set* pExcept_fds,
        /*OUT*/ MidpReentryData* pNewSignal);

/**
 * Handle received keyboard/keypad signals
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handleKey(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent);

/**
 * Handle received pointer signals
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handlePointer(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent);
    
/**
 * An input devices can produce bit-based keyboard events. Thus single
 * native event can produce several MIDP ones. This function detects
 * whether are one or more key bits still not converted into MIDP events
 *
 * @return true when pending key exists, false otherwise
 */
jboolean hasPendingKey();

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _MASTERMODE_HANDLE_SIGNAL_H_ */
