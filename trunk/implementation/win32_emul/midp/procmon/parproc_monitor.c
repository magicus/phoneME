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

#include "parproc_monitor.h"

#include <windows.h>
#include <tlhelp32.h>

#include "process_monitor.h"
#include "javacall_lifecycle.h"
#include "javacall_logging.h"

/** Reference to a started parent process monitor. */
static PROCESS_MONITOR* parentProcessMonitor;

/** 
 * Called when the parent process exits. 
 *
 * @param param reference to additional parameters required by the callback   
 */
static void parentProcessExited(void* param);

/**
 * Gets the parent process id of the specified child process.
 * 
 * @param parentProcessId a pointer to a <code>DWORD</code> variable to which
 *      to store the resulting parent process id 
 * @param childProcessId the id of the child process for which to get the parent
 *      process id 
 * @return <code>TRUE</code> if successfull, <code>FALSE</code> otherwise
 */
static BOOL getParentProcessId(DWORD* parentProcessId, DWORD childProcessId);

/**
 * Starts a parent process monitor. When the monitor detects that the parent
 * process has exited, it destroys the current process as well.
 */  
void 
startParentProcessMonitor() {
    DWORD parentProcessId;

    if (!getParentProcessId(&parentProcessId, GetCurrentProcessId())) {
        javautil_debug_print(JAVACALL_LOG_INFORMATION, "main",
                             "Failed to get parent process id");
        return;
    }

    parentProcessMonitor = startProcessMonitor(parentProcessId,
                                               parentProcessExited, NULL);
    if (parentProcessMonitor == NULL) {
        javautil_debug_print(JAVACALL_LOG_INFORMATION, "main",
                             "Failed to start parent process monitoring");
    }
}

/**
 * Stops the started parent process monitor.
 */  
void 
stopParentProcessMonitor() {
    if (parentProcessMonitor != NULL) {
        stopProcessMonitor(parentProcessMonitor);
    }
}

static BOOL
getParentProcessId(DWORD* parentProcessId, DWORD childProcessId) {
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 
                                                     childProcessId);
    BOOL succeeded = FALSE;
    
    if (snapshotHandle != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32First(snapshotHandle, &processEntry)) {
            do {
                if (processEntry.th32ProcessID == childProcessId) {
                    *parentProcessId = processEntry.th32ParentProcessID;
                    succeeded = TRUE;
                    break;                    
                }
            } while (Process32Next(snapshotHandle, &processEntry));
        }
    
        CloseHandle(snapshotHandle);
    }
    
    return succeeded;
}

static void 
parentProcessExited(void* param) {
    (void)param;

/** 
 * IMPL_NOTE: javanotify_shutdown doesn't work, using ExitProcess instead.
 *            Further investigation required.
 *             
 * javanotify_shutdown();
 */
 
    ExitProcess(1);
}
