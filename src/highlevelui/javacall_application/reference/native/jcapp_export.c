/*
 *
 *
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

#include <midp_logging.h>
#include <jcapp_export.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>
#include <javacall_lcd.h>
#include <string.h>
#include <kni.h>


gxj_screen_buffer gxj_system_screen_buffer;

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

int jcapp_get_screen_buffer() {
     javacall_lcd_color_encoding_type color_encoding;
     gxj_system_screen_buffer.alphaData = NULL;
     gxj_system_screen_buffer.pixelData = 
         javacall_lcd_get_screen (JAVACALL_LCD_SCREEN_PRIMARY,
                                  &gxj_system_screen_buffer.width,
                                  &gxj_system_screen_buffer.height,
                                  &color_encoding);                                
     if (JAVACALL_LCD_COLOR_RGB565 != color_encoding) {        
	    return -2;
     };                     
}


/**
 * Reset screen buffer content.
 */
void static jcapp_reset_screen_buffer() {
    memset (gxj_system_screen_buffer.pixelData, 0,
        gxj_system_screen_buffer.width * gxj_system_screen_buffer.height *
            sizeof (gxj_pixel_type));
}


/**
 * Initializes the Javacall native resources.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int jcapp_init() {
    javacall_lcd_color_encoding_type color_encoding;
 
    if (!JAVACALL_SUCCEEDED(javacall_lcd_init ()))
        return -1;        
 
    /**
     *   NOTE: Only JAVACALL_LCD_COLOR_RGB565 encoding is supported by phoneME 
     *     implementation. Other values are reserved for future  use. Returning
     *     the buffer in other encoding will result in application termination.
     */
    if (jcapp_get_screen_buffer() == -2) {
        REPORT_ERROR(LC_LOWUI, "Screen pixel format is the one different from RGB565!");
        return -2;
    }    

    jcapp_reset_screen_buffer();
    return 0;
}

/**
 * Finalize the Javacall native resources.
 */
void jcapp_finalize() {
    javacall_lcd_finalize ();
}

/**
 * Bridge function to request a repaint
 * of the area specified.
 *
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void jcapp_refresh(int x1, int y1, int x2, int y2)
{
    javacall_lcd_flush_partial (y1, y2);
}

/**
 * Turn on or off the full screen mode
 *
 * @param mode true for full screen mode
 *             false for normal
 */
void jcapp_set_fullscreen_mode(jboolean mode) {    

    javacall_lcd_set_full_screen_mode(mode);
    jcapp_get_screen_buffer();
    jcapp_reset_screen_buffer();
}

/**
 * Change screen orientation flag
 */
jboolean jcapp_reverse_orientation() {
    jboolean res = javacall_lcd_reverse_orientation(); 
    jcapp_get_screen_buffer();

    // Whether current Displayable won't repaint the entire screen on
    // resize event, the artefacts from the old screen content can appear.
    // That's why the buffer content is not preserved.
    jcapp_reset_screen_buffer();
    return res;
}

/**
 * Get screen orientation flag
 */
jboolean jcapp_get_reverse_orientation() {
    return javacall_lcd_get_reverse_orientation();
}

/**
 * Return screen width
 */
int jcapp_get_screen_width() {
    return javacall_lcd_get_screen_width();   
}

/**
 *  Return screen height
 */
int jcapp_get_screen_height() {
    return javacall_lcd_get_screen_height();
}

/**
 * Checks if soft button layer is supported
 * 
 * @return KNI_TRUE if native softbutton is supported, KNI_FALSE - otherwise
 */
jboolean jcapp_is_native_softbutton_layer_supported() {
    return javacall_lcd_is_native_softbutton_layer_supported();
}

/**
 * Paints the Soft Buttons when using a native layer
 * acts as intermidiate layer between kni and javacall 
 * 
 * @param label Label to draw (UTF16)
 * @param len Length of the lable (0 will cause removal of current label)
 * @param index Index of the soft button in the soft button bar.
 */
 void jcapp_set_softbutton_label_on_native_layer(unsigned short *label, int len,int index) {
	javacall_lcd_set_native_softbutton_label(label, len, index);
}


