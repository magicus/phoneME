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
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "portlibs/posix/threads.h"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include "javavm/include/interpreter.h"
#include "javavm/include/assert.h"

#ifdef CVM_JVMTI
#define NANOS_PER_SEC         1000000000L
#define NANOS_PER_MILLISEC    1000000
 
void CVMtimeClockInit(void) {
}

CVMBool
CVMtimeIsThreadCpuTimeSupported(void) {
    return CVM_FALSE;
}

CVMInt64
CVMtimeThreadCpuTime(CVMThreadID *thread) {
    return -1;
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
    return CVMtimeMillis() * NANOS_PER_MILLISEC; /* the best we can do. */
}
#endif

CVMInt64
CVMtimeMillis(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return (CVMInt64)(((CVMInt64)t.tv_sec) * 1000 + (CVMInt64)(t.tv_usec/1000));
}
