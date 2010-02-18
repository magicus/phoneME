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

#include <jvm.h>
#include <midp_logging.h>
#include <timer_queue.h>

#include "mastermode_check_signal.h"

KNIEXPORT KNI_RETURNTYPE_LONG JVM_JavaMilliSeconds();

/**
 * Adjust timeout value to not miss new comming timer
 * alarms while the system is blocked
 */
static void adjustTimeout(jlong currentTime, jlong *timeout) {
    TimerHandle* timer = peek_timer();
    if (timer != NULL) {
        jlong wakeupTime = get_timer_wakeup(timer);
        if (wakeupTime > currentTime) {
            jlong delta = wakeupTime - currentTime;
            if (*timeout < 0 || *timeout > delta) {
                REPORT_INFO2(LC_PUSH, "[adjustTimeout] timeout=%lu adjusted to %lu",
                    (long)*timeout, (long)delta);

                *timeout = delta;
            }
        }
    }
}

/*
 * This function is called by the VM periodically. It has to check if
 * system has sent a signal to MIDP and return the result in the
 * structs given.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until a signal sent to MIDP, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the system for a signal but do not block. Return to the
 *       caller immediately regardless of the if a signal was sent.
 *  -1 = Do not timeout. Block until a signal is sent to MIDP.
 *
 * IMPL NOTE: this function now is definitely not highlevelui-only. Redesign is
 * strongly recommended
 */
void checkForSystemSignal(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong timeout) {

    jboolean pendingSignal = KNI_FALSE;
    jlong currentTime = JVM_JavaMilliSeconds();

    /* Pending event should be processed prior to other events */
    pendingSignal = checkForPendingSignals(
            pNewSignal, pNewMidpEvent, currentTime);

    if (!pendingSignal) {
        /* Adjust timeout regarding near timers */
        adjustTimeout(currentTime, &timeout);
        /* Call all registered signal checkers during timeout */
        checkForAllSignals(pNewSignal, pNewMidpEvent, timeout);
    }
}
