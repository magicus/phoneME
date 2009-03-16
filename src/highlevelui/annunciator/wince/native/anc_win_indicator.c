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

/**
 * @file
 *
 * Native code to handle indicator status.
 */
extern jboolean drawBackLight(AncBacklightState mode);

/**
 * Platform handling code for turning off or on
 * indicators for signed MIDlet.
 *
 * @todo Currently indicator does nothing for Java
 * and platform widget modules as we are waiting for
 * UI input.
 */
void anc_show_trusted_indicator(jboolean isTrusted) {
    REPORT_WARN(LC_LOWUI, "anc_show_trusted_indicator: Stubbed out.");
    /* @todo implement security indicator and remove
     * the temporary workaround for warning removal.
     */
    (void)isTrusted;
}

/**
 * Porting implementation for network indicator.
 * It controls the LED as the network indicator, it
 * ONLY works on device. There is no equivalent in emulator.
 */
void anc_set_network_indicator(int counter) {
    REPORT_WARN(LC_LOWUI, "anc_set_network_indicator: Stubbed out.");
    /* Work around for compiler warning */
    (void)counter;
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
    return drawBackLight(mode);
}

/**
 * Turn Home indicator on or off.
 */
void anc_toggle_home_icon(jboolean isHomeOn) {
    REPORT_WARN(LC_LOWUI, "anc_toggle_home_icon: Stubbed out.");
    (void)isHomeOn;
}
