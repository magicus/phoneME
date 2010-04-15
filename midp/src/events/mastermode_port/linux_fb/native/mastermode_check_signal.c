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
 * Utility functions to check for system signals.
 */

#include <kni.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <midp_net_events.h>
#include <midp_logging.h>
#include <pcsl_network_generic.h>
#include <pcsl_network.h>
#include <fbapp_export.h>
#include <timer_queue.h>

#ifdef ENABLE_JSR_82_SOCK
#include <bt_generic.h>
#endif

#include "mastermode_check_signal.h"
#include "mastermode_handle_signal.h"

/* Forward declarations */
static jboolean checkForSocketPointerAndKeyboardSignal(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong timeout64);

static jboolean checkForNetworkStatusSignal(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong timeout64);

/** Static list of registered system signal checkers */
fCheckForSignal checkForSignal[] = {
    checkForSocketPointerAndKeyboardSignal,
    checkForNetworkStatusSignal,
    // ...
};

/** Number of registered system signal checkers */
int checkForSignalNum =
    sizeof(checkForSignal) / sizeof(fCheckForSignal);

/**
 * Check and handle socket & pointer & keyboard system signals.
 * The function groups signals that can be checked with a single system call.
 *
 * @param pNewSignal        OUT reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     OUT a native MIDP event to be stored to Java event queue
 * @param timeout64         IN  >0 the time system can be blocked waiting for a signal
 *                              =0 don't block the system, check for signals instantly
 *                              <0 block the system until a signal received
 *
 * @return KNI_TRUE if signal received, KNI_FALSE otherwise
 */
static jboolean checkForSocketPointerAndKeyboardSignal(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong timeout64) {

    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    int num_fds = 0, num_ready = 0;
    jlong sec, usec;
    struct timeval timeout;

    int mouse_fd = fbapp_get_mouse_fd();
    int keyboard_fd = fbapp_get_keyboard_fd();

    const SocketHandle* socketsList   = GetRegisteredSocketHandles();
#ifdef ENABLE_JSR_82_SOCK
    const SocketHandle* btSocketsList = GetRegisteredBtSocketHandles();
#endif /* ENABLE_JSR_82_SOCK */

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    (void)pNewMidpEvent;

    if (keyboard_fd != -1) {
        /* Set keyboard descriptor for select */
        FD_SET(keyboard_fd, &read_fds);
        if (num_fds <= keyboard_fd) {
            num_fds = keyboard_fd + 1;
        }
    }
    if (mouse_fd != -1) {
        /* Set pointer descriptor for select */
        FD_SET(mouse_fd, &read_fds);
        if (num_fds <= mouse_fd) {
            num_fds = mouse_fd + 1;
        }
    }

    /* Set the sockets to be checked during select */
    setSockets(socketsList, &read_fds, &write_fds, &except_fds, &num_fds);
#ifdef ENABLE_JSR_82_SOCK
    setSockets(btSocketsList, &read_fds, &write_fds, &except_fds, &num_fds);
#endif /* ENABLE_JSR_82_SOCK */

    /* Listen to collected descriptors during specified time interval */
    if (num_fds > 0) {

        if (timeout64 < 0) {
            num_ready = select(
                num_fds, &read_fds, &write_fds, &except_fds, NULL);
        } else {
            sec  = timeout64 / 1000;
            usec = (timeout64 % 1000) * 1000;
            timeout.tv_sec  = sec  & 0x7fffffff;
            timeout.tv_usec = usec & 0x7fffffff;
            num_ready = select(
                num_fds, &read_fds, &write_fds, &except_fds, &timeout);
        }
    }

    if (num_ready > 0) {
        if (keyboard_fd != -1 && FD_ISSET(keyboard_fd, &read_fds)) {
            /* Handle keyboard event */
            REPORT_INFO(LC_CORE, "[checkForSocketPointerAndKeyboardSignal] keyboard signal detected");
            handleKey(pNewSignal, pNewMidpEvent);
        } else if (mouse_fd != -1 && FD_ISSET(mouse_fd, &read_fds)) {
            /* Handle pointer event */
            REPORT_INFO(LC_CORE, "[checkForSocketPointerAndKeyboardSignal] pointer signal detected");
            handlePointer(pNewSignal, pNewMidpEvent);
        } else {
            REPORT_INFO(LC_CORE, "[checkForSocketPointerAndKeyboardSignal] socket signal detected");
            handleSockets(socketsList,
                          &read_fds, &write_fds, &except_fds, pNewSignal);
#ifdef ENABLE_JSR_82_SOCK
            handleSockets(btSocketsList,
                          &read_fds, &write_fds, &except_fds, pNewSignal);
#endif /* ENABLE_JSR_82_SOCK */
        }
        return KNI_TRUE;
    } /* num_ready > 0 */
    
    /* No pending signals were detected */
    return KNI_FALSE;
}

