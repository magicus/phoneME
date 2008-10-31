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

#ifndef _MIDP_CHECK_EVENTS_H
#define _MIDP_CHECK_EVENTS_H

#include <java_types.h>
#include <jvmspi.h>

/**
 * @file
 * @ingroup eventsystem
 * @brief Platform-specific routine to check for external events.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function is called by the VM periodically. It has to check if
 * any of the blocked threads are ready for execution, and call
 * SNI_UnblockThread() on those threads that are ready.
 *
 * @param blocked_threads Array of blocked threads
 * @param blocked_threads_count Number of threads in the blocked_threads array
 * @param timeout Values for the paramater:
 *                >0 = Block until an event happens, or until <timeout> 
 *                     milliseconds has elapsed.
 *                 0 = Check the events sources but do not block. Return to the
 *                     caller immediately regardless of the status of the event
 *                     sources.
 *                -1 = Do not timeout. Block until an event happens.
 */
void midp_check_events(JVMSPI_BlockedThreadInfo *blocked_threads,
		       int blocked_threads_count,
		       jlong timeout);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_CHECK_EVENTS_H */
