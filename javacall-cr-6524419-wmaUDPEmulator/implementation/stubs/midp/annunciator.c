/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * Sets the flashing effect for the device backlight. 
 * The flashing effect is intended to be used to attract the
 * user attention or as a special effect for games.
 *
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
 * Set the input mode.
 * Notify the platform to show the current input mode
 * @param mode equals the new mode just set values are one of the following:
 *             JAVACALL_INPUT_MODE_LATIN_CAPS      
 *             JAVACALL_INPUT_MODE_LATIN_LOWERCASE  
 *             JAVACALL_INPUT_MODE_NUMERIC         
 *             JAVACALL_INPUT_MODE_SYMBOL    
 *             JAVACALL_INPUT_MODE_T9
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_input_mode_icon(javacall_input_mode_type mode){
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
    
    
    
    
    
#ifdef __cplusplus
} //extern "C"
#endif

