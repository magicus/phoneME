/*
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

#include <stdio.h>
#include <kni.h>
#include <midpNativeAppManager.h>
#include <midpNamsTestEventProducer.h>

#define INVALID_ISOLATE_ID -1

static int namsTestServiceIsolateId = INVALID_ISOLATE_ID;

/**
 * The NAMS background listener callback function.
 */
static void nams_test_bg_lsnr(jint reason) {
    nams_send_bg_test_event(namsTestServiceIsolateId, reason);
}

/**
 * The NAMS foreground listener callback function.
 */
static void nams_test_fg_lsnr(jint appId, jint reason) {
    nams_send_fg_test_event(namsTestServiceIsolateId, appId, reason);
}

/**
 * The NAMS state change listener callback function.
 */
static void nams_test_state_lsnr(jint appId, jint state, jint reason) {
    nams_send_state_test_event(namsTestServiceIsolateId, appId, state, reason);
}

/**
 * Initializes the native portion of the NAMS Test Service.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsTestService_initialize(void) {
    namsTestServiceIsolateId = KNI_GetParameterAsInt(1);

    midp_system_set_background_listener(nams_test_bg_lsnr);
    midp_midlet_set_foreground_listener(nams_test_fg_lsnr);
    midp_midlet_set_state_change_listener(nams_test_state_lsnr);
}

/**
 * Cleans up the native portion of the NAMS Test Service.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsTestService_cleanup(void) {
    namsTestServiceIsolateId = INVALID_ISOLATE_ID;
    midp_system_set_background_listener(NULL);
    midp_midlet_set_foreground_listener(NULL);
    midp_midlet_set_state_change_listener(NULL);
}
