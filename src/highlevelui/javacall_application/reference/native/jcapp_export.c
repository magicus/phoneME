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

#include <midp_logging.h>
#include <jcapp_export.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>
#include <javacall_lcd.h>
#include <string.h>
#include <kni.h>


gxj_screen_buffer* gxj_system_screen_buffers;
jint* ids;
int num_of_screens;

gxj_screen_buffer* get_screen_by_id(int hardwareId) {
  int i;
  if ((ids == NULL) || gxj_system_screen_buffers == NULL) return NULL;
  for(i = 0; i < num_of_screens; i++) {
    if (ids[i] == hardwareId) {
      return &gxj_system_screen_buffers[i];
    }
  } 

}

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 * @param hardwareId unique id of hardware display
 */

int jcapp_get_screen_buffer(int hardwareId) {
     javacall_lcd_color_encoding_type color_encoding;
     gxj_screen_buffer* buff = get_screen_by_id(hardwareId);
     buff->alphaData = NULL;
     buff->pixelData = 
         javacall_lcd_get_screen (hardwareId,
                                  &buff->width,
                                  &buff->height,
                                  &color_encoding);                                
     if (JAVACALL_LCD_COLOR_RGB565 != color_encoding) {        
	    return -2;
     };

     return 0;
}


/**
 * Reset screen buffer content.
 * @param hardwareId unique id of hardware display
 */
void static jcapp_reset_screen_buffer(int hardwareId) {
  gxj_screen_buffer* buff = get_screen_by_id(hardwareId);
  memset (buff->pixelData, 0,
	  buff->width * buff->height * sizeof (gxj_pixel_type));
}


/**
 * Initializes the Javacall native resources.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int jcapp_init() {
    int i;
    int size;
    javacall_lcd_color_encoding_type color_encoding;
 
    if (!JAVACALL_SUCCEEDED(javacall_lcd_init ()))
        return -1;        
 
    
    ids = jcapp_get_display_device_ids(&num_of_screens);

    size = sizeof(gxj_screen_buffer) * num_of_screens;
    system_screens = (gxj_screen_buffer*)midpMalloc(size);
    if (system_screens != NULL) {
        memset(system_screens, 0, size);
    } 

    for (i = 0; i < num_of_screens; i++) {
      
      /**
       *   NOTE: Only JAVACALL_LCD_COLOR_RGB565 encoding is supported by phoneME 
       *     implementation. Other values are reserved for future  use. Returning
       *     the buffer in other encoding will result in application termination.
       */
      if (jcapp_get_screen_buffer(ids[i]) == -2) {
        REPORT_ERROR(LC_LOWUI, "Screen pixel format is the one different from RGB565!");
        return -2;
      }    
      
      jcapp_reset_screen_buffer(ids[i]);
    }
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
 * @param hardwareId unique id of hardware display
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void jcapp_refresh(int hardwareId, int x1, int y1, int x2, int y2)
{
    javacall_lcd_flush_partial (hardwareId, y1, y2);
}

/**
 * Turn on or off the full screen mode
 *
 * @param hardwareId unique id of hardware display
 * @param mode true for full screen mode
 *             false for normal
 */
void jcapp_set_fullscreen_mode(int hardwareId, jboolean mode) {    

    javacall_lcd_set_full_screen_mode(hardwareId, mode);
    jcapp_get_screen_buffer(hardwareId);
    jcapp_reset_screen_buffer(hardwareId);
}

/**
 * Change screen orientation flag
 * @param hardwareId unique id of hardware display
 */
jboolean jcapp_reverse_orientation(int hardwareId) {
    jboolean res = javacall_lcd_reverse_orientation(hardwareId); 
    jcapp_get_screen_buffer(hardwareId);

    // Whether current Displayable won't repaint the entire screen on
    // resize event, the artefacts from the old screen content can appear.
    // That's why the buffer content is not preserved.
    jcapp_reset_screen_buffer(hardwareId);
    return res;
}

/**
 * Get screen orientation flag
 * @param hardwareId unique id of hardware display
 */
jboolean jcapp_get_reverse_orientation(int hardwareId) {
    return javacall_lcd_get_reverse_orientation(hardwareId);
}

/**
 * Return screen width
 * @param hardwareId unique id of hardware display
 */
int jcapp_get_screen_width(int hardwareId) {
    return javacall_lcd_get_screen_width(hardwareId);   
}

/**
 *  Return screen height
 * @param hardwareId unique id of hardware display
 */
int jcapp_get_screen_height(int hardwareId) {
    return javacall_lcd_get_screen_height(hardwareId);
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


/** 
 * Get display device name by id
 */
char* jcapp_get_display_name(int hardwareId) {
    return javacall_lcd_get_display_name(hardwareId);
}


/**
 * Check if the display device is primary
 */
jboolean jcapp_is_display_primary(int hardwareId) {
    return javacall_lcd_is_display_primary(hardwareId);
}

/**
 * Check if the display device is build-in
 */
jboolean jcapp_is_display_buildin(int hardwareId) {
    return javacall_lcd_is_display_buildin(hardwareId);
}

/**
 * Check if the display device supports pointer events
 */
jboolean jcapp_is_display_ptr_supported(int hardwareId) {
    return javacall_lcd_is_display_ptr_supported(hardwareId);
}

/**
 * Check if the display device supports pointer motion  events
 */
jboolean jcapp_is_display_ptr_motion_supported(int hardwareId){
    return javacall_lcd_is_display_ptr_motion_supported(hardwareId);
}

/**
 * Get display device capabilities
 */
int jcapp_get_display_capabilities(int hardwareId) {
  return javacall_lcd_get_display_capabilities(hardwareId);
}

jint* jcapp_get_display_device_ids(jint* n) {
    return javacall_lcd_get_display_device_ids(n);
}
