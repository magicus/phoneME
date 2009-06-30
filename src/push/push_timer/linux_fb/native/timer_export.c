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

 /**
  * @file
  *
  * Implementation of the porting interfaces needed
  * to support timer alarms in the push subsystem.
  */

#include <stdlib.h>
#include <jvm.h>
#include <midp_logging.h>
#include <midp_thread.h>
#include <timer_queue.h>
#include <push_server_export.h>

/** Handle push timer alarm on timer wakeup & free timer instance */
static void handlePushTimerAlarm(TimerHandle *timer) {
    if (timer != NULL) {
        int pushHandle = (int)(get_timer_data(timer));
        if (findPushTimerBlockedHandle(pushHandle) != 0) {
            /* The push system is waiting for this alarm */
            REPORT_INFO1(LC_PUSH,
                "[handlePushTimerAlarm] timer alarm with pushHandle=%#x",
                pushHandle);
            midp_thread_signal(PUSH_SIGNAL, 0, 0);
        }
        delete_timer(timer);
    }
}

/**
 * Destroy the alarm timer given its implementation specific handle.
 * In this implementation alarm handle is used also as a timer handle.
 *
 * @param timerHandle The implementation specific handle to the timer object
 * @return 0 if successful
 */
int destroyTimerHandle(int pushHandle) {

    REPORT_INFO1(LC_PUSH, "[destroyTimerHandle] pushHandle=%#x", pushHandle);
    delete_timer_by_userdata((void*)pushHandle);
    return 0;
}

/**
 * Create and start a new alarm timer, returning its handle.
 * In this implementation alarm handle is used also as a timer handle.
 *
 * @param alarmHandle Handle to the alarm to create a timer for
 * @param time Time from the current moment to activate the alarm
 * @return Implementation specific handle of the newly created timer
 */
int createTimerHandle(int pushHandle, jlong wakeupInMilliSeconds) {
    TimerHandle* timer;
    jlong currentTime = JVM_JavaMilliSeconds();
    REPORT_INFO2(LC_PUSH, "[createTimerHandle] pushHandle=%#x, wakeupInSeconds=%lu",
        pushHandle, (long)wakeupInMilliSeconds);

    timer = new_timer(currentTime + wakeupInMilliSeconds,
        (void*)pushHandle, (void*)handlePushTimerAlarm);
    return pushHandle;
}
