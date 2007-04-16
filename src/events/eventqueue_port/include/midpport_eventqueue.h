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

#ifndef _MIDPPORT_EVENTQUEUE_H_
#define _MIDPPORT_EVENTQUEUE_H_

/**
 * @defgroup events_queueport Event Queue Porting Interface
 * @ingroup events
 */

/**
 * @file
 * @ingroup events_queueport
 *
 * @brief Porting interface for the event queue.
 */

#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Contains all the fields needed for any MIDP event.
 * See StoreMIDPEvent below for usage.
 */
typedef struct _MidpEvent {
    /** Must be one of the types below. */
    jint type;
    /** Generic int parameter 1. */
    jint intParam1;
    /** Generic int parameter 2. */
    jint intParam2;
    /** Generic int parameter 3. */
    jint intParam3;
    /** Generic int parameter 4. */
    jint intParam4;
    /** Generic int parameter 5. */
    jint intParam5;
    /** Generic String parameter 1. */
    pcsl_string stringParam1;
    /** Generic String parameter 2. */
    pcsl_string stringParam2;
    /** Generic String parameter 3. */
    pcsl_string stringParam3;
    /** Generic String parameter 4. */
    pcsl_string stringParam4;
    /** Generic String parameter 5. */
    pcsl_string stringParam5;
    /** Generic String parameter 6. */
    pcsl_string stringParam6;
} MidpEvent;

/** Create the event queue lock. */
void midp_createEventQueueLock(void);

/** Destroy the event queue lock. */
void midp_destroyEventQueueLock(void);

/** Wait to get the event queue lock and then lock it. */
void midp_waitAndLockEventQueue(void);

/** Unlock the event queue. */
void midp_unlockEventQueue(void);

/**
 * Store an event to post to the Java platform event queue. Usage:
 * <pre>
 *   MidpEvent event;
 *
 *   MIDP_EVENT_INITIALIZE(event);
 *
 *   event.type = COOL_NEW_EVENT;  // this constant is in midpEvents.h
 *   event.intParam1 = x;
 *   event.intParam2 = y;
 *
 *   StoreMIDPEvent(event, 0);
 * </pre>
 *
 * @param event The event to enqueue.
 * @param isolateId ID of an Isolate or 0 for SMV mode
 */
void StoreMIDPEvent(MidpEvent event, int isolateId);

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _MIDPPORT_EVENTQUEUE_H_ */
