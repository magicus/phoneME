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
#include <stdio.h>

#include <kni.h>
#include <jvm.h>
#include <sni.h>

#include <midp_logging.h>
#include <midp_constants_data.h>
#include <midpNativeThread.h>

/**
 * @file
 *
 * Platform specific system services for work with native threads.
 */

#if ENABLE_NATIVE_APP_MANAGER && ENABLE_I3_TEST
/**
 * starts another native thread.
 * The primary usage of this function is testing of NAMS subsystem 
 * - second thread is used to throw initial events to main application thread.
 *
 * ATTENTION: this is a stub ! 
 * Put real plarform specific thread start here if needed !
 * If platfoprm specific thread routine signature differs from 
 * "midp_ThreadRoutine", use platform speficic thread routine as a wrapper 
 * to prepare parameters for "midp_ThreadRoutine" and to call it ...
 *
 * @param thread thread routine
 * @param param thread routine parameter
 *
 * @return handle of created thread
 */
midp_ThreadId midp_startNativeThread(midp_ThreadRoutine thread, 
    midp_ThreadRoutineParameter param) {

    (void)thread;
    (void)param;
    REPORT_WARN(LC_AMS, "midp_startNativeThread: Stubbed out."); 
    return MIDP_INVALID_NATIVE_THREAD_ID;
}

#endif

/**
 * suspends current thread for a given number of seconds.
 * Used to place java stack to suspended state in default suspend/resume
 * implementation.
 * Another usage of this function is testing of NAMS subsystem -
 * additional thread needs to wait for s\ome time until java subsystem will 
 * initialize inself correctly.
 *
 * ATTENTION: this is a stub ! 
 *
 * @param duration  how many seconds to sleep
 */
void midp_sleepNativeThread(int duration) {
    (void)duration;
    REPORT_WARN(LC_AMS, "midp_sleepNativeThread: Stubbed out."); 
}

/**
 * Returns the platform-specific handle of the current thread.
 *
 * @return handle of the current created thread
 */
midp_ThreadId midp_getCurrentThreadId() {
  return MIDP_INVALID_NATIVE_THREAD_ID;
}

