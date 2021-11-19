/*
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


#include "lime.h"

#include <stdio.h>
#include <stdlib.h>

#include "javacall_annunciator.h"
#include "javacall_lcd.h"
#include "local_defs.h"


#include "img/img_lock.h"
#include "img/img_network.h"
#include "img/img_caps.h"
#include "img/img_lowercase.h"
#include "img/img_numeric.h"
#include "img/img_symbols.h"
#include "img/img_T9.h"


javacall_bool first_time = JAVACALL_TRUE;
static int screenWidth = 0;
static int screenHeight = 0;
static javacall_lcd_color_encoding_type colorEncoding;

#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_GRAPHICS_CLASS "GraphicsBridge"
#define LIME_DEVICE_CLASS "DeviceBridge"
#define LIME_MEDIA_CLASS "MediaBridge"

/* WTK input mode types, copied from GraphicsBridge */
typedef enum {
	/* Constant that indicates that input mode is "latin CAPITAL letters" */
    INPUT_MODE_ABC = 1,
	/* Constant that indicates that input mode is "latin lower case letters" */
    INPUT_MODE_abc = 2,
	/* Constant that indicates that input mode is "digits" */
    INPUT_MODE_123 = 3,
	/* Constant that indicates that input mode is "symbols" */
    INPUT_MODE_SYM = 4,
	/* Constant that indicates that input mode is "hira1" */
    INPUT_MODE_HIRA1 = 16,
	/* Constant that indicates that input mode is "hira2" */
    INPUT_MODE_HIRA2 = 32,
	/* Constant that indicates that input mode is "hira3" */
    INPUT_MODE_HIRA3 = 48,
	/* Constant that indicates that input mode is "kana" */
    INPUT_MODE_KANA  = 64
} wtk_input_mode_type;


#define INFINITE_VIB 1000*1000*100 /*Large enough value*/
/**
 * Turn device's Vibrate on/off
 *
 * @param turnVibrateOn if <tt>1</tt>, turn vibrate on, else turn vibrate off
 *
 * @return <tt>JAVACALL_OK</tt> if device supports vibration
 *         <tt>JAVACALL_FAIL</tt> if device does not supports vibration
 *
 */
javacall_result javacall_annunciator_vibrate(javacall_bool onOff) {
    bool_t res;
    int duration = 0;

    static LimeFunction *f = NULL;

    if (onOff == JAVACALL_TRUE) {
        duration = INFINITE_VIB;
    }

    if (f==NULL)
    {
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_MEDIA_CLASS,
                            "playVibrationSound");
    }
	f->call(f, &res, (int) duration);

    if (res)
    {
        return JAVACALL_OK;
    }
    else
    {
        return JAVACALL_FAIL;
    }
}

/**
 * Sets the flashing effect for the device backlight.
 * The flashing effect is intended to be used to attract the
 * user attention or as a special effect for games.
 *
 *
 * @param  turnBacklightToBright <tt>1</tt> to turn backlight to bright mode
 *                               <tt>0</tt> to turn backlight to dim mode
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> on failure, or not supported on device
 *
 */
javacall_result javacall_annunciator_flash_backlight(javacall_bool enableBacklight) {
    static LimeFunction *f = NULL;

    if (f==NULL)
    {
		f = NewLimeFunction(LIME_PACKAGE,
							LIME_DEVICE_CLASS,
							"setBacklight");
    }
    f->call(f, NULL, enableBacklight);
    return JAVACALL_OK;
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
javacall_result javacall_annunciator_display_trusted_icon(javacall_bool enableTrustedIcon) {
    static LimeFunction *f = NULL;

    if (f==NULL)
    {
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_GRAPHICS_CLASS,
                            "drawTrustedIcon");
    }
    f->call(f, NULL, enableTrustedIcon);

    return JAVACALL_OK;
}

/**
 * Turning notification indicator icon off or on, for background MIDlet notifications.
 *
 * @param enableNotificationIcon boolean value specifying whether to display the notification icon
 *
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not
 *         supported on device
 */