/**
 * Check and handle network status (up/down) system signal.
 *
 * @param pNewSignal        OUT reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     OUT a native MIDP event to be stored to Java event queue
 * @param timeout64         IN  >0 the time system can be blocked waiting for a signal
 *                              =0 don't block the system, check for signals instantly
 *                              <0 block the system until a signal received
 *
 * @return KNI_TRUE if signal received, KNI_FALSE otherwise
 */
static jboolean checkForNetworkStatusSignal(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong timeout64) {

    int status;
    jboolean res = midp_check_net_status_signal(&status);

    (void)pNewMidpEvent;
    (void)timeout64;

    if (res == KNI_TRUE) {
        pNewSignal->descriptor = 0;
        pNewSignal->waitingFor = NETWORK_STATUS_SIGNAL;
        pNewSignal->status = status;
    }

    return res;
}

/**
 * Check and handle key signal pending since the previous check.
 *
 * @param pNewSignal        OUT reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     OUT a native MIDP event to be stored to Java event queue
 *
 * @return KNI_TRUE if pending key signal detected, KNI_FALSE otherwise
 */
jboolean checkForPendingKeySignal(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent) {

    if (hasPendingKey()) {
        REPORT_INFO(LC_CORE, "[checkForPendingKeySignal] pending key detected");
        handleKey(pNewSignal, pNewMidpEvent);
        return KNI_TRUE;
    }
    return KNI_FALSE;
}

/**
 * Check and handle all timer alarms expired to the current time.
 *
 * @param currenTime        IN  current system time to check timers expiration
 *
 * @return KNI_TRUE if expired timer detected, KNI_FALSE otherwise
 */
jboolean checkForPendingTimerSignal(jlong currentTime) {

    TimerHandle* prev = NULL;
    TimerHandle* timer = peek_timer();
    jboolean hasPendingTimer = KNI_FALSE;

    /* Process all expired timer alarms */
    while (timer != NULL && timer != prev) {
        jlong wakeupTime = get_timer_wakeup(timer);
        if (wakeupTime > currentTime) {
            break;
        }

        /* Timer handler is responsible to remove its timer */
        wakeup_timer(timer);
        hasPendingTimer = KNI_TRUE;
        prev = timer;
        timer = peek_timer();
    }

    return hasPendingTimer;
}

/**
 * Check for pending signals.
 *
 * @param pNewSignal        OUT reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     OUT a native MIDP event to be stored to Java event queue
 * @param currenTime        IN  current system time to check time based pending signals
 *
 * @return KNI_TRUE if a pending signal received, KNI_FALSE otherwise
 */
jboolean checkForPendingSignals(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong currentTime) {

    if (checkForPendingKeySignal(pNewSignal, pNewMidpEvent)) {
        return KNI_TRUE;
    }
    return checkForPendingTimerSignal(currentTime);
}

/**
 * Calls each signal checker with evaluated timeout per checker.

 * @param pNewSignal        OUT reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     OUT a native MIDP event to be stored to Java event queue
 * @param timeout           IN  >0 the time system can be blocked waiting for a signal
 *                              =0 don't block the system, check for signals instantly
 *                              <0 block the system until a signal received
 *
 * @return KNI_TRUE as soon as a signal was detected by one of the checkers.
 *      KNI_FALSE will be return if no signals were receieved.
 */
jboolean checkForAllSignals(MidpReentryData* pNewSignal,
    MidpEvent* pNewMidpEvent, jlong timeout) {

    jlong spentTime = 0;
    jlong timeoutPerChecker = timeout;

    /* Evaluate time per checker */
    if (checkForSignalNum > 1) {
        /* The next call of an each checker should happen
         * not later than after a specified response time */
        if (timeout > DEFAULT_RESPONSE_TIME || timeout < 0) {
            timeoutPerChecker = DEFAULT_RESPONSE_TIME / checkForSignalNum;
        } else if (timeout > 0) {
            timeoutPerChecker = timeout / checkForSignalNum;
        }
        REPORT_INFO3(LC_CORE,
            "[checkForAllSignals] share timeout=%lu between %d checkers, timeoutPerChecker=%lu",
            (long)timeout, checkForSignalNum, (long)timeoutPerChecker);
    }

    /* Call all registered signal checkers during timeout & break checking
     * on any signal receipt. Remember the next checker to start from it when
     * the signals checking will be requested next time. It should guarantee
     * equal priorities for all registered checkers */
    do {
        int cnt = 0;
        static int i = 0;
        while (cnt++ < checkForSignalNum) {
            if (checkForSignal[i](
                    pNewSignal, pNewMidpEvent, timeoutPerChecker)) {
                return KNI_TRUE;
            }
            /* Timeout shouldn't be exceeded */
            spentTime += timeoutPerChecker != 0 ? timeoutPerChecker : 1;
            if (timeout > 0 && spentTime >= timeout) {
                break;
            }
            /* Cyclic increment of the current checker number */
            i = (i+1) % checkForSignalNum;
        }
    } while (timeout < 0 || spentTime < timeout);
    /* No signals were received during timeout */
    return KNI_FALSE;
}
