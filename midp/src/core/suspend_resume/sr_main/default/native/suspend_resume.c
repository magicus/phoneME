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

#include <suspend_resume.h>
#include <suspend_resume_vm.h>
#include <suspend_resume_port.h>
#include <suspend_resume_lcdui.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <midpInit.h>
#include <midpMalloc.h>
#include <midpError.h>
#include <midpPauseResume.h>

/** Suspendable resource that reprsents the VM. */
VM vm = { KNI_FALSE };
/** Suspendable resource that reprsents the LCDUI. */
LCDUIState lcduiState;

/** Java stack state from suspend/resume point of view. */
static int sr_state = SR_INVALID;

/** Flag to determine if a suspend operation killed all users MIDlets. */
static jboolean allMidletsKilled = KNI_FALSE;

/**
 * Resources list record for a resource to  be processed by
 * suspend/resume system. Nature of the resouce is unknown to
 * the suspend/resume system, only its state and suspend/resume
 * routines are important.
 */
typedef struct _SuspendableResource {
    SRState state;
    /**
     * Pointer to the resource.
     * IMPL_NOTE: resources are currently identified by this field.
     * If the field stops being unique due to appearance of some resources,
     * other identification mechanism should be implemented.
     */
    void *resource;
    /** A routine to be applied to the resource during system suspending. */
    SuspendResumeProc suspend;
    /** A routine to be applied to the resource during system resuming. */
    SuspendResumeProc resume;

    /** Pointer to the next resource in the list. */
    struct _SuspendableResource *next;
} SuspendableResource;

/** List of all registered suspendabe resources. */
static SuspendableResource *sr_resources = NULL;

/**
 * Empty procedure to be used as default one in case when no action is
 * required to suspend or resume a resource.
 */
SuspendResumeProc SR_EMPTY_PROC = NULL;

/** Returns current java stack state. */
SRState midp_getSRState() {
    return sr_state;
}

#define SWITCH_STATE(rs, fromState, proc, toState) \
    if ((rs)->state == fromState) { \
        if ((proc) == SR_EMPTY_PROC || (proc)((rs)->resource) == ALL_OK) { \
            (rs)->state = toState; \
        } else { \
            (rs)->state = SR_INVALID; \
        } \
    }

#define DESTROY_RESOURCE(target, prev) { \
    if (NULL == (prev)) { \
        sr_resources = (target)->next; \
    } else { \
        (prev)->next = (target)->next; \
    } \
    midpFree(target); \
}

void suspend_resources() {
    SuspendableResource *cur;

    REPORT_INFO(LC_LIFECYCLE, "suspend_resources()");

    for (cur = sr_resources; NULL != cur; cur = cur->next) {
        SWITCH_STATE(cur, SR_ACTIVE, cur->suspend, SR_SUSPENDED);
    }

    sr_state = SR_SUSPENDED;
    REPORT_INFO(LC_LIFECYCLE, "suspend_resources(): suspended");
}

void resume_resources() {
    SuspendableResource *cur;
    SuspendableResource *prev = NULL;

    REPORT_INFO(LC_LIFECYCLE, "resume_resources()");

    for (cur = sr_resources; NULL != cur; prev = cur, cur = cur->next) {
        SWITCH_STATE(cur, SR_SUSPENDED, cur->resume, SR_ACTIVE);

        if (cur->state == SR_INVALID) {
            DESTROY_RESOURCE(cur, prev);
            cur = prev;
        }
    }
}

/**
 * Requests java stack to release resources and suspend.
 * When the stack s suspended, there are two ways of requesting the stack
 * to resume. One is to pass control to midp_checkAndResume() and provide
 * midp_checkResumeRequest() to return KNI_TRUE. Another one is to simply
 * call midp_resume(). The ways are logically equivalent but one of them may
 * be more convenent on a port to a particular patform.
 */
void midp_suspend() {
    REPORT_INFO(LC_LIFECYCLE, "midp_suspend()");

    /* suspend request may arrive while system is not initialized */
    sr_initSystem();

    switch (sr_state) {
    case SR_ACTIVE:
        sr_state = SR_SUSPENDING;

        if (getMidpInitLevel() >= VM_LEVEL) {
            MidpEvent event;
            MIDP_EVENT_INITIALIZE(event)
            event.type = PAUSE_ALL_EVENT;
            midpStoreEventAndSignalAms(event);
        } else {
            suspend_resources();
        }
        break;
    case SR_RESUMING:
        sr_state = SR_SUSPENDING;
        break;
    default:
        break;
    }
}

void resume_java() {
    if (getMidpInitLevel() >= VM_LEVEL) {
        MidpEvent event;
        MIDP_EVENT_INITIALIZE(event);
        event.type = ACTIVATE_ALL_EVENT;
        midpStoreEventAndSignalAms(event);
    }

    sr_state = SR_ACTIVE;
    REPORT_INFO(LC_LIFECYCLE, "midp_resume(): midp resumed");
}

