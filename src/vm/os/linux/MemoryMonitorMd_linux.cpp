/*
 *   
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

/*
 * MemoryMonitorMd_linux.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_MemoryMonitorMd_linux.cpp.incl"

#if ENABLE_MEMORY_MONITOR

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * The maximum time in nanoseconds between two flushes of the memory monitor
 * send buffer.
 */ 
#define FLUSH_PERIOD 500000000 

static pthread_mutex_t memmonitorMutex;

static pthread_t flushThread;
static int threadStarted = 0;
static int exitThread = 0;

/**
 * This thread ensures that the memory monitor send buffer is flushed (send to 
 * the server site) at least once in FLUSH_PERIOD milliseconds.
 * 
 * @param args not used  
 */  
static void*
memmonitor_flushThread(void* args) { 
    struct timespec sleepTime;
    struct timespec remainingTime;

	(void)args;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = FLUSH_PERIOD;

    while (!exitThread) {
        nanosleep(&sleepTime, &remainingTime);

        MemoryMonitor::flushBuffer();
        
        MemoryMonitor::setFlushed(0);
    }

    pthread_exit(NULL);
    return NULL;
}

#ifdef __cplusplus
}
#endif

void MemoryMonitorMd::startup() {
    memset(&memmonitorMutex, 0, sizeof(memmonitorMutex));
    pthread_mutex_init(&memmonitorMutex, NULL);
}

void MemoryMonitorMd::shutdown() {
    pthread_mutex_destroy(&memmonitorMutex);
}

void MemoryMonitorMd::startFlushThread() {
    if (pthread_create(&flushThread, NULL, memmonitor_flushThread, NULL) 
            == 0) {
        threadStarted = 1;
    }
}

void MemoryMonitorMd::stopFlushThread() {
    if (threadStarted) {
        exitThread = 1;
        pthread_join(flushThread, NULL);
        threadStarted = 0;
        exitThread = 0;
    }
}

void MemoryMonitorMd::lock() {
    pthread_mutex_lock(&memmonitorMutex);
}

void MemoryMonitorMd::unlock() {
    pthread_mutex_unlock(&memmonitorMutex);
}

u_long MemoryMonitorMd::htonl_m(u_long hostlong) {
    return htonl(hostlong);
}

#endif // ENABLE_MEMORY_MONITOR
