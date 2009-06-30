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

#ifndef _MASTERMODE_CHECK_SIGNAL_H_
#define _MASTERMODE_CHECK_SIGNAL_H_

/**
 * @file
 *
 * Utility functions to check for system signals.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * In the case there are a signals that can't be checked with a single system
 * call, the proper response time should be guaranteed for all awaited signals.
 */
#define DEFAULT_RESPONSE_TIME 500

/**
 * The signature of system signal checker.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 * @param timeout           >0 the time system can be blocked waiting for a signal
 *                          =0 don't block the system, check for signals instantly
 *                          <0 block the system until a signal received
 *
 * @return KNI_TRUE if a signal received, KNI_FALSE otherwise
 */
typedef jboolean (*fCheckForSignal)(/*OUT*/ MidpReentryData* pNewSignal,
    /*OUT*/ MidpEvent* pNewMidpEvent, jlong timeout);

/**
 * Check for pending signals.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 * @param currenTime        current system time to check time based pending signals
 *
 * @return KNI_TRUE if a pending signal received, KNI_FALSE otherwise
 */
jboolean checkForPendingSignals(/*OUT*/ MidpReentryData* pNewSignal,
    /*OUT*/ MidpEvent* pNewMidpEvent, jlong currentTime);

/** Static list of registered system signal checkers */
extern fCheckForSignal checkForSignal[];

/** Number of registered system signal checkers */
extern int checkForSignalNum;

/**
 * Calls each signal checker with evaluated timeout per checker.

 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 * @param timeout           >0 the time system can be blocked waiting for a signal
 *                          =0 don't block the system, check for signals instantly
 *                          <0 block the system until a signal received
 *
 * @return KNI_TRUE as soon as a signal was detected by one of the checkers.
 *      KNI_FALSE will be return if no signals were receieved.
 */
jboolean checkForAllSignals(/*OUT*/ MidpReentryData* pNewSignal,
    /*OUT*/ MidpEvent* pNewMidpEvent, jlong timeout);


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _MASTERMODE_CHECK_SIGNAL_H_ */
