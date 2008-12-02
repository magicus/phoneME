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

#ifndef _MIDP_NATIVE_THREAD_H_
#define _MIDP_NATIVE_THREAD_H_

#include <kni.h>

/**
 * @defgroup core_threads Native-Thread Interaction External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_threads
 *
 * @brief Interface to basic system services
 *
 * <p>Accessible system services to start/sleep native threads.
 * 
 */


#ifdef __cplusplus
extern "C" {
#endif

/* 
 * include platform specific thread types defitinions:
 * data, constants, function prototypes ... 
 */
#include <midpNativeThreadImpl.h> 

#if ENABLE_NATIVE_APP_MANAGER && ENABLE_I3_TEST

/**
 * starts another native thread.
 *
 * The primary usage of this function is testing of NAMS subsystem - 
 * additional thread is used to throw initial midlet start events 
 * to main application thread (where VM runs).
 *
 * @param thread thread routine
 * @param param thread routine parameter
 *
 * @return handle of created thread
 */
extern midp_ThreadId midp_startNativeThread(
    midp_ThreadRoutine* thread, 
    midp_ThreadRoutineParameter param);

#endif

/**
 * suspends current thread for a given number of seconds.
 *
 * Used to place java stack to suspended state in default suspend/resume
 * implementation.
 *
 * Another usage of this function is testing of NAMS subsystem -
 * additional thread needs to wait for some time until java subsystem will 
 * initialize itself correctly.
 *
 * @param duration  how many seconds to sleep
 */
extern void midp_sleepNativeThread(int duration);

/**
 * Returns the platform-specific handle of the current thread.
 *
 * @return handle of the current created thread
 */
extern midp_ThreadId midp_getCurrentThreadId();

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _MIDP_NATIVE_THREAD_H_ */
