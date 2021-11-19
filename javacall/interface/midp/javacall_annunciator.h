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

#ifndef __JAVACALL_ANNUNCIATOR_H
#define __JAVACALL_ANNUNCIATOR_H



#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_annunciator.h
 * @ingroup MandatoryAnnunciator
 * @brief Javacall interfaces for annunciator
 */
 
#include "javacall_defs.h" 




    
/** @defgroup MandatoryAnnunciator Annunciator API 
 * @ingroup JTWI
 * Annunciator APIs define the functionality for:
 *   
 * - Vibrate 
 * - Backlight 
 * - Input indicator icon
 * - Trusted indicator icon
 * - Network indicator icon
 * - Play of preset audible tone
 * 
 * @{
 */


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
javacall_result javacall_annunciator_vibrate(javacall_bool enableVibrate);
    
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
javacall_result javacall_annunciator_flash_backlight(
                     javacall_bool enableBrightBack);
    
    
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
javacall_result javacall_annunciator_display_trusted_icon(
                     javacall_bool enableTrustedIcon);
    



    
/**
 * Controls the network LED or equivalent network indicator.
 *
 * @param enableNetworkIndicator boolean value indicating if network indicator
 *             icon should be enabled
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_network_icon(
                     javacall_bool enableNetworkIndicator);





/**
 * @enum javacall_input_mode_type
 * @brief Input mode icon type
 */
typedef enum {
    /** Input mode indication for Latin caps */
    JAVACALL_INPUT_MODE_LATIN_CAPS      =1,
    /** Input mode indication for Latin lowercase */
    JAVACALL_INPUT_MODE_LATIN_LOWERCASE =2,
    /** Input mode indication for numeric */
    JAVACALL_INPUT_MODE_NUMERIC         =3,
    /** Input mode indication for symbol */
    JAVACALL_INPUT_MODE_SYMBOL          =4,
    /** Input mode indication for T9 */
    JAVACALL_INPUT_MODE_T9              =5,
    /** Input mode indication for off */
    JAVACALL_INPUT_MODE_OFF             =6
} javacall_input_mode_type;

/**
 * Set the input mode.
 * Notify the platform to show the current input mode
 * @param mode equals the new mode just set values are one of the following :
 *        - JAVACALL_INPUT_MODE_LATIN_CAPS      
 *        - JAVACALL_INPUT_MODE_LATIN_LOWERCASE  
 *        - JAVACALL_INPUT_MODE_NUMERIC         
 *        - JAVACALL_INPUT_MODE_SYMBOL    
 *        - JAVACALL_INPUT_MODE_T9
 *        - JAVACALL_INPUT_MODE_OFF
 *             
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_input_mode_icon(
                     javacall_input_mode_type mode);
  

/**
 * Turning Home indicator off or on. 
 *
 * @param enableHomeIndicator boolean value indicating if home indicator
 *             icon should be enabled
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not 
 *         supported on device
 */
javacall_result javacall_annunciator_display_home_icon(
                     javacall_bool enableHomeIndicator);



/**
 * @enum javacall_audible_tone_type
 * @brief Audible tone type for alert
 */
typedef enum {
    /** An INFO tone type typically provides non-threatening information to the user. */
    JAVACALL_AUDIBLE_TONE_INFO         =1,
    /** A WARNING tone type is a hint to warn the user of a potentially dangerous operation. */
    JAVACALL_AUDIBLE_TONE_WARNING      =2,
    /** An ERROR tone type is a hint to alert the user to an erroneous operation. */
    JAVACALL_AUDIBLE_TONE_ERROR        =3,
    /** An ALARM tone type is a hint to alert the user to an event for which 
        the user has previously requested to be notified. */
    JAVACALL_AUDIBLE_TONE_ALARM        =4,
    /** A CONFIRMATION tone type is a hint to confirm user actions. */
    JAVACALL_AUDIBLE_TONE_CONFIRMATION =5
}javacall_audible_tone_type;

/**
 * Play a sound of the given type.
 *
 * @param soundType must be one of the sound types defined
 *        - JAVACALL_AUDIBLE_TONE_INFO         : Sound for informative alert
 *        - JAVACALL_AUDIBLE_TONE_WARNING      : Sound for warning alert 
 *        - JAVACALL_AUDIBLE_TONE_ERROR        : Sound for error alert 
 *        - JAVACALL_AUDIBLE_TONE_ALARM        : Sound for alarm alert
 *        - JAVACALL_AUDIBLE_TONE_CONFIRMATION : Sound for confirmation alert
 *        
 * @return <tt>JAVACALL_OK</tt> if a sound was actually emitted or  
 *         <tt>JAVACALL_FAIL</tt> or negative value if error occured 
 *         or device is in mute mode 
 */
javacall_result javacall_annunciator_play_audible_tone(
                     javacall_audible_tone_type soundType);
    
/**
 * @enum javacall_tactile_type
 * @brief Tactile effect type for touch event
 */
typedef enum {
    /** A button pressed effect */
    JAVACALL_TACTILE_PRESSED = 0,
    /** An item dragged effect */
    JAVACALL_TACTILE_DRAGGED = 1,
    /** A scroll list flickered effect */
    JAVACALL_TACTILE_FLICKERED = 2
} javacall_tactile_type;

/**
 * Get feedback on touching a control
 * @param type must be one of the types defined in javacall_tactile_type enum
 */
javacall_result javacall_play_tactile_feedback(javacall_tactile_type type);

/** @} */


#ifdef __cplusplus
} //extern "C"
#endif

#endif

