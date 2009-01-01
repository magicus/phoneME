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
#include <anc_vibrate.h>
#include <midpEventUtil.h>

/**
 * @file
 *
 * Internal native function implementation for MIDP vibrate functions
 */

/**
 * @internal
 *
 * Internal native implementation of Java function vibrate0()
 *
 * @verbatim 
 * FUNCTION:      vibrate0(II)Z 
 * CLASS:         com.sun.midp.lcdui.DisplayDeviceAccess
 * TYPE:          native function to play vibration
 * OVERVIEW:      turn on the vibrator mechanism if <code>onOff</code>
 *                is true, or off if it is false.
 *                return false if this <code>Display</code> does not have foreground.
 * INTERFACE (operand stack manipulation):
 *   parameters:  display the display ID associated with the Display object
 *                onOff start/stop vibrator
 *   returns:     KNI_TRUE if the device supports vibrate and Display has
 *                foreground,
 *                KNI_FALSE otherwise
 * @endverbatim
 */

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_DisplayDeviceAccess_vibrate0) {
    jboolean onoff = KNI_GetParameterAsBoolean(2);
    int displayId = KNI_GetParameterAsInt(1);

    if(!midpHasForeground(displayId) || anc_stop_vibrate() == KNI_FALSE){
        KNI_ReturnBoolean(KNI_FALSE);
    } else if (KNI_FALSE == onoff) {
        KNI_ReturnBoolean(anc_stop_vibrate());
    } else {
        KNI_ReturnBoolean(anc_start_vibrate());
    }
}

