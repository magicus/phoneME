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

#include <kni.h>
#include <midpNativeAppManager.h>
#include <midpNamsTestEventProducer.h>

/* from nams_test_service_kni.c */
extern int g_namsTestServiceIsolateId;

/**
 * The NAMS background listener callback function.
 */
static void nams_test_bg_lsnr(const NamsEventData* pEventData) {
    if (pEventData->state != MIDP_DISPLAY_STATE_BACKGROUND &&
        pEventData->state != MIDP_DISPLAY_STATE_BACKGROUND_REQUEST) {
        /* probably foreground request - don't handle */
        return;
    }

    nams_send_bg_test_event(g_namsTestServiceIsolateId, pEventData->reason);
}

/**
 * The NAMS foreground listener callback function.
 */
static void nams_test_fg_lsnr(const NamsEventData* pEventData) {
    if (pEventData->state != MIDP_DISPLAY_STATE_FOREGROUND &&
        pEventData->state != MIDP_DISPLAY_STATE_FOREGROUND_REQUEST) {
        /* probably background request - don't handle */
        return;
    }

    nams_send_fg_test_event(g_namsTestServiceIsolateId, pEventData->appId,
                            pEventData->reason);
}

/**
 * The NAMS state change listener callback function.
 */
static void nams_test_state_lsnr(const NamsEventData* pEventData) {
    nams_send_state_test_event(g_namsTestServiceIsolateId, pEventData->appId,
                               pEventData->state, pEventData->reason);
}

/**
 * Initializes NAMS Test Service listeners.
 */
void nams_test_service_setup_listeners() {
    midp_add_event_listener(nams_test_bg_lsnr, DISPLAY_EVENT_LISTENER);
    midp_add_event_listener(nams_test_fg_lsnr, DISPLAY_EVENT_LISTENER);
    midp_add_event_listener(nams_test_state_lsnr, MIDLET_EVENT_LISTENER);
}

/**
 * Removes NAMS Test Service listeners.
 */
void nams_test_service_remove_listeners() {
    midp_remove_all_event_listeners(ANY_EVENT_LISTENER);
}
