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

#include "process_monitor.h"

#include <process.h>

/**
 * Constructs a process monitor.
 * 
 * @return reference to the constructed process monitor or <code>NULL</code>
 *      to indicate failure
 */ 
static PROCESS_MONITOR* constructProcessMonitor();
/**
 * Destroys a process monitor.
 * 
 * @param processMonitor reference to the process monitor to destroy
 */   
static void destroyProcessMonitor(PROCESS_MONITOR* processMonitor);
/**
 * Function which monitors a process on a separate thread.
 * 
 * @param processMonitor reference to a process monitor associated with the
 *      thread 
 */   
static unsigned __stdcall monitorThread(const PROCESS_MONITOR* processMonitor);

/**
 * Starts a process monitor for the specified process. When the monitor detects
 * that the process has exited, it calls the given callback.  
 * 
 * @param processId the process id
 * @param exitCallback the callback to call when the process has exited
 * @param exitCallbackParam parameter to pass to the callback
 * @return reference to the created process monitor  
 */
PROCESS_MONITOR* 
startProcessMonitor(DWORD processId, PROCESS_EXIT_CALLBACK exitCallback,
                    void* exitCallbackParam) {
    PROCESS_MONITOR* processMonitor = constructProcessMonitor();    
    if (processMonitor == NULL) {
        return NULL;
    }
    
    processMonitor->exitCallback = exitCallback;
    processMonitor->exitCallbackParam = exitCallbackParam;
    
    processMonitor->processHandle = 
            OpenProcess(SYNCHRONIZE, FALSE, processId);
    if (processMonitor->processHandle == INVALID_HANDLE_VALUE) {
        destroyProcessMonitor(processMonitor);
        return NULL;
    }
    
    processMonitor->exitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (processMonitor->exitEvent == NULL) {
        destroyProcessMonitor(processMonitor);
        return NULL;
    }
        
    processMonitor->threadHandle = 
            (HANDLE)_beginthreadex(NULL, 0, 
                                   (unsigned (__stdcall *)(void*))monitorThread, 
                                   processMonitor, 0, NULL);
    if (processMonitor->threadHandle == NULL) {
        destroyProcessMonitor(processMonitor);
        return NULL;
    }
    
    return processMonitor;
}

/**
 * Stops the specified process monitor.
 * 
 * @param processMonitor reference to the process monitor to stop    
 */
void 
stopProcessMonitor(PROCESS_MONITOR* processMonitor) {
    SetEvent(processMonitor->exitEvent);
        
    WaitForSingleObject(processMonitor->threadHandle, INFINITE);
    destroyProcessMonitor(processMonitor);
}

static PROCESS_MONITOR*
constructProcessMonitor() {
    PROCESS_MONITOR* processMonitor = 
            (PROCESS_MONITOR*)malloc(sizeof(PROCESS_MONITOR));
            
    memset(processMonitor, 0, sizeof(PROCESS_MONITOR));
    processMonitor->processHandle = INVALID_HANDLE_VALUE;

    return processMonitor;
}

static void
destroyProcessMonitor(PROCESS_MONITOR* processMonitor) {
    if (processMonitor->threadHandle != NULL) {
        CloseHandle(processMonitor->threadHandle);
    }
    
    if (processMonitor->exitEvent != NULL) {
        CloseHandle(processMonitor->exitEvent);
    }
    
    if (processMonitor->processHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(processMonitor->processHandle);
    }
        
    free(processMonitor);
}

static unsigned __stdcall
monitorThread(const PROCESS_MONITOR* processMonitor) {
    HANDLE waitHandles[2];

    waitHandles[0] = processMonitor->exitEvent;
    waitHandles[1] = processMonitor->processHandle;

    if (WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE) 
            == (WAIT_OBJECT_0 + 1)) {
        processMonitor->exitCallback(processMonitor->exitCallbackParam);
    }

    _endthreadex(0);
    return 0;
}
