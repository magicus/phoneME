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

#include <suspend_resume.h>
#include <suspend_resume_vm.h>
#include <suspend_resume_port.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <midpInit.h>
#include <midpMalloc.h>
#include <midpError.h>

/* Only required for default (testing) port. See midp_checkResumeRequest(). */
#include <suspend_resume_test.h>

/**  Java stack state from suspend/resume point of view. */
static jboolean sr_state = SR_INVALID;

/**
 * Testing purposes only. Used for fake implementation of
 * midp_checkResumeRequest().
 */
#define SUSPEND_TIMEOUT 10000

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
SuspendResumeProc   SR_EMPTY_PROC = NULL;

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

void midp_suspend() {
    REPORT_INFO(LC_LIFECYCLE, "midp_suspend()");

    /* suspend request may arrive while system is not initialized */
    sr_initSystem();

    if (SR_ACTIVE == sr_state) {
        if (getMidpInitLevel() >= VM_LEVEL) {
            MidpEvent event;
            MIDP_EVENT_INITIALIZE(event)
            event.type = PAUSE_ALL_EVENT;
            midpStoreEventAndSignalAms(event);
        } else {
            suspend_resources();
        }

        sr_state = SR_SUSPENDED;
        REPORT_INFO(LC_LIFECYCLE, "midp_suspend(): midp suspended");
    }
}

void midp_resume() {
    REPORT_INFO(LC_LIFECYCLE, "midp_resume()");

    if (SR_SUSPENDED == sr_state) {
        resume_resources();

        if (getMidpInitLevel() >= VM_LEVEL) {
            MidpEvent event;
            MIDP_EVENT_INITIALIZE(event);
            event.type = ACTIVATE_ALL_EVENT;
            midpStoreEventAndSignalAms(event);
        }

        sr_state = SR_ACTIVE;
        REPORT_INFO(LC_LIFECYCLE, "midp_resume(): midp resumed");
    }
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_suspend_SuspendSystem_00024MIDPSystem_suspended0) {
    suspend_resources();
    KNI_ReturnVoid();
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

VM vm = { KNI_FALSE };

void sr_initSystem() {
    if (SR_INVALID == sr_state) {
        sr_registerResource((void*)&vm, &suspend_vm, &resume_vm);
        sr_state = SR_ACTIVE;
    }
}

void sr_finalizeSystem() {
    SuspendableResource *cur;
    SuspendableResource *next;

    for (cur = sr_resources; NULL != cur; cur = next) {
        next = cur->next;
        midpFree(cur);
    }

    sr_resources = NULL;
    sr_state = SR_INVALID;
}

jboolean midp_checkAndResume() {
    jboolean res = KNI_FALSE;
    if (SR_SUSPENDED == sr_state && midp_checkResumeRequest()) {
        midp_resume();
        res = KNI_TRUE;
    }

    return res;
}
