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

#ifndef _MIDP_NET_EVENTS_H
#define _MIDP_NET_EVENTS_H

#include <java_types.h>

/**
 * @file
 * @ingroup eventsystem
 * @brief Platform-specific routine to check for network events.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if a network status signal is received.
 *
 * @param pStatus on exit will hold a new network status (1 - up, 0 - down)
 *
 * @return KNI_TRUE if a network status signal was received, KNI_FALSE otherwise
 */
jboolean midp_check_net_status_signal(int* pStatus);

/**
 * This function is called when the network initialization
 * or finalization is completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
void midp_network_status_event(int isInit, int status);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_NET_EVENTS_H */
