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

#include <memMonitor_md.h>

#include <time.h>
#include <memMonitor.h>

/** 
 * The maximum time in nanoseconds between two flushes of the memory monitor
 * send buffer.
 */ 
#define FLUSH_PERIOD 500000000 

pthread_mutex_t memmonitorMutex;

static pthread_t flushThread;
static int threadStarted = 0;
static int exitThread = 0;

static void* memmonitor_flushThread(void* args); 

/**
 * Platform depended initialization of the memory monitor.
 */  
void 
memmonitor_md_startup() {
    memset(&memmonitorMutex, 0, sizeof(memmonitorMutex));
    pthread_mutex_init(&memmonitorMutex, NULL);
}

/**
 * Platform depended uninitialization of the memory monitor.
 */  
void 
memmonitor_md_shutdown() {
    pthread_mutex_destroy(&memmonitorMutex);
}

/**
 * Starts the memory monitor send buffer flushing thread.
 */  
void 
memmonitor_md_startFlushThread() {
    if (pthread_create(&flushThread, NULL, memmonitor_flushThread, NULL) 
            == 0) {
        threadStarted = 1;
    }
}

/**
 * Stops the memory monitor send buffer flushing thread.
 */  
void 
memmonitor_md_stopFlushThread() {
    if (threadStarted) {
        exitThread = 1;
        pthread_join(flushThread, NULL);
        threadStarted = 0;
        exitThread = 0;
    }
}

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

    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = FLUSH_PERIOD;

    while (!exitThread) {
        nanosleep(&sleepTime, &remainingTime);

        if (!memmonitor_flushed) {
            memmonitor_flushBuffer();
        }
        
        memmonitor_flushed = 0;
    }

    pthread_exit(NULL);
    return NULL;
}

