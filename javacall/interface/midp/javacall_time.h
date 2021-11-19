/*
 *
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
#ifndef __JAVACALL_TIME_H_
#define __JAVACALL_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_time.h
 * @ingroup Time
 * @brief Javacall interfaces for time
 */

#include "javacall_defs.h"

/**
 * @defgroup Time Time API
 * @ingroup JTWI
 * @{
 */

/**
 * @defgroup MandatoryTime Mandatory Time API
 * @ingroup Time
 *
 * Time APIs define the functionality for:
 *   - Initialize a timer
 *   - Cancel a timer
 *   - Get current time
 *   - Get timezone
 *
 * @{
 */

/**
* @typedef javacall_callback_func
*/
typedef void (*javacall_callback_func)(javacall_handle handle);

/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 * For non-cyclic timers, the platform must finalize timer resources after invoking the callback.
 *
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if -1, then ignore the call
 * @param cyclic <tt>JAVACALL_TRUE</tt> indicates that the timer should be repeated cyclically,
 *               <tt>JAVACALL_FALSE</tt> indicates that this is a one-shot timer that should call the callback function once
 * @param func callback function should be called in platform's context once the timer
 *             expires
 * @param handle A pointer to the returned handle that will be associated with this timer
 *               On success.
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         or <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_initialize_timer(
                    int                      wakeupInMilliSecondsFromNow,
                    javacall_bool            cyclic,
                    javacall_callback_func   func,
                    /*OUT*/ javacall_handle  *handle
                    );

/**
 *
 * Disable a set native timer and free resources used by the timer.
 * @param handle The handle of the timer to be finalized
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_finalize_timer(javacall_handle handle);

/*
 *
 * Temporarily disable timer interrupts. This is called when
 * the VM is about to sleep (when there's no Java thread to execute)
 *
 * @param handle timer handle to suspend
 *
 */
void javacall_time_suspend_ticks(javacall_handle handle);

/*
 *
 * Enable  timer interrupts. This is called when the VM
 * wakes up and continues executing Java threads.
 *
 * @param handle timer handle to resume
 *
 */
void javacall_time_resume_ticks(javacall_handle handle);

/*
 * Suspend the current process sleep for ms milliseconds
 */
void javacall_time_sleep(javacall_uint64 ms);

/**
 * Return local timezone ID string. This string is maintained by this
 * function internally. Caller must NOT try to free it.
 *
 * This function should handle daylight saving time properly. For example,
 * for time zone America/Los_Angeles, during summer time, this function
 * should return GMT-07:00 and GMT-08:00 during winter time.
 *
 * @return Local timezone ID string pointer. The ID string should be in the
 *         format of GMT+/-??:??. For example, GMT-08:00 for PST.
 */
char* javacall_time_get_local_timezone(void);




/**
 * returns number of fine resolution milliseconds elapsed since
 * midnight(00:00:00), January 1, 1970.
 * Implementation hint: use javacall_time_get_seconds_since_1970 and
 * javacall_time_get_clock_milliseconds to get an accurate result
 * (see javacall_impl\\win32\\src\\time.c).
 *
 * @return milliseconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_int64 javacall_time_get_milliseconds_since_1970(void);

/** @} */


/**
 * @defgroup OptionalTime Optional Time API
 * @ingroup Time
 *
 * @{
 */

/**
 * @typedef javacall_time_seconds
 * @brief Seconds
 */
typedef unsigned long javacall_time_seconds;

/**
 * returns the number of seconds elapsed since midnight (00:00:00), January 1, 1970,
 *
 * @return seconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_time_seconds /*OPTIONAL*/ javacall_time_get_seconds_since_1970(void);

/**
 * @typedef javacall_time_milliseconds
 * @brief Milliseconds
 */
typedef unsigned long javacall_time_milliseconds;

/**
 * returns the milliseconds elapsed time counter
 *
 * @return elapsed time in milliseconds
 */
javacall_time_milliseconds /*OPTIONAL*/ javacall_time_get_clock_milliseconds(void);

/**
 * Returns the value of the monotonic clock counter.
 * This counter must be monotonic and must have resolution and read 
 * time not lower than that of javacall_time_get_clock_milliseconds().
 * <p>
 * The CLDC VM build flag SUPPORTS_MONOTONIC_CLOCK should be set to
 * a non-zero value if the platform provides the monotonic clock,
 * otherwise it should be set to zero.
 *
 * @return the value of the monotonic clock counter
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_monotonic_clock_counter(void);

/**
 * Returns the frequency of the monotonic clock counter in the number 
 * of periods per second.
 * <p>
 * The CLDC VM build flag SUPPORTS_MONOTONIC_CLOCK should be set to
 * a non-zero value if the platform provides the monotonic clock,
 * otherwise it should be set to zero.
 *
 * @return the frequency of the monotonic clock counter
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_monotonic_clock_frequency(void);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif


