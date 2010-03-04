/*
 * @(#)time_md.c	1.8 06/10/10
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

#include "javavm/include/porting/time.h"
#include "javavm/include/interpreter.h"

#ifdef WINCE
#include <winbase.h>
#else
#include <windows.h>
#endif

#define FT2INT64(ft) \
        ((CVMInt64)(ft).dwHighDateTime << 32 | (CVMInt64)(ft).dwLowDateTime)

#ifdef CVM_JVMTI

#define NANOS_PER_SEC         1000000000L
#define NANOS_PER_MILLISEC    1000000

static int    hasPerformanceCount = 0;
static int    hasSuperClock = 0;
static CVMInt64 initialPerformanceCount;
static CVMInt64 performanceFrequency;
static CVMInt64 multiplier;
 
void CVMtimeClockInit(void) {
    LARGE_INTEGER count;
    if (QueryPerformanceFrequency(&count)) {
 	hasPerformanceCount = 1;
 	performanceFrequency = count.QuadPart;
 	if (performanceFrequency > NANOS_PER_SEC) {
 	    /* super high speed clock > 1GHz */
 	    hasSuperClock = 1;
 	    multiplier = performanceFrequency / NANOS_PER_SEC;
 	} else {
 	    multiplier = NANOS_PER_SEC / performanceFrequency;
 	}
 	QueryPerformanceCounter(&count);
 	initialPerformanceCount = count.QuadPart;
    }
}

CVMBool
CVMtimeIsThreadCpuTimeSupported(void) {
    /* see CVMthreadCputime */
    FILETIME CreationTime;
    FILETIME ExitTime;
    FILETIME KernelTime;
    FILETIME UserTime;

    if (GetThreadTimes(GetCurrentThread(), &CreationTime,
		       &ExitTime, &KernelTime, &UserTime) == 0) {
	return CVM_FALSE;
    } else {
	return CVM_TRUE;
    }
}

CVMInt64
CVMtimeThreadCpuTime(CVMThreadID *thread) {
    /* This code is copy from clasic VM -> hpi::sysThreadCPUTime
     * If this function changes, os::is_thread_cpu_time_supported() should too
     */
    FILETIME CreationTime;
    FILETIME ExitTime;
    FILETIME KernelTime;
    FILETIME UserTime;

    if ( GetThreadTimes(thread->handle,	&CreationTime,
			&ExitTime, &KernelTime, &UserTime) == 0)
	return -1;
    else
	return (FT2INT64(UserTime) + FT2INT64(KernelTime)) * 100;
}

void CVMtimeThreadCpuClockInit(CVMThreadID *threadID) {
    (void)threadID;
}

CVMInt64
CVMtimeCurrentThreadCpuTime(CVMThreadID *threadID) {
    return CVMtimeThreadCpuTime(threadID);
}

CVMInt64
CVMtimeNanosecs(void)
{
    if (!hasPerformanceCount) { 
 	return CVMtimeMillis() * NANOS_PER_MILLISEC; /* the best we can do. */
    } else {
 	LARGE_INTEGER current_count;  
 	CVMInt64 current;
 	CVMInt64 mult;
 	CVMInt64 time;
 	QueryPerformanceCounter(&current_count);
 	current = current_count.QuadPart;
 	mult = multiplier;
 	if (!hasSuperClock) {
 	    time = current * multiplier;
 	} else {
 	    time = current / multiplier;
 	}
 	return time;
    }
}
#endif

CVMInt64
CVMtimeMillis(void)
{
#ifndef WINCE
    static CVMInt64 fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;

    if (fileTime_1_1_70 == 0) {
        /* Initialize fileTime_1_1_70 -- the Win32 file time of midnight
         * 1/1/70.
         */

        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);
    }

    GetSystemTime(&st0);
    SystemTimeToFileTime(&st0, &ft0);

    return (FT2INT64(ft0) - fileTime_1_1_70) / 10000;
#else 
    CVMInt64 fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;
    static CVMInt64 originTick = 0;
    CVMInt64 ttt;

    if (originTick == 0) {
        /* Initialize fileTime_1_1_70 -- the Win32 file time of midnight
         * 1/1/70.
         */

        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);

        GetSystemTime(&st0);
        SystemTimeToFileTime(&st0, &ft0);
        originTick = (FT2INT64(ft0) - fileTime_1_1_70) / 10000;
        originTick -= GetTickCount(); 
    }

    ttt = GetTickCount() + originTick;
    return ttt;
#endif
}

#ifdef WINCE
#include "javavm/include/ansi/time.h"

clock_t
clock(void)
{
    return 0;
}
#endif
