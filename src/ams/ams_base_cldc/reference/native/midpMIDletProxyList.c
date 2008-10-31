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

#include <string.h>

#include <midpMidletSuiteUtils.h>
#include <midpMIDletProxyList.h>
#include <midp_foreground_id.h>
#include <midpPauseResume.h>
#include <midpMalloc.h>
/**
 * @file
 * Native state of the MIDlet proxy list.
 */


/** Reset the MIDlet proxy list for the next run of the VM. */
void midpMIDletProxyListReset() {
#if ENABLE_MULTIPLE_DISPLAYS
    int sizeInBytes = maxDisplays * sizeof(int);
    gForegroundDisplayIds = midpMalloc(sizeInBytes);
    memset(gForegroundDisplayIds, 0, sizeInBytes);
#else
    gForegroundDisplayId = -1;
#endif // ENABLE_MULTIPLE_DISPLAYS   
    gForegroundIsolateId = midpGetAmsIsolateId();
}

/**
 * Sets the foreground MIDlet.
 *
 * @param isolateId ID of the foreground Isolate
 * @param displayId ID of the foreground Display
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_setForegroundInNativeState) {
#if ENABLE_MULTIPLE_DISPLAYS
    int displayId = KNI_GetParameterAsInt(2);
    int i;
    for (i = 0; i < maxDisplays; i++) {
      if (gForegroundDisplayIds[i] == -1) {
	gForegroundDisplayIds[i] = displayId;
	break;
      }
    }
#else
    gForegroundDisplayId = KNI_GetParameterAsInt(2);
#endif /* ENABLE_MULTIPLE_DISPLAYS */
    gForegroundIsolateId = KNI_GetParameterAsInt(1);
    KNI_ReturnVoid();
}


/**
 * Resets the foreground MIDlet.
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_resetForegroundInNativeState) {
#if ENABLE_MULTIPLE_DISPLAYS
  int i;  
  for (i = 0; i < maxDisplays; i++) {
    gForegroundDisplayIds[i] = -1;
  }
#else 
    gForegroundDisplayId = -1;
#endif /* ENABLE_MULTIPLE_DISPLAYS */
  
  KNI_ReturnVoid();
}


/**
 * Native implementation of Java function: notifySuspendAll0().
 * Class: com.sun.midp.lcdui.DefaultEventHandler.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_notifySuspendAll0) {
    /* Call the platform dependent code */
    pdMidpNotifySuspendAll();

    KNI_ReturnVoid();
}


/**
 * Native implementation of Java function: notifyResumeAll0().
 * Class: com.sun.midp.lcdui.DefaultEventHandler.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletProxyList_notifyResumeAll0) {
    /* Call the platform dependent code */
    pdMidpNotifyResumeAll();

    KNI_ReturnVoid();
}
