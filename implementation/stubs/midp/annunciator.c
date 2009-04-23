/*
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

#ifdef __cplusplus
extern "C" {
#endif
    
#include "javacall_annunciator.h"

/**
 * Turn device's Vibrate on/off
 *
 * @param enableVibrate if <tt>1</tt>, turn vibrate on, else turn vibrate off
 *
 * @return <tt>JAVACALL_OK</tt> if device supports vibration or
 *         <tt>JAVACALL_FAIL</tt> if the device does not supports 
 *         vibration
 *                
 */
javacall_result javacall_annunciator_vibrate(javacall_bool enableVibrate){
    return JAVACALL_FAIL;
}
    
/**
 * Sets the backlight state of the device.
 *
 * The MIDP2.x specification defines a method on the Display class 
 * called <code>flashBacklight(duration)</code>, which invokes a 
 * "flashing effect".
 * The flashing effect is intended to be used to attract the
 * users attention, or as a special effect for games.
 *
 * The javacall_annunciator_flash_backlight() function takes one
 * boolean parameter, used to set the backlight to either "bright" or
 * "dim". The exact brightness values are left to the discretion of the
 * implementation.
 *
 * Initial state of the backlight (i.e., before this API is invoked for
 * the first time) is assumed to be "bright". Java will generally leave
 * the backlight in the "bright" state when not flashing, but this
 * is not guaranteed.
 *
 * @param  enableBrightBack <tt>1</tt> to turn backlight to bright mode
 *         <tt>0</tt> to turn backlight to dim mode
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 *                
 */
javacall_result javacall_annunciator_flash_backlight(javacall_bool enableBrightBack){
    return JAVACALL_FAIL;
}
    
    
/**
 * Turning trusted indicator icon off or on, for signed MIDlets.
 *
 * @param enableTrustedIcon boolean value specifying whether running MIDlet 
 *         is signed
 *                
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_trusted_icon(javacall_bool enableTrustedIcon){
    return JAVACALL_FAIL;
}
    

/**
 * Controls the network LED or equivalent network indicator.
 *
 * @param enableNetworkIndicator boolean value indicating if network indicator
 *             icon should be enabled
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_network_icon(javacall_bool enableNetworkIndicator){
    return JAVACALL_FAIL;
}

/**
 * Turning Home indicator off or on. 
 *
 * @param enableHomeIndicator boolean value indicating if home indicator
 *             icon should be enabled
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_home_icon(javacall_bool enableHomeIndicator) {
    return JAVACALL_FAIL;
}


/**
 * Play a sound of the given type.
 *
 * @param soundType must be one of the sound types defined
 *        JAVACALL_AUDIBLE_TONE_INFO         : Sound for informative alert
 *        JAVACALL_AUDIBLE_TONE_WARNING      : Sound for warning alert 
 *        JAVACALL_AUDIBLE_TONE_ERROR        : Sound for error alert 
 *        JAVACALL_AUDIBLE_TONE_ALARM        : Sound for alarm alert
 *        JAVACALL_AUDIBLE_TONE_CONFIRMATION : Sound for confirmation alert
 * @return <tt>JAVACALL_OK<tt> if a sound was actually emitted or  
 *         <tt>JAVACALL_FAIL</tt> or negative value if error occured 
 *         or device is in mute mode 
 */
javacall_result javacall_annunciator_play_audible_tone(javacall_audible_tone_type soundType) {
    return JAVACALL_FAIL;
}
    
    
/**
 * Get feedback on touching a control
 * @param type must be one of the types defined in javacall_tactile_type enum
 */
javacall_result javacall_play_tactile_feedback(javacall_tactile_type type) {
    return JAVACALL_FAIL;
}
    
#ifdef __cplusplus
} //extern "C"
#endif

