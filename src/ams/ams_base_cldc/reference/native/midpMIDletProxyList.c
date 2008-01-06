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
#include <midpMidletSuiteUtils.h>
#include <midpMIDletProxyList.h>
#include <midp_foreground_id.h>
#include <midpPauseResume.h>

/**
 * @file
 * Native state of the MIDlet proxy list.
 */

/** Reset the MIDlet proxy list for the next run of the VM. */
void midpMIDletProxyListReset() {
    gForegroundIsolateId = midpGetAmsIsolateId();
    gForegroundDisplayId = -1;
}

/**
 * Sets the foreground MIDlet.
 *
 * @param isolateId ID of the foreground Isolate
 * @param displayId ID of the foreground Display
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_setForegroundInNativeState) {
    gForegroundIsolateId = KNI_GetParameterAsInt(1);
    gForegroundDisplayId = KNI_GetParameterAsInt(2);
    KNI_ReturnVoid();
}


/**
 * Native implementation of Java function: notifyPausedAll0().
 * Class: com.sun.midp.main.MIDletProxyList.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_notifyPausedAll0) {
    /* Call the platform dependent code */
    pdMidpNotifyPausedAll();

    KNI_ReturnVoid();
}


/**
 * Native implementation of Java function: notifyResumedAll0().
 * Class: com.sun.midp.main.MIDletProxyList.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_notifyResumedAll0) {
    /* Call the platform dependent code */
    pdMidpNotifyResumedAll();

    KNI_ReturnVoid();
}

/**
 * Native implementation of Java function: notifyInternalPausedAll0().
 * Class: com.sun.midp.main.MIDletProxyList.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_notifyInternalPausedAll0) {

    /* Call the platform dependent code */
    pdMidpNotifyInternalPausedAll();

    KNI_ReturnVoid();
}

/**
 * Native implementation of Java function: notifyInternalResumedAll0().
 * Class: com.sun.midp.main.MIDletProxyList.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_notifyInternalResumedAll0) {

    /* Call the platform dependent code */
    pdMidpNotifyInternalResumedAll();

    KNI_ReturnVoid();
}