javacall_result javacall_annunciator_display_notification_icon(javacall_bool enableNotificationIcon) {
    static LimeFunction *f = NULL;
    if (f==NULL)
    {
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_GRAPHICS_CLASS,
                            "drawNotificationIcon");
    }
    f->call(f, NULL, enableNotificationIcon);

    return JAVACALL_OK;
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
javacall_result javacall_annunciator_display_network_icon(int networkIndicatorCounter) {
    static LimeFunction *f = NULL;
	javacall_bool enable = JAVACALL_FALSE;

    if (f==NULL)
    {
		f = NewLimeFunction(LIME_PACKAGE,
							LIME_DEVICE_CLASS,
							"setNetworkIndicator");
    }

    f->call(f, NULL, networkIndicatorCounter);

	return JAVACALL_OK;

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
javacall_result javacall_annunciator_display_input_mode_icon(javacall_input_mode_type mode) {

	static LimeFunction *f = NULL;
    javacall_input_mode_type type;

    switch (mode) {
    case JAVACALL_INPUT_MODE_LATIN_CAPS:
        type = INPUT_MODE_ABC;
        break;
    case JAVACALL_INPUT_MODE_LATIN_LOWERCASE:
        type = INPUT_MODE_abc;
        break;
    case JAVACALL_INPUT_MODE_NUMERIC:
        type = INPUT_MODE_123;
        break;
    case JAVACALL_INPUT_MODE_SYMBOL:
        type = INPUT_MODE_SYM;
        break;
    case JAVACALL_INPUT_MODE_T9:
        type = INPUT_MODE_abc; // no t9 mode in WTK
        break;
    case JAVACALL_INPUT_MODE_OFF:
        type = INPUT_MODE_abc;
        break;
    default:
        javautil_debug_print (JAVACALL_LOG_ERROR, "annunciator", "Invalid input mode %d.",mode);
        return JAVACALL_INVALID_ARGUMENT;
        break;
    };

    if (f==NULL)
    {
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_GRAPHICS_CLASS,
                            "setInputMode");
    }
    f->call(f, NULL, mode);

    return JAVACALL_OK;
}

/**
 * Play a sound of the given type.
 *
 * @param soundType must be one of the sound types defined
 *                  JAVACALL_AUDIBLE_TONE_INFO         : Sound for informative alert
 *                  JAVACALL_AUDIBLE_TONE_WARNING      : Sound for warning alert
 *                  JAVACALL_AUDIBLE_TONE_ERROR        : Sound for error alert
 *                  JAVACALL_AUDIBLE_TONE_ALARM        : Sound for alarm alert
 *                  JAVACALL_AUDIBLE_TONE_CONFIRMATION : Sound for confirmation alert
 * @return <tt>JAVACALL_OK<tt> if a sound was actually emitted or
 *         </tt>JAVACALL_FAIL</tt> if error occured or device is in mute mode
 */
javacall_result javacall_annunciator_play_audible_tone(javacall_audible_tone_type soundType) {
	 int    retCode;
	 static LimeFunction *f = NULL;

	 if (f==NULL)
	 {
		 f = NewLimeFunction(LIME_PACKAGE,
							 LIME_MEDIA_CLASS,
							 "playAlertSound");
	 }
	 f->call(f, &retCode, soundType);

	 if (retCode == 1) {
		 return JAVACALL_OK;
	 } else {
		 return JAVACALL_FAIL;
	 }
}


/**
 * Controls the secure connection indicator.
 *
 * The secure connection indicator will be displayed when SSL
 * or HTTPS connection is active.
 *
 * @param enableIndicator boolean value indicating if the secure
 * icon should be enabled
 * @return <tt>JAVACALL_OK</tt> operation was supported by the device
 * <tt>JAVACALL_FAIL</tt> or negative value on failure, or if not
 * supported on device
 */
javacall_result javacall_annunciator_display_secure_network_icon(
        javacall_bool enableIndicator) {
    return JAVACALL_OK;
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

//TODO: Add impl
javacall_result javacall_annunciator_display_home_icon(javacall_bool enableHomeIndicator) {
    return JAVACALL_FAIL;
}
