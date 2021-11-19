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

/**
 * @file
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "javacall_annunciator.h"
#include "javacall_lcd.h"

#include "local_defs.h"

#include "img/topbar.h"
#include "img/img_network.h"
#include "img/img_home.h"
#include "img/img_trusted.h"

static javacall_bool trustedOn = JAVACALL_FALSE;
static javacall_bool networkOn = JAVACALL_FALSE;
static javacall_bool homeOn = JAVACALL_FALSE;

#if 1
extern javacall_pixel* getTopbarBuffer(int* screenWidth, int* screenHeight);
#else
javacall_pixel* getTopbarBuffer(int* screenWidth, int* screenHeight) {
    return NULL;
}
#endif

/**
 * Premultiply color components by it's corresponding alpha component.
 *
 * Formula: Cs = Csr * As (for source pixel),
 *          Cd = Cdr * Ad (analog for destination pixel).
 *
 * @param C one of the raw color components of the pixel (Csr or Cdr in the formula).
 * @param A the alpha component of the source pixel (As or Ad in the formula).
 * @return color component in premultiplied form.
 */
#define PREMULTUPLY_ALPHA(C, A) \
    (unsigned char)( (int)(C) * (A) / 0xff )


/**
 * The source is composited over the destination (Porter-Duff Source Over
 * Destination rule).
 *
 * Formula: Cr = Cs + Cd*(1-As)
 *
 * Note: the result is always equal or less than 0xff, i.e. overflow is impossible.
 *
 * @param Cs a color component of the source pixel in premultiplied form
 * @param As the alpha component of the source pixel
 * @param Cd a color component of the destination pixel in premultiplied form
 * @return a color component of the result in premultiplied form
 */
#define ADD_PREMULTIPLIEDCOLORS_SRCOVER(Cs, As, Cd) \
    (unsigned char)( (int)(Cs) + (int)(Cd) * (0xff - (As)) / 0xff )

    
/**
 * Combine separate source and destination color components.
 *
 * Note: all backround pixels are treated as full opaque.
 *
 * @param Csr one of the raw color components of the source pixel
 * @param As the alpha component of the source pixel
 * @param Cdr one of the raw color components of the destination pixel
 * @return a color component of the result in premultiplied form
 */
#define ADD_COLORS(Csr, As, Cdr) \
    ADD_PREMULTIPLIEDCOLORS_SRCOVER( \
            PREMULTUPLY_ALPHA(Csr, As), \
            As, \
            PREMULTUPLY_ALPHA(Cdr, 0xff) )


/**
 * Draws image from raw data array to top bar offscreen buffer
 */
void util_draw_bitmap(const unsigned char *bitmap_data, 
                      javacall_lcd_color_encoding_type color_format,
                      int bitmap_width,
                      int bitmap_height,
                      int screen_top,
                      int screen_left) {

    int x, y;
    int bitmap_horizontal_gap;
    unsigned negy, negx;
    javacall_pixel *scrn;
    int startx, starty;
    int screenWidth = 0;
    int screenHeight = 0;

    unsigned char bytesPerPix = 3;

    if (color_format == JAVACALL_LCD_COLOR_RGB888) {
        bytesPerPix = 3;
    } else if (color_format == JAVACALL_LCD_COLOR_RGBA) {
        bytesPerPix = 4;
    } else {
        return;
    }

    scrn = getTopbarBuffer(&screenWidth, &screenHeight);
    if (NULL == scrn) {
        return;
    }

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
        bitmap_horizontal_gap = bytesPerPix * (bitmap_width - (screenWidth - startx));
        bitmap_width = screenWidth - startx;
    }
    if (starty + bitmap_height > screenHeight) {
        bitmap_height = screenHeight - starty;
    }

    bitmap_data = bitmap_data + negy * screenWidth * bytesPerPix + negx * bytesPerPix;

    for (y = screen_top; y < screen_top + bitmap_height; y++) {
        for (x = screen_left; x < screen_left + bitmap_width; x++) {
            if ((y >= 0) && (x >= 0) && (y < screenHeight) && (x < screenWidth)) {
                unsigned char r = *bitmap_data++;
                unsigned char g = *bitmap_data++;
                unsigned char b = *bitmap_data++;

                if (color_format == JAVACALL_LCD_COLOR_RGBA) {
                  javacall_pixel pix = scrn[(y * screenWidth) + x];
                  unsigned char rd = GET_RED_FROM_PIXEL(pix);
                  unsigned char gd = GET_GREEN_FROM_PIXEL(pix);
                  unsigned char bd = GET_BLUE_FROM_PIXEL(pix);
                  unsigned char alpha = *bitmap_data++;

                  r = ADD_COLORS(r, alpha, rd);
                  g = ADD_COLORS(g, alpha, gd);
                  b = ADD_COLORS(b, alpha, bd);
                }                
                scrn[(y * screenWidth) + x] = RGB2PIXELTYPE(r, g, b);
            }
        }
        bitmap_data += bitmap_horizontal_gap;
    }

}

/**
 * Redraws top bar 
 */
void drawTopbarImage(void) {

    util_draw_bitmap((unsigned char*)topbar_data, topbar_color_format, topBarWidth, topBarHeight, 0, 0);

    if (networkOn) {
        util_draw_bitmap(network_data, network_color_format, network_width, network_height, 0, network_x);
    }

    if (trustedOn) {
        util_draw_bitmap(trusted_data, trusted_color_format, trusted_width, trusted_height, 0, trusted_x);
    }

    if (homeOn) {
        util_draw_bitmap(home_data, home_color_format, home_width, home_height, 0, home_x);
    }

    javacall_lcd_flush(javacall_lcd_get_current_hardwareId());
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

    trustedOn = enableTrustedIcon;

    drawTopbarImage();

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

    networkOn = enableNetworkIndicator;

    drawTopbarImage();

    return JAVACALL_OK;
}

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
javacall_result javacall_annunciator_display_input_mode_icon(javacall_input_mode_type mode) {
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
    homeOn = enableHomeIndicator;

    drawTopbarImage();

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

