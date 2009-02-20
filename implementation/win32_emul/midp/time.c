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
#include <windows.h>
#include <mmsystem.h>
#include <time.h>

#ifdef WIN32
#include <sys/timeb.h>
#endif

#include "javacall_time.h"

#define MAX_SUSPENDED_TIMERS 5

static javacall_handle suspended_timers[MAX_SUSPENDED_TIMERS];

void CALLBACK win32_timer_callback(UINT uTimerID, UINT uMsg,
                                   DWORD dwUser, DWORD dw1, DWORD dw2){
    int i;
    javacall_callback_func func = (javacall_callback_func)dwUser;

    /* Check, if the timer has been suspended */
    for (i = 0; i < MAX_SUSPENDED_TIMERS; i++) {
        if (uTimerID == (UINT)suspended_timers[i]) {
            return;
        }
    }

    func((javacall_handle *)uTimerID);
}

/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 * If a later timer exists, cancel it and create a new timer
 *
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if -1, then ignore the call
 * @param cyclic <tt>1</tt> indicates that the timer should be repeated cuclically,
 *               <tt>0</tt> indicates that this is a one-shot timer that should call the callback function once
 * @param func callback function should be called in platform's context once the timer
 *			   expires
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
                    /*OUT*/ javacall_handle	*handle){

    MMRESULT hTimer;

    if (!handle || !func) {
        return JAVACALL_INVALID_ARGUMENT;
    }

    hTimer = timeSetEvent(wakeupInMilliSecondsFromNow,
            10, /* 10ms: tuned resolution from CLDC_HI porting experiences */
            win32_timer_callback,
            (DWORD)func,
            (JAVACALL_TRUE == cyclic ? TIME_PERIODIC : TIME_ONESHOT));

    if (0 == hTimer) {
        *handle = NULL;
        return JAVACALL_FAIL;
    } else {
        *handle = (javacall_handle)hTimer;
        return JAVACALL_OK;
    }
}

/*
 *
 * Temporarily disable timer interrupts. This is called when
 * the VM is about to sleep (when there's no Java thread to execute)
 *
 * @param handle timer handle to suspend
 *
 */
void javacall_time_suspend_ticks(javacall_handle handle){
    int i;
    for (i = 0; i < MAX_SUSPENDED_TIMERS; i++) {
        if (suspended_timers[i] == handle) {
            return; /* the timer has already been suspended */
        } else if (suspended_timers[i] == NULL) {
            suspended_timers[i] = handle;
            return;
        }
    }
}

/*
 *
 * Enable  timer interrupts. This is called when the VM
 * wakes up and continues executing Java threads.
 *
 * @param handle timer handle to resume
 *
 */
void javacall_time_resume_ticks(javacall_handle handle){
    int i;
    for (i = 0; i < MAX_SUSPENDED_TIMERS; i++) {
        if (suspended_timers[i] == handle) {
            suspended_timers[i] = NULL;
            return;
        }
    }
}

/*
 * Suspend the current process sleep for ms milliseconds
 */
void javacall_time_sleep(javacall_uint64 ms){
    Sleep((DWORD)ms);
}

/**
 *
 * Disable a set native timer
 * @param handle The handle of the timer to be finalized
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_finalize_timer(javacall_handle handle) {

    if (NULL == handle) {
        return JAVACALL_INVALID_ARGUMENT;
    }

    /* Remove timer from the suspended timers list (if exists) */
    javacall_time_resume_ticks(handle);

    if (TIMERR_NOERROR == timeKillEvent((UINT)handle)) {
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 * At least one native timer can be used concurrently.
 * If a later timer exists, cancel it and create a new timer
 *
 * @param type type of alarm to set, either JAVACALL_TIMER_PUSH, JAVACALL_TIMER_EVENT
 *                              or JAVACALL_TIMER_WATCHDOG
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if JAVACALL_FAIL, then ignore the call
 *
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> on failure
 */
//javacall_result	javacall_time_create_timer(javacall_timer_type type, int wakeupInMilliSecondsFromNow){
//    return JAVACALL_FAIL;
//}

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
char* javacall_time_get_local_timezone(void){

   static char tzstring[128];
   int diff_in_hours,diff_in_minutes;
   struct timeb tstruct;
   tzset();
   ftime( &tstruct );

   diff_in_hours=-(int)(tstruct.timezone/60);
   diff_in_minutes=tstruct.timezone%60;
   if (diff_in_minutes<0) {diff_in_minutes=-diff_in_minutes;}

   if (tstruct.dstflag) { diff_in_hours++; }
   if (diff_in_hours<-12) {diff_in_hours+=24;}

   if (diff_in_hours<0) {
       sprintf(tzstring, "GMT%d:%02d",diff_in_hours,diff_in_minutes);
   }else{
       sprintf(tzstring, "GMT+%d:%02d",diff_in_hours,diff_in_minutes);
   }
   return tzstring;

}


/**
 * returns number of milliseconds elapsed since midnight(00:00:00), January 1, 1970,
 *
 * @return milliseconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_milliseconds_since_1970(void){

    static javacall_int64 delta=0;

    if (delta==0) {
        delta=javacall_time_get_seconds_since_1970();
        delta=delta*1000-javacall_time_get_clock_milliseconds();
    }

    return delta+javacall_time_get_clock_milliseconds();
}

//javacall_int64 /*OPTIONAL*/ javacall_time_get_milliseconds_since_1970(void){
// This _time64 function is available under VisualC++.net
//    return _time64(NULL);
//}



/**
 * returns the number of seconds elapsed since midnight (00:00:00), January 1, 1970,
 *
 * @return seconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_time_seconds /*OPTIONAL*/ javacall_time_get_seconds_since_1970(void){

    return (javacall_time_seconds)time(NULL);
}




/**
 * returns the milliseconds elapsed time counter
 *
 * @return elapsed time in milliseconds
 */
javacall_time_milliseconds /*OPTIONAL*/ javacall_time_get_clock_milliseconds(void){

    return (javacall_time_milliseconds) (((javacall_int64)clock())*1000/CLOCKS_PER_SEC);
}

/**
 * Returns the value of the monotonic clock counter.
 * This counter must be monotonic and must have resolution and read 
 * time not lower than that of javacall_time_get_clock_milliseconds().
 *
 * @return the value of the monotonic clock counter
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_monotonic_clock_counter(void) {
  LARGE_INTEGER count;
  /* 
   * NOTE: we assume performance counter is available and do not check
   * the return value
   */
  QueryPerformanceCounter(&count);
  return (javacall_int64)count.QuadPart;
}

/**
 * Returns the frequency of the monotonic clock counter in the number 
 * of periods per second.
 *
 * @return the frequency of the monotonic clock counter
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_monotonic_clock_frequency(void) {
  static javacall_int64 frequency = -1;
  javacall_int64 f = frequency;
  if (f < 0) {
    LARGE_INTEGER freq;
    /*
     * NOTE: we assume performance counter is available and do not check
     * the return value
     */
    QueryPerformanceFrequency(&freq);
    frequency = f = freq.QuadPart;
  }
  return f;
}
