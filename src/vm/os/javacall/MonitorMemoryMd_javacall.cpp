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

/*
 * MonitorMemoryMd_javacall.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_MonitorMemoryMd_javacall.cpp.incl"

#if ENABLE_MEMORY_MONITOR

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * The maximum time in milliseconds between two flushes of the memory monitor
 * send buffer.
 */ 
#define FLUSH_PERIOD 500

HANDLE memmonitorMutex = NULL;

static HANDLE flushThread = NULL;
static int exitThread = 0;


/**
 * This thread ensures that the memory monitor send buffer is flushed (send to 
 * the server site) at least once in FLUSH_PERIOD milliseconds.
 * 
 * @param args not used  
 */  
static unsigned __stdcall
memmonitor_flushThread(void* args) { 
    while (!exitThread) {
        Sleep(FLUSH_PERIOD);
        
        MonitorMemory::flushBuffer();
        
        MonitorMemory::setFlushed(0);
    }

    _endthreadex(0);
    return 0;
}

#ifdef __cplusplus
}
#endif

void MonitorMemoryMd::startup() {
    memmonitorMutex = CreateMutex(NULL, FALSE, NULL);
}

void MonitorMemoryMd::shutdown() {
    CloseHandle(memmonitorMutex);
    memmonitorMutex = NULL;
}

void MonitorMemoryMd::startFlushThread() {
    unsigned threadID;    
    flushThread = (HANDLE)_beginthreadex(NULL, 0, &memmonitor_flushThread, NULL,
            0, &threadID);
}

void MonitorMemoryMd::stopFlushThread() {
    if (flushThread != NULL) {
        exitThread = 1;
        WaitForSingleObject(flushThread, INFINITE);
        CloseHandle(flushThread);
        flushThread = NULL;
        exitThread = 0;
    }
}

void MonitorMemoryMd::lock() {
    WaitForSingleObject(memmonitorMutex, INFINITE);
}

void MonitorMemoryMd::unlock() {
    ReleaseMutex(memmonitorMutex);
}

u_long MonitorMemoryMd::htonl_m(u_long hostlong) {
    return htonl(hostlong);
}

#endif // ENABLE_MEMORY_MONITOR
