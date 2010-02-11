/*
 * @(#)time.h	1.10 06/10/10
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
 *
 */

/*
 * Porting layer for time functions.
 */

#ifndef _INCLUDED_PORTING_TIME_H
#define _INCLUDED_PORTING_TIME_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/threads.h"

/*
 * Return time in milliseconds, as defined by System.currentTimeMillis()
 */
CVMInt64 CVMtimeMillis(void);

#ifdef CVM_JVMTI
/*
 * The following functions are used for high-resolution timer support
 * in JVMTI.  These functions are called by functions in jvmtiEnv.c
 */
/*
 * Initialize the high-resolution clocks.  Called during VM startup.
 */
void CVMtimeClockInit(void);
/*
 * Initialize the per-thread high-resolution timer.  Called during the
 * thread creation process in the VM.
 * args:
 *  threadID - CVM thread ID (i.e. not the underlying OS thread id)
 */
void CVMtimeThreadCpuClockInit(CVMThreadID *threadID);
/*
 * Returns the current value of the most precise available system
 * timer, in nanoseconds.
 * This timer only returns elapsed time and not time-of-day time.
 * This method provides nanosecond precision, but not
 * necessarily nanosecond accuracy.
 */
CVMInt64 CVMtimeNanosecs(void);
/*
 * Thread CPU Time - return the fast estimate on a platform
 * For example:
 * On Solaris - call gethrvtime (fast) - user time only
 * On Linux   - fast clock_gettime where available - user+sys
 *            - otherwise: very slow /proc fs - user+sys
 * On Windows - GetThreadTimes - user+sys
 */
CVMInt64 CVMtimeThreadCpuTime(CVMThreadID *threadID);
/*
 * CVMtimeCurrentThreadCputime() is included in the HPI because 
 * the OS may provide a more efficient implementation than going
 * through CVMtimeThreadCputime().
 * If such an efficient implementation does not exists, the port
 * can choose to implement it as a call to CVMtimeThreadCputime()
 * instead.
 * The caller from shared code will always pass the CVMThreadID of
 * the current thread.  This is for the convenient of the port.
 */
CVMInt64 CVMtimeCurrentThreadCpuTime(CVMThreadID *threadID);
/*
 * Returns true if this OS supports per-thread cpu timers
 */
CVMBool CVMtimeIsThreadCpuTimeSupported();
#endif

#include CVM_HDR_TIME_H

#endif /* _INCLUDED_PORTING_TIME_H */
