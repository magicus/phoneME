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

/**
 * @file
 *
 * All native function related to javax.microedition.LCDUI class
 */


#include <sni.h>
#include <jvm.h>
#include <commonKNIMacros.h>

#include <lcdlf_export.h>
#include <midpEventUtil.h>
#include <gxapi_graphics.h>
#include <imgapi_image.h>

/**
 * Calls platform specific function to redraw a portion of the display.
 * <p>
 * Java declaration:
 * <pre>
 *     refresh0(IIIII)V
 * </pre>
 * Java parameters:
 * <pre>
 *   hardwareId The display hardware ID associated with the Display Device
 *   displayId The display ID associated with the Display object
 *   x1  Upper left corner x-coordinate
 *   y1  Upper left corner y-coordinate
 *   x2  Lower right corner x-coordinate
 *   y2  Lower right corner y-coordinate
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_DisplayDevice_refresh0) {
    int y2 = KNI_GetParameterAsInt(6);
    int x2 = KNI_GetParameterAsInt(5);
    int y1 = KNI_GetParameterAsInt(4);
    int x1 = KNI_GetParameterAsInt(3);
    jint displayId = KNI_GetParameterAsInt(2);
    jint hardwareId = KNI_GetParameterAsInt(1);

#ifdef JVM_HINT_VISUAL_OUTPUT
    {
#if ENABLE_ISOLATES
        int taskid = JVM_CurrentIsolateID();
#else
        int taskid = 0;
#endif
        // Make interpretation log less aggresive.
        JVM_SetHint(taskid, JVM_HINT_VISUAL_OUTPUT, 0);
    }
#endif

    if (midpHasForeground(displayId)) {
      // Paint only if this is the foreground MIDlet
      lcdlf_refresh(hardwareId, x1, y1, x2, y2);
    }

    KNI_ReturnVoid();
}

/**
 *
 * Calls platform specific function to set display area 
 * to be in full screen or normal screen mode for drawing.
 * <p>
 * Java declaration:
 * <pre>
 *    setFullScreen0(IZ)V
 * </pre>
 * Java parameters:
 * <pre>
 *    hardwareId The display hardware ID associated with the Display Device
 *    displayId The display ID associated with the Display object
 *    mode If true we should grab all area available
 *         if false, relinquish area to display 
 *         status and commands
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_DisplayDevice_setFullScreen0) {
    jboolean mode = KNI_GetParameterAsBoolean(3);
    jint displayId = KNI_GetParameterAsInt(2);
    jint hardwareId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      lcdlf_set_fullscreen_mode(hardwareId, mode);
    }
    KNI_ReturnVoid();
}

/**
 *
 * Calls platform specific function to clear native resources
 * when foreground is gained by a new Display.
 * <p>
 * Java declaration:
 * <pre>
 *    gainedForeground0(I)V
 * </pre>
 * Java parameters:
 * <pre>
 *    hardwareId The display hardware ID associated with the Display Device
 *    displayId The display ID associated with the Display object
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_DisplayDevice_gainedForeground0) {
    jint displayId = KNI_GetParameterAsInt(2);
    jint hardwareId = KNI_GetParameterAsInt(1);


    if (midpHasForeground(displayId)) {
      lcdlf_gained_foreground(hardwareId);
    }
    KNI_ReturnVoid();
}

/**
 * Calls platform specific function to invert screen orientation flag
 * Java parameters:
 * <pre>
 *    hardwareId The display hardware ID associated with the Display Device
 * </pre>
*/
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_reverseOrientation0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    jboolean res = 0;
    res = lcdlf_reverse_orientation(hardwareId);
    KNI_ReturnBoolean(res);
}

/**
 * Calls platform specific function to handle clamshell event 
*/
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_DisplayDevice_clamshellHandling0) {
    lcdlf_handle_clamshell_event();
    KNI_ReturnVoid();
}




/**
 * Calls platform specific function to invert screen orientation flag
 * Java parameters:
 * <pre>
 *    hardwareId The display hardware ID associated with the Display Device
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_getReverseOrientation0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    jboolean res = 0;
    res = lcdlf_get_reverse_orientation(hardwareId);
    KNI_ReturnBoolean(res);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_lcdui_DisplayDevice_getScreenHeight0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    int height = lcdlf_get_screen_height(hardwareId);
    KNI_ReturnInt(height);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_lcdui_DisplayDevice_getScreenWidth0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    int height = lcdlf_get_screen_width(hardwareId);
    KNI_ReturnInt(height);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_directFlush0) {
    jboolean success = KNI_FALSE;
    int height  = KNI_GetParameterAsInt(4);
    jint hardwareId;

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(g);

    KNI_GetParameterAsObject(3, img);
    KNI_GetParameterAsObject(2, g);
    hardwareId = KNI_GetParameterAsInt(1);

    if (!KNI_IsNullHandle(img)) {
      success = lcdlf_direct_flush(hardwareId, GXAPI_GET_GRAPHICS_PTR(g), 
				   IMGAPI_GET_IMAGE_PTR(img)->imageData,
				   height);
    }

    KNI_EndHandles();

    KNI_ReturnBoolean(success);
} 

/**
 * Calls platform specific function to get the dislay device name 
 * Java parameters:
 * <pre>
 *    hardwareId The display hardware ID associated with the Display Device
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_lcdui_DisplayDevice_getDisplayName0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    char *displayName = lcdlf_get_display_name(hardwareId);

    KNI_StartHandles(1);
    KNI_DeclareHandle(str);

    if (displayName != NULL) {
        KNI_NewStringUTF(displayName, str);
    } else {
      KNI_ReleaseHandle(str); /* Set 'str' to null String object */
    }
    KNI_EndHandlesAndReturnObject(str);
}


/**
 * Calls platform specific function to get the dislay device name 
 * Java parameters:
 * <pre>
 *    hardwareId The display hardware ID associated with the Display Device
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_isDisplayPrimary0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    jboolean res = lcdlf_is_display_primary(hardwareId);
    KNI_ReturnBoolean(res);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_isbuildInDisplay0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    jboolean res = lcdlf_is_display_buildin(hardwareId);
    KNI_ReturnBoolean(res);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_isDisplayPenSupported0) {
    jint hardwareId =  KNI_GetParameterAsInt(1);
    jboolean res = lcdlf_is_display_pen_supported(hardwareId);
    KNI_ReturnBoolean(res);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDevice_isDisplayPenMotionSupported0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    jboolean res = lcdlf_is_display_pen_motion_supported(hardwareId);
    KNI_ReturnBoolean(res);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_lcdui_DisplayDevice_getDisplayCapabilities0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    int height = lcdlf_get_display_capabilities(hardwareId);
    KNI_ReturnInt(height);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_lcdui_DisplayDeviceContainer_getDisplayDevicesIds0) {
    int i;

    jint n;
    jint* display_device_ids = lcdlf_get_display_device_ids(&n);

    KNI_StartHandles(1);
    KNI_DeclareHandle(returnArray);

    SNI_NewArray(SNI_INT_ARRAY, n, returnArray);

    for (i = 0; i < n; i++) {
      KNI_SetIntArrayElement(returnArray, (jint)i, display_device_ids[i]);
    }

    KNI_EndHandlesAndReturnObject(returnArray);
}
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_DisplayDevice_displayStateChanged0) {
    jint hardwareId = KNI_GetParameterAsInt(1);
    jint state = KNI_GetParameterAsInt(2);
    lcdlf_display_device_state_changed(hardwareId, state);
    KNI_ReturnVoid();
}
