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
#ifndef _MIDP_MASTERMODE_PORT_H
#define _MIDP_MASTERMODE_PORT_H


/**
 * @defgroup events_master Master Mode Specific Porting Interface
 * @ingroup events
 */

/**
 * @file
 * @ingroup events_master
 * @brief Porting interface for platform specific event handling in master mode.
 */ 

#include <kni.h>
#include <jvm.h>
#include <sni.h>

#include <midpEvents.h>
#include <midpServices.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function is called by the VM periodically. It has to check if
 * system has sent a signal to MIDP and return the result in the
 * structs given.
 *
 * Values for the <timeout> parameter:
 *  >0 = Block until a signal sent to MIDP, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the system for a signal but do not block. Return to the
 *       caller immediately regardless of the if a signal was sent.
 *  -1 = Do not timeout. Block until a signal is sent to MIDP.
 */
extern void checkForSystemSignal(MidpReentryData* pNewSignal,
                                 MidpEvent* pNewMidpEvent,
                                 jlong timeout);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_MASTERMODE_PORT_H */
