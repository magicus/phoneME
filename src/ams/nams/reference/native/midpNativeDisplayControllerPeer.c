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

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midpEvents.h>
#include <midpNativeAppManager.h>
#include <midpEventUtil.h>

static MIDP_MIDLET_FOREGROUND_LISTENER foregroundListener = NULL;
static MIDP_SYSTEM_BACKGROUND_LISTENER backgroundListener = NULL;

/**
 * Sets the foreground listener.
 *
 * @param listener            The midlet foreground listener
 */
void midp_midlet_set_foreground_listener(
        MIDP_MIDLET_FOREGROUND_LISTENER listener) {
    foregroundListener = listener;
}

/**
 * Sets the background listener.
 *
 * @param listener            The background listener
 */
void midp_system_set_background_listener(
        MIDP_SYSTEM_BACKGROUND_LISTENER listener) {
    backgroundListener = listener;
}

/**
 * Select which running midlet should have the foreground.
 *
 * @param appId               The application id used to identify the app
 */
void midp_midlet_set_foreground(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_SET_FOREGROUND_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
}

/**
 * Notify the native application manager of the MIDlet foreground change.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeDisplayControllerPeer_notifyMidletHasForeground
        (void) {
    jint externalAppId = KNI_GetParameterAsInt(1);

    if (foregroundListener != NULL) {
        foregroundListener(externalAppId, 0);
    }
}

/**
 * Forwards MIDlet background requests to the native layer.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeDisplayControllerPeer_forwardBackgroundRequest
        (void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    (void)externalAppId; /* suppress compiler warnings */

    if (backgroundListener != NULL) {
        backgroundListener(0);
    }
}
