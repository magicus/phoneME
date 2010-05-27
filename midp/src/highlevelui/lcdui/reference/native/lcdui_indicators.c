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

#include <commonKNIMacros.h>
#include <anc_indicators.h>
#include <midpEventUtil.h>

/**
 * @file
 *
 * @brief Public share implementation related to any indicator.
 *
 * This file contains all share native function to handle
 * drawing indicators, including trusted icon, networking 
 * indicator, and backlight.
 */

/**
 * IMPL_NOTE:(Deoxy - Consider using internal)
 *
 * FUNCTION:      drawTrustedIcon(II)V
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          virtual native function
 * OVERVIEW:      set the drawing of the trusted MIDlet icon
 * INTERFACE (operand stack manipulation):
 *   parameters:  displayId The display ID associated with the caller Display
 *                drawTrusted     
 *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Display_drawTrustedIcon0) {
    jboolean drawTrusted = KNI_GetParameterAsBoolean(2);
    jint displayId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      /* Call platform dependent implementation */
      anc_show_trusted_indicator(drawTrusted);
    }
  
    KNI_ReturnVoid();
}

/**
 * IMPL_NOTE:(Deoxy - Consider using internal)
 *
 * IMPL_NOTE:(Deoxy - Consider using verbatim)
 * FUNCTION:      showBacklight(II)Z
 * CLASS:         com.sun.midp.lcdui.DisplayDeviceAccess
 * TYPE:          virtual native function
 * OVERVIEW:      toggle Backlight
 * INTERFACE (operand stack manipulation):
 *   parameters:  displayId The display ID associated with the caller Display
 *
 *   returns:     KNI_TRUE if backlight is controllable,
 *                KNI_FALSE otherwise
 * IMPL_NOTE:(Deoxy - Consider using endverbatim)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDeviceAccess_toggleBacklight0) {
    jint displayId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      /* Call the platform dependent to toggle the backlight */ 
      KNI_ReturnBoolean((jboolean)anc_show_backlight(ANC_BACKLIGHT_TOGGLE));
    }

    KNI_ReturnBoolean(KNI_FALSE);
}

/**
 * IMPL_NOTE:(Deoxy - Consider using internal)
 *
 * IMPL_NOTE:(Deoxy - Consider using verbatim)
 * FUNCTION:      isBacklightSupported (II)Z
 * CLASS:         com.sun.midp.lcdui.DisplayDeviceAccess
 * TYPE:          virtual native function
 * OVERVIEW:      tests if backlight supported
 * INTERFACE (operand stack manipulation):
 *   parameters:  displayId The display ID associated with the caller Display
 *
 *   returns:     KNI_TRUE if backlight is controllable,
 *                KNI_FALSE otherwise
 * IMPL_NOTE:(Deoxy - Consider using endverbatim)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDeviceAccess_isBacklightSupported0) {
    jint displayId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      /* Call the platform dependent to toggle the backlight */ 
      KNI_ReturnBoolean((jboolean)anc_show_backlight(ANC_BACKLIGHT_SUPPORTED));
    }

    KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_IndicatorManager_toggleHomeIcon0) {

    jboolean isHomeOn = KNI_GetParameterAsBoolean(1);

    anc_toggle_home_icon(isHomeOn);

    KNI_ReturnVoid();
}


#ifdef ENABLE_NETWORK_INDICATOR

int gAncNetworkIndicatorCount = 0;

#endif

