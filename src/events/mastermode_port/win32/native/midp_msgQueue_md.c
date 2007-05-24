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
#include <windows.h>

#include <midp_mastermode_port.h>
#include <win32app_export.h>

MidpReentryData* pSignalResult;
MidpEvent* pMidpEventResult;
int appManagerRequestWaiting = 0;

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
 */
void checkForSystemSignal(MidpReentryData* pNewSignal,
                          MidpEvent* pNewMidpEvent,
                          jlong timeout) {
    MSG msg;
    unsigned long before;
    unsigned long after;

    if (timeout > 0) {
        before = (unsigned long)GetTickCount();
    }
    
    /*
     * The only way to get output from the window procedure is to use
     * shared data.
     */
    pSignalResult = pNewSignal;
    pMidpEventResult = pNewMidpEvent;

    do {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (timeout == 0) {
                return;
            }

            /*
             * Unblock at the specified timeout even if not messages are
             * present.
             * A negative timeout value means to block until an event comes in.
             */
            if (timeout > 0) {
                if (SetTimer(win32app_get_window_handle(), EVENT_TIMER_ID,
                       (UINT)timeout, NULL) == 0) {
                    /* Timer already exists from last time. */
                    KillTimer(win32app_get_window_handle(), EVENT_TIMER_ID);
                    
                    if (SetTimer(win32app_get_window_handle(), EVENT_TIMER_ID,
                                 (UINT)timeout, NULL) == 0) {
                        /* Can't set up the timer so do not block. */
                        return;
                    }
                }
            }

            GetMessage(&msg, NULL, 0, 0);
        }

        /* Dispatching the message will call WndProc below. */
        appManagerRequestWaiting = 0;
        DispatchMessage(&msg);

        if (appManagerRequestWaiting ||
               pSignalResult->waitingFor != NO_SIGNAL) {
            /* We got signal to unblock a Java thread. */
            return;
        }

        if (timeout < 0) {
            /* Wait until there is a signal for MIDP. */
            continue;
        }

        if (timeout > 0) {
            after = (unsigned long)GetTickCount();
            if (after > before) {
                timeout -= (jlong)(after - before);
            } else {
                /* The tick count has wrapped. (happens every 49.7 days) */
                timeout -= (jlong)((unsigned long)0xFFFFFFFF - before + after);
            }
        }
    } while (timeout > 0);
}
