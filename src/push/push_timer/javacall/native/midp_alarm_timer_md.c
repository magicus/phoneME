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

#include <stdio.h>

#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <java_types.h>

#include <midpServices.h>
#include <timer_export.h>
#include <midp_logging.h>

#include <javacall_time.h>
#include <midp_jc_event_defs.h>
#include <javacall_events.h>

/**
 * @file
 *
 * Implementation of a alarm timer.
 */

/**
 * A callback function called by javacall when the timer triggers.
 *
 * @param handle Handle of the triggered timer
 */
static void
push_alarm_handler(javacall_handle handle) {
    midp_jc_event_union e;
    e.eventType  = MIDP_JC_EVENT_PUSH;
    e.data.pushEvent.alarmHandle = (int)handle;
#if !ENABLE_CDC
    javacall_event_send((unsigned char*)&e,sizeof(midp_jc_event_union));
#else
    javacall_event_send_cvm(MIDP_EVENT_QUEUE_ID, (unsigned char*)&e,
        sizeof(midp_jc_event_union));
#endif
}

/**
 * Create and start a new alarm timer, returning its handle.
 *
 * @param alarmHandle Handle to the alarm to create a timer for
 * @param time Time from the current moment to activate the alarm
 * @return Implementation specific handle of the newly created timer
 */
int
createTimerHandle(int alarmHandle, jlong time) {
    /* alarmHandle is really an address to push entry */
    javacall_handle handle;

    (void)alarmHandle;
    if (JAVACALL_OK != 
        javacall_time_initialize_timer ((int)time,
                                        JAVACALL_FALSE,
                                        &push_alarm_handler,
                                        &handle) ) {
        return -1;
    } else {
        return (int)handle;
    };
}

/**
 * Destroy the alarm timer given its implementation specific handle.
 *
 * @param timerHandle The implementation specific handle to the timer object
 * @return 0 if successful
 */
int
destroyTimerHandle(int timerHandle) {
    if (JAVACALL_OK == javacall_time_finalize_timer ((void *)timerHandle))
        return 0;
    else
        return -1;
}