/**
 * Requests java stack to resume normal processing restoring resources
 * where possible.
 */
void midp_resume() {
    REPORT_INFO(LC_LIFECYCLE, "midp_resume()");

    switch (sr_state) {
    case SR_SUSPENDED:
        resume_resources();
        resume_java();
        break;
    case SR_SUSPENDING:
        sr_state = SR_RESUMING;
        break;

    default:
        break;
    }
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_suspend_SuspendSystem_isResumePending) {
    (void)midp_checkAndResume();
    KNI_ReturnBoolean(midp_getSRState() == SR_RESUMING);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_suspend_SuspendSystem_00024MIDPSystem_suspended0) {
    allMidletsKilled = KNI_GetParameterAsBoolean(1);

    /*
     * Checking that midp_resume() has not been called during suspending
     * of java side.
     */
    if (sr_state == SR_SUSPENDING) {
        suspend_resources();
    } else {
        /* the sate is SR_RESUMING - pending for safe resume. */
        resume_java();
    }

    KNI_ReturnVoid();
}

KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_suspend_SuspendSystem_00024MIDPSystem_allMidletsKilled) {
    jboolean ret = allMidletsKilled;
    allMidletsKilled = KNI_FALSE;
    KNI_ReturnBoolean(ret);
}

void sr_registerResource(
        void *resource,
        SuspendResumeProc suspendProc,
        SuspendResumeProc resumeProc) {

    SuspendableResource *sr = (SuspendableResource*)
            midpMalloc(sizeof(SuspendableResource));
    sr->next = sr_resources;
    sr_resources = sr;

    sr->resource = resource;
    sr->suspend = suspendProc;
    sr->resume = resumeProc;

    sr->state = SR_ACTIVE;
}

void sr_unregisterResource(void *resource) {
    SuspendableResource *cur;
    SuspendableResource *prev = NULL;

    for (cur = sr_resources; NULL!= cur; prev = cur, cur = cur->next) {
        if(cur->resource == resource) {
            DESTROY_RESOURCE(cur, prev);
            break;
        }
    }
}

void sr_initSystem() {
    if (SR_INVALID == sr_state) {
        lcduiState.locale = NULL;
        sr_registerResource((void*)&vm, &suspend_vm, &resume_vm);
        sr_registerResource((void*)&lcduiState, &suspend_lcdui, &resume_lcdui);        
        sr_state = SR_ACTIVE;
    }
}

void sr_repairSystem() {
    REPORT_INFO(LC_LIFECYCLE, "sr_repairSystem()");

    switch (sr_state) {
    case SR_RESUMING:
        sr_state = SR_ACTIVE;
        break ;
    case SR_SUSPENDING:
        suspend_resources();
        allMidletsKilled = KNI_TRUE;
        break;
    default:
        break;
    }
}

void sr_finalizeSystem() {
    SuspendableResource *cur;
    SuspendableResource *next;

    REPORT_INFO(LC_LIFECYCLE, "sr_finalizeSystem()");

    for (cur = sr_resources; NULL != cur; cur = next) {
        next = cur->next;
        midpFree(cur);
    }

    sr_resources = NULL;
    sr_state = SR_INVALID;
}

/**
 * Checks if there is active request for java stack to resume and invokes
 * stack resuming routines if requested. Makes nothing in case the java
 * stack is not currently suspended. There is no need to call this function
 * from the outside of java stack, it is called automatically if suspended
 * stack receives control.
 *
 * @return KNI_TRUE if java stack resuming procedures were invoked.
 */
jboolean midp_checkAndResume() {
    jboolean res = KNI_FALSE;
    SRState s = midp_getSRState();

    REPORT_INFO(LC_LIFECYCLE, "midp_checkAndResume()");

    if ((SR_SUSPENDED == s || SR_SUSPENDING == s) &&
            midp_checkResumeRequest()) {
        midp_resume();
        res = KNI_TRUE;
    }

    return res;
}

/**
 * Waits while java stack stays suspended then calls midp_resume().
 * If java stack is not currently suspened, returns false immediately.
 * Otherwise calls midp_checkAndResume() in cycle until it detects
 * resume request and performs resuming routines.
 *
 * Used in VM maser mode only.
 *
 * @retrun KNI_TRUE if java stack was suspended and resume request
 *         was detected within this call, KNI_FALSE otherwise
 */
jboolean midp_waitWhileSuspended() {
    jboolean ret = KNI_FALSE;

    while (SR_SUSPENDED == midp_getSRState()) {
        ret = KNI_TRUE;
        midp_checkAndResume();

        /*
         * IMPL_NOTE: condition here is not midp_checkAndResume() to
         * support special testing scenario when system is suspended
         * but VM continues working.
         */
        if (!vm.isSuspended) {
            break;
        }
    }

    return ret;
}
