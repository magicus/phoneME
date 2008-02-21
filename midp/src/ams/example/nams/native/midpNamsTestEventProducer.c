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

#include <midpEvents.h>
#include <midpNativeAppManager.h>
#include <midpNamsTestEventProducer.h>

/**
 * Creates a NAMS test event and sends it to the event queue of the
 * indicated isolate.
 *
 * @param isolateId the destination isolate
 * @param p1 the value for intParam1, typically the appId
 * @param p2 the value for intParam2, typically indicating the NAMS callback
 * @param p3 the value for intParam3, typically the midlet state
 * @param p4 the value for intParam4, typically the reason code
 */
static void
nams_test_event_post(int isolateId, jint p1, jint p2, jint p3, jint p4) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);
    evt.type = TEST_EVENT;
    evt.intParam1 = p1;
    evt.intParam2 = p2;
    evt.intParam3 = p3;
    evt.intParam4 = p4;

    StoreMIDPEventInVmThread(evt, isolateId);
}


/**
 * See midpNamsTestEventProducer.h for documentation.
 */
void
nams_send_bg_test_event(int isolateId, jint reason) {
    nams_test_event_post(isolateId,
                         -1,      /* unused */
                         0,       /* background listener */
                         MIDP_DISPLAY_STATE_BACKGROUND,
                         reason);
}


/**
 * See midpNamsTestEventProducer.h for documentation.
 */
void
nams_send_fg_test_event(int isolateId, jint appId, jint reason) {
    nams_test_event_post(isolateId,
                         appId,
                         1,       /* foreground listener */
                         MIDP_DISPLAY_STATE_FOREGROUND,
                         reason);
}


/**
 * See midpNamsTestEventProducer.h for documentation.
 */
void
nams_send_state_test_event(int isolateId,
                           jint appId,
                           jint state,
                           jint reason) {
    nams_test_event_post(isolateId,
                         appId,
                         2,       /* state change listener */
                         state,
                         reason);
}
