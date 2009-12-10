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


#include <kni.h>
#include <anc_indicators.h>
#include <midp_logging.h>
#include <javacall_annunciator.h>

/**
 * @file
 *
 * Native code to handle indicator status.
 */

/**
 * Platform handling code for turning off or on
 * indicators for signed MIDlet.
 */
void anc_show_trusted_indicator(jboolean isTrusted) {
    (void) javacall_annunciator_display_trusted_icon(isTrusted);
}

/**
 * Porting implementation for network indicator.
 */
void anc_set_network_indicator(int counter) {
    (void) javacall_annunciator_display_network_icon(counter);
}

/**
 *  Turn on or off the backlight, or toggle it.
 *  The backlight will be turned on to the system configured level.
 *  This function is only valid if QT's COP and QWS is available.
 *
 *  @param mode if <code>mode</code> is:
 *              <code>ANC_BACKLIGHT_ON</code> - turn on the backlight  
 *              <code>ANC_BACKLIGHT_OFF</code> - turn off the backlight  
 *              <code>ANC_BACKLIGHT_TOGGLE</code> - toggle the backlight
 *              <code>ANC_BACKLIGHT_SUPPORTED<code> - do nothing  
 *              (this is used to determine if backlight control is 
 *              supported on a system without  changing the state of 
 *              the backlight.)
 *  @return <code>KNI_TRUE</code> if the system supports backlight 
 *              control, or <code>KNI_FALSE</code> otherwise.
 */
jboolean anc_show_backlight(AncBacklightState mode) {
    static int backlightEnabled = 0;
    javacall_result result;

        switch (mode) {
        case ANC_BACKLIGHT_ON:
                backlightEnabled = 1;
                break;
        case ANC_BACKLIGHT_OFF:
                backlightEnabled = 0;
                break;
        case ANC_BACKLIGHT_TOGGLE:
                backlightEnabled = backlightEnabled ? 0 : 1;
                break;
        case ANC_BACKLIGHT_SUPPORTED:
                return KNI_TRUE;
                break;
        default:
                return KNI_FALSE;
                break;
        }

    result = javacall_annunciator_flash_backlight(backlightEnabled);

    return (result == JAVACALL_OK ? KNI_TRUE : KNI_FALSE);
}

/**
 * Turn Home indicator on or off.
 */ 
void anc_toggle_home_icon(jboolean isHomeOn) {
    (void) javacall_annunciator_display_home_icon(isHomeOn);
}
