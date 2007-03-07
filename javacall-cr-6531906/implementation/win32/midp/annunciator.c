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

/**
 * @file
 *
 *
 */

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

void util_lcd_init(void) {

    if (first_time) {
        javacall_lcd_get_screen(JAVACALL_LCD_SCREEN_PRIMARY,
                                &screenWidth, &screenHeight, NULL);
    }
    javacall_lcd_flush();
    first_time = JAVACALL_FALSE;

}

void util_clear_screen(unsigned short *scrn, int screenSize) {

    int index;

    for (index = 0; index < screenSize; index++) {
        scrn[index] = RGB2PIXELTYPE(255, 255, 255);
    }
}

void util_draw_bitmap(unsigned char *bitmap_rgb,
                      int bitmap_width,
                      int bitmap_height,
                      int screen_top,
                      int screen_left) {

    int x, y;
    int bitmap_horizontal_gap;
    unsigned negy, negx;
    javacall_pixel *scrn;
    int startx, starty;

    scrn = javacall_lcd_get_screen(JAVACALL_LCD_SCREEN_PRIMARY,
                                   &screenWidth, &screenHeight, &colorEncoding);
    ////
    startx = screen_left < 0 ? 0 : screen_left;
    starty = screen_top < 0 ? 0 : screen_top;

    if ((startx >= screenWidth) || (starty >= screenHeight)) {
        return;
    }

    negy = (screen_top < 0) ? 0 - screen_top : 0;
    negx = (screen_left < 0) ? 0 - screen_left : 0;

    bitmap_width = bitmap_width - negx;
    bitmap_height = bitmap_height - negy;
    bitmap_horizontal_gap = 0;

    if (startx + bitmap_width > screenWidth) {
        bitmap_horizontal_gap = 3 * (bitmap_width - (screenWidth - startx));
        bitmap_width = screenWidth - startx;
    }
    if (starty + bitmap_height > screenHeight) {
        bitmap_height = screenHeight - starty;
    }

    //srcraster=src->pixels+negy*src->x+negx;
    //scrn=scrn+starty*screenWidth+startx;
    bitmap_rgb = bitmap_rgb + negy * screenWidth + negx;
    /////

    for (y = screen_top; y < screen_top + bitmap_height; y++) {
        for (x = screen_left; x < screen_left + bitmap_width; x++) {
            if ((y >= 0) && (x >= 0) && (y < screenHeight) && (x < screenWidth)) {
                unsigned short r = *bitmap_rgb++;
                unsigned short g = *bitmap_rgb++;
                unsigned short b = *bitmap_rgb++;

                scrn[(y * screenWidth) + x] = RGB2PIXELTYPE(r, g, b);
                //scrn[(y*screenWidth)+x]= tmp;
            }
        }
        bitmap_rgb += bitmap_horizontal_gap;
    }
}


/**
 * Turn device's Vibrate on/off
 *
 * @param turnVibrateOn if <tt>1</tt>, turn vibrate on, else turn vibrate off
 *
 * @return <tt>JAVACALL_OK</tt> if device supports vibration
 *         <tt>JAVACALL_FAIL</tt> if device does not supports vibration
 *
 */
