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

#ifndef _MIDP_SLAVEMODE_PORT_H_
#define _MIDP_SLAVEMODE_PORT_H_

#include "java_types.h"


/**
 * @defgroup events_slave Slave Mode Specific Porting Interface
 * @ingroup events
 */

/**
 * @file
 * @ingroup events_master
 * @brief Porting interface for platform specific event handling in slave mode.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Requests that the VM control code schedule a time slice as soon
 * as possible, since Java platform threads are waiting to be run.
 */
void midp_slavemode_schedule_vm_timeslice(void);


/**
 * Executes bytecodes for a time slice
 *
 * @return <tt>-2</tt> if JVM has exited
 *         <tt>-1</tt> if all the Java threads are blocked waiting for events
 *         <tt>timeout value</tt>  the nearest timeout of all blocked Java threads
 */
jlong midp_slavemode_time_slice(void);


/**
 * Runs the platform-specific event loop.
 */
void midp_slavemode_event_loop(void);

/**
 * This function is called when the network initialization
 * or finalization is completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
void midp_network_status_event_port(int isInit, int status);

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _MIDP_SLAVEMODE_PORT_H_ */
