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

#ifndef _PROCESS_MONITOR_H_
#define _PROCESS_MONITOR_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Type which represents a process monitor callback function. */
typedef void (*PROCESS_EXIT_CALLBACK)(void* param);

/** Type which represents a process monitor. */
typedef struct _PROCESS_MONITOR {
    HANDLE threadHandle;
    
    HANDLE exitEvent;
    
    HANDLE processHandle;
    
    PROCESS_EXIT_CALLBACK exitCallback;
    
    void* exitCallbackParam;
} PROCESS_MONITOR;

/**
 * Starts a process monitor for the specified process. When the monitor detects
 * that the process has exited, it calls the given callback.  
 * 
 * @param processId the process id
 * @param exitCallback the callback to call when the process has exited
 * @param exitCallbackParam parameter to pass to the callback
 * @return reference to the created process monitor  
 */
PROCESS_MONITOR* startProcessMonitor(DWORD processId, 
                                     PROCESS_EXIT_CALLBACK exitCallback,
                                     void* exitCallbackParam);

/**
 * Stops the specified process monitor.
 * 
 * @param processMonitor reference to the process monitor to stop    
 */
void stopProcessMonitor(PROCESS_MONITOR* processMonitor);

#ifdef __cplusplus
}
#endif

#endif /* _PROCESS_MONITOR_H_ */
