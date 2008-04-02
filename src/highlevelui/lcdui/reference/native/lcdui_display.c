/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *   displayId The display ID associated with the Display object
 *   x1  Upper left corner x-coordinate
 *   y1  Upper left corner y-coordinate
 *   x2  Lower right corner x-coordinate
 *   y2  Lower right corner y-coordinate
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Display_refresh0) {
    int y2 = KNI_GetParameterAsInt(5);
    int x2 = KNI_GetParameterAsInt(4);
    int y1 = KNI_GetParameterAsInt(3);
    int x1 = KNI_GetParameterAsInt(2);
    jint displayId = KNI_GetParameterAsInt(1);

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
      lcdlf_refresh(x1, y1, x2, y2);
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
 *    displayId The display ID associated with the Display object
 *    mode If true we should grab all area available
 *         if false, relinquish area to display 
 *         status and commands
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Display_setFullScreen0) {
    jboolean mode = KNI_GetParameterAsBoolean(2);
    jint displayId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      lcdlf_set_fullscreen_mode(mode);
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
 *    displayId The display ID associated with the Display object
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Display_gainedForeground0) {
    jint displayId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      lcdlf_gained_foreground();
    }
    KNI_ReturnVoid();
}

/**
 * Calls platform specific function to invert screen orientation flag
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Display_reverseOrientation0) {
    jboolean res = 0;
    res = lcdlf_reverse_orientation();
    KNI_ReturnBoolean(res);
}

/**
 * Calls platform specific function to invert screen orientation flag
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Display_getReverseOrientation0) {
    jboolean res = 0;
    res = lcdlf_get_reverse_orientation();
    KNI_ReturnBoolean(res);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Display_getScreenHeight0) {
    int height = lcdlf_get_screen_height();
    KNI_ReturnInt(height);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Display_getScreenWidth0) {
    int height = lcdlf_get_screen_width();
    KNI_ReturnInt(height);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Display_directFlush) {
    jboolean success = KNI_FALSE;
    int height  = KNI_GetParameterAsInt(3);

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(g);

    KNI_GetParameterAsObject(2, img);
    KNI_GetParameterAsObject(1, g);

    if (!KNI_IsNullHandle(img)) {
      success = lcdlf_direct_flush(GXAPI_GET_GRAPHICS_PTR(g), 
				   IMGAPI_GET_IMAGE_PTR(img)->imageData,
				   height);
    }

    KNI_EndHandles();

    KNI_ReturnBoolean(success);
} 