javacall_result javacall_annunciator_vibrate(javacall_bool enableVibrate) {

    sprintf(print_buffer, "Vibrate %s\n",
            enableVibrate ? "Enabled" : "Disabled");
    javacall_print(print_buffer);
    return JAVACALL_FAIL;

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

    sprintf(print_buffer, "Backlight %s\n", enableBacklight ? "Enabled" : "Disabled");
    javacall_print(print_buffer);
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
javacall_result javacall_annunciator_display_trusted_icon(javacall_bool enableTrustedIcon) {

#if 0
    javacall_pixel *scrn;

    scrn = javacall_lcd_get_screen(JAVACALL_LCD_SCREEN_PRIMARY,
                                   &screenWidth, &screenHeight, &colorEncoding);

    if (enableTrustedIcon) {
        util_draw_bitmap(lock_data, lock_width, lock_height, 0, 0);
        javacall_lcd_flush();

    } else {
        util_clear_screen(scrn, (screenWidth * screenHeight));
        javacall_lcd_flush();
    }
#endif

    sprintf(print_buffer, "Trusted Icon %s\n",
            enableTrustedIcon ? "Enabled" : "Disabled");
    javacall_print(print_buffer);

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
javacall_result javacall_annunciator_display_network_icon(javacall_bool enableNetworkIndicator) {

#if 0
    javacall_pixel *scrn;
    javacall_bool indicator_on;

    //javacall_print(print_buffer);  // Much info on screen

    indicator_on = JAVACALL_FALSE;
    util_lcd_init();
    scrn = javacall_lcd_get_screen(JAVACALL_LCD_SCREEN_PRIMARY,
                                   &screenWidth, &screenHeight, &colorEncoding);
    if (enableNetworkIndicator) {
        indicator_on = JAVACALL_TRUE;

        util_draw_bitmap(network_data, network_width, network_height, 0, 0);
        javacall_lcd_flush();

    } else {
        indicator_on = JAVACALL_FALSE;
        util_clear_screen(scrn, (screenWidth * screenHeight));
        javacall_lcd_flush();
    }
#endif

    sprintf(print_buffer, "Network Icon %s\n",
            enableNetworkIndicator ? "Enabled" : "Disabled");
    javacall_print (print_buffer);

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
    
    static char input_mode_types[][20] = {
      "LATIN_CAPS",
      "LATIN_LOWERCASE",
      "NUMERIC",
      "SYMBOL",
      "T9",
      "OFF"
    };
    char *type;

    switch (mode) {
    case JAVACALL_INPUT_MODE_LATIN_CAPS:
        type = input_mode_types[0];
        break;
    case JAVACALL_INPUT_MODE_LATIN_LOWERCASE:
        type = input_mode_types[1];
        break;
    case JAVACALL_INPUT_MODE_NUMERIC:
        type = input_mode_types[2];
        break;
    case JAVACALL_INPUT_MODE_SYMBOL:
        type = input_mode_types[3];
        break;
    case JAVACALL_INPUT_MODE_T9:
        type = input_mode_types[4];
        break;
    case JAVACALL_INPUT_MODE_OFF:
        type = input_mode_types[5];
        break;
    default:
        javacall_print ("Invalid input mode ");
        javacall_print (itoa(mode, print_buffer, 10));
	javacall_print (".\n");
        return JAVACALL_INVALID_ARGUMENT;
        break;
    };

    javacall_print ("Setting input mode to ");
    javacall_print (type);
    javacall_print (".\n");

    return JAVACALL_OK;

#if 0
    javacall_pixel *scrn;
    javacall_bool indicator_on;

    sprintf(print_buffer, "Setting input mode to %d\n", mode);
    javacall_print(print_buffer);

    indicator_on = JAVACALL_FALSE;
    util_lcd_init();
    scrn = javacall_lcd_get_screen(JAVACALL_LCD_SCREEN_PRIMARY,
                                   &screenWidth, &screenHeight, &colorEncoding);

    switch (mode) {
    case JAVACALL_INPUT_MODE_LATIN_CAPS:
        util_clear_screen(scrn, (screenWidth * screenHeight));
        util_draw_bitmap(caps_data, caps_width, caps_height, 0, 0);
        javacall_lcd_flush();
        break;
    case JAVACALL_INPUT_MODE_LATIN_LOWERCASE:
        util_clear_screen(scrn, (screenWidth * screenHeight));
        util_draw_bitmap(lowCase_data, lowCase_width, lowCase_height, 0, 0);
        javacall_lcd_flush();
        break;
    case JAVACALL_INPUT_MODE_NUMERIC:
        util_clear_screen(scrn, (screenWidth * screenHeight));
        util_draw_bitmap(numeric_data, numeric_width, numeric_height, 0, 0);
        javacall_lcd_flush();
        break;
    case JAVACALL_INPUT_MODE_SYMBOL:
        util_clear_screen(scrn, (screenWidth * screenHeight));
        util_draw_bitmap(symbol_data, symbol_width, symbol_height, 0, 0);
        javacall_lcd_flush();
        break;
    case JAVACALL_INPUT_MODE_T9:
        util_clear_screen(scrn, (screenWidth * screenHeight));
        util_draw_bitmap(t9_data, t9_width, t9_height, 0, 0);
        javacall_lcd_flush();
        break;
    default:
        break;
    }

#endif
    return JAVACALL_NOT_IMPLEMENTED;
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

    switch (soundType) {
    case JAVACALL_AUDIBLE_TONE_INFO:
        sprintf(print_buffer, "now playing INFO\n");
        javacall_print(print_buffer);
        break;
    case JAVACALL_AUDIBLE_TONE_WARNING:
        sprintf(print_buffer, "now playing WARNING\n");
        javacall_print(print_buffer);
        break;
    case JAVACALL_AUDIBLE_TONE_ERROR:
        sprintf(print_buffer, "now playing ERROR\n");
        javacall_print(print_buffer);
        break;
    case JAVACALL_AUDIBLE_TONE_ALARM:
        sprintf(print_buffer, "now playing ALARM\n");
        javacall_print(print_buffer);
        break;
    case JAVACALL_AUDIBLE_TONE_CONFIRMATION:
        sprintf(print_buffer, "now playing CONFIRMATION\n");
        javacall_print(print_buffer);
        break;
    default:
        sprintf(print_buffer, "ERROR: INVALID SOUND TYPE\n");
        javacall_print(print_buffer);
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
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
