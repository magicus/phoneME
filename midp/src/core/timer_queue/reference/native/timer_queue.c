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
 * Implementation of the timer queue inteface.
 * Can be used to support any timer alarms in the master mode.
 */

#include <kni.h>
#include <stdlib.h>
#include <midpMalloc.h>
#include <timer_queue.h>

/**
 *  TimerHandle
 *
 *  Implementation of data structure to keep an ordered list of
 *  upcoming timers. Operations on data structure:
 *    add    : insert new timer into ordered timers list
 *    get    : fetch first pending timer
 *    peek   : fetch first pending timer but do not remove from data-structure
 *    new    : create a new entry and enqueue in data-structure
 *    delete : remove an entry from data-structure and free its memory
 *    remove : remove a specific entry indicated by user data
 **/
struct _TimerHandle {
    jlong timeToWakeup;             /* Absolute time to wakeup */
    void* userData;                 /* User data provided with timer */
    fTimerCallback userCallback;    /* User action on alarm */
    struct _TimerHandle* next;      /* Next timer handle in the queue */
};

/** Timers queue */
static TimerHandle* rootTimer = NULL;

/**
 * Insert new timer to the correct place in timer queue
 * ordered by wakeup time
 *
 * @param newTimer new timer to add to queue
 */
void add_timer(TimerHandle* newTimer) {
    TimerHandle **ptr = &rootTimer;
    REPORT_INFO1(LC_PUSH, "[add_timer] newTimer=%p", newTimer);

    for(; *ptr != NULL; ptr = &((*ptr)->next)) {
        if ((*ptr)->timeToWakeup > newTimer->timeToWakeup) {
           break;
        }
    }
    newTimer->next = *ptr;
    (*ptr) = newTimer;
}

/**
 * Create a new entry and enqueue in data-structure
 *
 * @param timeToWakeup the abs time for the timer (used to order the timer queue)
 * @param userData user specified data
 * @param userCallback user specified callback
 *
 * @return poitner to new element if successful,
 * NULL if there was an error allocating this timer.
 */
TimerHandle* new_timer(
    jlong timeToWakeup, void* userData, fTimerCallback userCallback) {

    TimerHandle* newTimer = (TimerHandle*)midpMalloc(sizeof(TimerHandle));
    if (newTimer != NULL) {
        REPORT_INFO3(LC_PUSH, "[new_timer] timeToWakeup=%#lx userData=%p userCallback=%p",
            (long)timeToWakeup, userData, (void *)userCallback);

        newTimer->next = NULL;
 		newTimer->timeToWakeup = timeToWakeup;
		newTimer->userData = userData;
        newTimer->userCallback = userCallback;
        add_timer(newTimer);
    }
    return newTimer;
}

/** Search for handle instance in the sockets list and return its reference */
static TimerHandle** get_timer_ptr(TimerHandle* timer) {
    TimerHandle** ptr;
    if (timer != NULL) {
        for (ptr = &rootTimer; *ptr != NULL; ptr = &((*ptr)->next)) {
            if (*ptr == timer) return ptr;
        }
    }
    return NULL;
}

/**
 * Remove specified timer handler from queue and free allocated memory
 *
 * @param timer instance of timer that should be remove
 */
void delete_timer(TimerHandle* timer) {
    TimerHandle **ptr = get_timer_ptr(timer);
    REPORT_INFO1(LC_PUSH, "[delete_timer] timer=%p", timer);

    if (ptr != NULL) {
        *ptr = timer->next;
        midpFree(timer);
    }
}


/**
 * Remove specified timer handler from queue but don't free allocated memory
 *
 * @param timer instance that should be remove
 *
 * @return poitner to timer instance that should be remove from queue
 */
TimerHandle* remove_timer(TimerHandle* timer) {
    TimerHandle **ptr = get_timer_ptr(timer);
    REPORT_INFO1(LC_PUSH, "[remove_timer] timer=%p", timer);

    if (ptr != NULL) {
        *ptr = timer->next;
    }

    return timer;
}

/**
 * Remove a specific entry indicated by user data.
 * Only first element matching the userData is removed.
 *
 * @param userdata user specified data used to identify the entry to remove
 */
void delete_timer_by_userdata(void* userdata) {
    TimerHandle* timer;
    REPORT_INFO1(LC_PUSH, "[delete_timer_by_userdata] userdata=%p", userdata);

    for(timer = rootTimer; timer != NULL; timer = timer->next) {
        if (timer->userData == userdata) {
            delete_timer(timer);
            return;
        }
    }
}

/**
 * Fetch first pending timer and remove from data-structure
 *
 * @return poitner to first pending timer if successful,
 * NULL if there is not timer
 */
TimerHandle* get_timer() {
    TimerHandle* timer;
    if (rootTimer != NULL) {
        timer = rootTimer;
        rootTimer = rootTimer->next;
        timer->next = NULL;
        return timer;
    }
    return NULL;
}

/**
 * Fetch first pending timer without removing it from data-structure
 *
 * @return poitner to first pending timer if successful,
 * NULL if there is not timer
 */
TimerHandle* peek_timer() {
    return rootTimer;
}


/**
 * Get user data of the timer
 *
 * @param timer instance of the timer to get user data from
 * @return untyped user data registered during timer creation,
 *    or NULL for NULL timer handle
 */
void* get_timer_data(const TimerHandle *timer) {
    if (timer != NULL) {
        return timer->userData;
    }
    return NULL;
}

/**
 * Get absoulte wakeup time of the timer
 *
 * @param timer instance of the timer to get wakeup time from
 * @return absoulte time when the timer should wakeup,
 *    or -1 for NULL timer handle
 */
jlong get_timer_wakeup(const TimerHandle *timer) {
    if (timer != NULL) {
        return timer->timeToWakeup;
    }
    return -1;
}

/**
 * Set absoulte wakeup time for the timer
 *
 * @param timer instance of the timer to set wakeup time to
 * @param timeToWakeup absolute time when the timer should wakeup
 */
void set_timer_wakeup(TimerHandle *timer, jlong timeToWakeup) {
    if (timer != NULL) {
        timer->timeToWakeup = timeToWakeup;
    }
}

/**
 * Perform wakeup action accosiated with the timer
 *
 * @param timer instance of the timer to wakeup
 */
void wakeup_timer(TimerHandle *timer) {
    if (timer != NULL && timer->userCallback != NULL) {
        REPORT_INFO1(LC_PUSH, "[wakeup_timer] timer=%p", timer);
        timer->userCallback(timer);    
    }
}
