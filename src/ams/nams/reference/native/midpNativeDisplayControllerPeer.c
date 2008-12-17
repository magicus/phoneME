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

#include <stdio.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midpEvents.h>
#include <midpNativeAppManager.h>
#include <midpEventUtil.h>

/* from midpNativeAppManager.c */
extern MIDPError
nams_listeners_notify(NamsListenerType listenerType,
                      const NamsEventData* pEventData);

/**
 * Select which running MIDlet should have the foreground.  If appId is a
 * valid application ID, that application is placed into the foreground. If
 * appId is MIDLET_APPID_NO_FOREGROUND, the current foreground MIDlet will be
 * put into background and no MIDlet will have the foreground.
 *
 * If appId is invalid, or that application already has the foreground, this
 * has no effect, but the foreground listener will be called anyway.
 *
 * @param appId The ID of the application to be put into the foreground,
 *              or the special value MIDLET_APPID_NO_FOREGROUND (that is
 *              defined in src/configuration/common/constants.xml)
 *
 * @return error code: ALL_OK if successful
 */
MIDPError midp_midlet_set_foreground(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_SET_FOREGROUND_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Notify the native application manager of the MIDlet foreground change.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeDisplayControllerPeer_notifyMidletHasForeground
        (void) {
    NamsEventData eventData;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    eventData.event  = MIDP_NAMS_EVENT_STATE_CHANGED;
    eventData.appId  = KNI_GetParameterAsInt(1);
    eventData.state  = MIDP_DISPLAY_STATE_FOREGROUND;
    eventData.reason = 0;

    nams_listeners_notify(DISPLAY_EVENT_LISTENER, &eventData);
}

/**
 * Forwards MIDlet background requests to the native layer.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeDisplayControllerPeer_forwardBackgroundRequest
        (void) {
    NamsEventData eventData;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    eventData.event  = MIDP_NAMS_EVENT_STATE_CHANGED;
    eventData.appId  = KNI_GetParameterAsInt(1);
    eventData.state  = MIDP_DISPLAY_STATE_BACKGROUND_REQUEST;
    eventData.reason = 0;
    
    nams_listeners_notify(DISPLAY_EVENT_LISTENER, &eventData);
}
