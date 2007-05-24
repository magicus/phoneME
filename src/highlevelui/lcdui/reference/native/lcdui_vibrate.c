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
 * Internal native implementation of Java function nVibrate()
 *
 * @verbatim 
 * FUNCTION:      vibrate0(II)Z 
 * CLASS:         javax.microedition.lcdui.Display
 * TYPE:          native function to play vibration
 * OVERVIEW:      turn on the vibrator mechanism for <code>dur</code>
 *                microseconds, or turn off the vibrator if 
 *                <code>dur</code> is zero, return false if this
 *                <code>Display</code> does not have foreground.
 * INTERFACE (operand stack manipulation):
 *   parameters:  display the display ID associated with the Display object
 *                dur duration of vibration in microseconds
 *   returns:     KNI_TRUE if the device supports vibrate and Display has
 *                foreground,
 *                KNI_FALSE otherwise
 * @endverbatim
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Display_vibrate0) {
    int dur = KNI_GetParameterAsInt(2);
    int displayId = KNI_GetParameterAsInt(1);

    if(!midpHasForeground(displayId) || anc_stop_vibrate() == KNI_FALSE){
        KNI_ReturnBoolean(KNI_FALSE);
    } else {
        KNI_ReturnBoolean(anc_start_vibrate(dur));
    }
}
