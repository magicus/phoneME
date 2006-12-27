/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _SUSPEND_RESUME_H_
#define _SUSPEND_RESUME_H_

#include <midp_global_status.h>
#include <kni.h>

/**
 * @file suspend_resume.h
 *
 * Java stack Suspend/Resume interface
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interval for checking for resume request, in milliseconds in case
 * java stack is suspended.
 */
#define SR_RESUME_CHECK_TIMEOUT 100

/**
 * Common type for routines used to suspend/resume a resource.
 */
typedef MIDPError (*SuspendResumeProc) (void *rs);

/**
 * Registers a native resource in the Suspend/Resume system.
 * Registered resources are suspended by suspendProc and resumed resumeProc
 * specified. If there is nothing to do with the resource oneither suspend or
 * resume operation use SR_EMPTY_PROC as corresponding parameter.
 * @param resource ponter to the resource, should identify the resource.
 * @param suspendProc pointer to the function that should be applied to
 *        the resource during system suspend.
 * @param resumeProc pointer to the function that should be applied to
 *        the resource during system resume.
 */
extern void sr_registerResource(
    void *resource,
    SuspendResumeProc suspendProc,
    SuspendResumeProc resumeProc);

/**
 * Unregister given resource from the suspend/resume system.
 * @param resource pointer to the resource, should identify the resource. The
 *        resources are considered to be the same if pointers are equal.
 */
extern void sr_unregisterResource(void *resource);

/**
 * Initializes suspend/resume system. Should be called from MIDP initializing
 * routines.
 */
extern void sr_initSystem();

/**
 * Destroyes suspend/resume system.
 */
extern void sr_finalizeSystem();

/**
 * Suspend/resume routine for resources that should not be processed during
 * either suspend or resume.
 */
extern SuspendResumeProc SR_EMPTY_PROC;

/**
 * Requests java stack to release resources and suspend.
 * To resume the stack it is required to pas control to it and provide
 * conditions for midp_checkResumeRequest() to return KNI_TRUE.
 */
extern void midp_suspend();

/**
 * Checks if there is a request for java stack to resume normal operation.
 * This function is called from midp_checkAndResume() and requires porting.
 *
 * @return KNI_TRUE if java stack is requested to resume, KNI_FALSE if it is
 * not.
 */
extern jboolean midp_checkResumeRequest();

/**
 * Checks if there is active request for java stack to resume and invokes
 * stack resuming routines if requested. Makes nothing in case the java
 * stack is not currently suspended. There is no need to call this function
 * from the outside of java stack, it is called automatically if suspended
 * stack receives control.
 *
 * @return KNI_TRUE if java stack resuming procedures were invoked.
 */
extern jboolean midp_checkAndResume();

/**
 * Waits while java stack stays suspended then calls midp_resume().
 * If java stack is not currently suspened, returns immediately.
 * Otherwise calls midp_checkAndResume() in cycle until it detects
 * resume request and performs resuming routines.
 *
 * Used in VM maser mode only.
 */
extern void midp_waitWhileSuspended();

/**
 * Possible states of suspendable resources and java stack.
 *
 * A suspendable resource is initially in SR_ACTIVE state and then is switched
 * to SR_SUPENDED and back by supend and resume routines. If either suspend
 * or resume routine fails for a resource it is switched to SR_INVALID state
 * and then removed from the system.
 *
 * For java stack, SR_INVALID means that suspend/resume system is not
 * initialized.
 */
typedef enum _SRState {
    SR_SUSPENDED,
    SR_ACTIVE,
    SR_INVALID
} SRState;

/**
 * Current state of java stack from suspend/resume point of view.
 * @return SR_INVALID if suspend/resume system is not initialized,
 *         SR_ACTIVE if java stack is active,
 *         SR_SUSPENDED if java stack is suspended.
 */
extern SRState midp_getSRState();

#ifdef __cplusplus
}
#endif

#endif /* _SUSPEND_RESUME_H_ */
