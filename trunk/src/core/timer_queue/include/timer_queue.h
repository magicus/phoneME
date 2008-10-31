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

#ifndef _TIMER_QUEUE_H_
#define _TIMER_QUEUE_H_

/**
 * @file
 *
 * Timer queue inteface.
 * Can be used to support any timer alarms in the master mode.
 */

#include <kni.h>
#include <java_types.h>
#include <midpServices.h>
#include <midpEvents.h>

/** Opaque timer handle type */
typedef struct _TimerHandle TimerHandle;

/** Signature of timer alarm handler */
typedef void (*fTimerCallback)(TimerHandle* timer);

/**
 * Get user data of the timer
 *
 * @param timer instance of the timer to get user data from
 * @return untyped user data registered during timer creation,
 *    or NULL for NULL timer handle
 */
void *get_timer_data(const TimerHandle *timer);

/**
 * Get absoulte wakeup time of the timer
 *
 * @param timer instance of the timer to get wakeup time from
 * @return absoulte time when the timer should wakeup,
 *    or -1 for NULL timer handle
 */
jlong get_timer_wakeup(const TimerHandle *timer);


/**
 * Set bsoulte wakeup time for the timer
 *
 * @param timer instance of the timer to set wakeup time to
 * @param timeToWakeup absolute time when the timer should wakeup
 */
void set_timer_wakeup(TimerHandle *timer, jlong timeToWakeup);

/**
 * Insert new timer to the correct place in timer queue
 * ordered by wakeup time
 *
 * @param newTimer new timer to add to queue
 */
void add_timer(TimerHandle* newTimer);

/**
 * Create a new entry and enqueue in data-structure
 *
 * @param timeToWakeup the abs time for the timer (used to order the timer queue)
 * @param userData user specified data
 * @param userCallback user specified callback
 *
 * @return poitner to new element if successful,
 *    NULL if there was an error allocating this timer.
 */
TimerHandle* new_timer(
    jlong timeToWakeup, void* userData, fTimerCallback userCallback);

/**
 * Remove specified timer handler from queue and free allocated memory
 *
 * @param timer instance of timer that should be remove
 */
void delete_timer(TimerHandle* timer);

/**
 * Remove specified timer handler from queue but don't free allocated memory
 *
 * @param timer instance that should be remove
 *
 * @return poitner to timer instance that should be remove from queue
 */
TimerHandle* remove_timer(TimerHandle* timer);


/**
 * Remove a specific entry indicated by user data.
 * Only first element matching the userData is removed.
 *
 * @param userdata user specified data used to identify the entry to remove
 */
void delete_timer_by_userdata(void* userdata);

/**
 * Fetch first pending timer and remove from data-structure
 *
 * @return poitner to first pending timer if successful,
 *    NULL if there is not timer
 */
TimerHandle* get_timer();

/**
 * Fetch first pending timer without removing it from data-structure
 *
 * @return poitner to first pending timer if successful,
 *    NULL if there is not timer
 */
TimerHandle* peek_timer();

/**
 * Perform wakeup action accosiated with the timer
 *
 * @param timer instance of the timer to wakeup
 */
void wakeup_timer(TimerHandle *timer);

#endif /*_TIMER_QUEUE_H_*/
