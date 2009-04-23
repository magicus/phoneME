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

#include "appManager.h"
#include "appManagerLCDUI.h"

#include <javacall_memory.h>
#include <javacall_lcd.h>


static SBuffer VRAM = {(javacall_pixel*)NULL, 0, 0};


SBuffer* GetLCDUIBuffer() {
    return &VRAM;
}

extern "C" {

/**
 * Initialize LCD API
 * Will be called once the Java platform is launched
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure
 */
javacall_result javacall_lcd_init(void) {
    if (VRAM.hdc == NULL) {
        VRAM.hdc = 
            (javacall_pixel*)javacall_malloc(MAIN_WINDOW_CHILD_AREA_WIDTH *
                                             MAIN_WINDOW_CHILD_AREA_HEIGHT * 
                                             sizeof(javacall_pixel));
        if (VRAM.hdc == NULL) {
            wprintf(
                _T("ERROR: javacall_lcd_init(): VRAM allocation failed!\n"));
            return JAVACALL_FAIL;
        }

        VRAM.width  = MAIN_WINDOW_CHILD_AREA_WIDTH;
        VRAM.height = MAIN_WINDOW_CHILD_AREA_HEIGHT;
    }

    return JAVACALL_OK;
}

/**
 * The function javacall_lcd_finalize is called by during Java VM shutdown,
 * allowing the  * platform to perform device specific lcd-related shutdown
 * operations.
 * The VM guarantees not to call other lcd functions before calling
 * javacall_lcd_init( ) again.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_finalize(void) {
    if (VRAM.hdc != NULL) {
        VRAM.height = 0;
        VRAM.width = 0;
        javacall_free(VRAM.hdc);
        VRAM.hdc = NULL;
    }

    return JAVACALL_OK;
}

/**
 * Get screen raster pointer
 *
 * @param hardwareId id of hardware display
 * @param screenWidth output paramter to hold width of screen
 * @param screenHeight output paramter to hold height of screen
 * @param colorEncoding output paramenter to hold color encoding,
 *        which can take one of the following:
 *              -# JAVACALL_LCD_COLOR_RGB565
 *              -# JAVACALL_LCD_COLOR_ARGB
 *              -# JAVACALL_LCD_COLOR_RGBA
 *              -# JAVACALL_LCD_COLOR_RGB888
 *              -# JAVACALL_LCD_COLOR_OTHER
 *
 * @return pointer to video ram mapped memory region of size
 *         ( screenWidth * screenHeight )
 *         or <code>NULL</code> in case of failure
 */
javacall_pixel* javacall_lcd_get_screen(int hardwareId,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding) {
   (void)hardwareId;
    if (screenWidth != NULL) {
        *screenWidth = VRAM.width;
    }

    if (screenHeight != NULL) {
        *screenHeight = VRAM.height;
    }

    if (colorEncoding != NULL) {
        *colorEncoding = JAVACALL_LCD_COLOR_RGB565;
    }

    return VRAM.hdc;
}

/**
 * Set or unset full screen mode.
 *
 * This function should return <code>JAVACALL_FAIL</code> if full screen mode
 * is not supported.
 * Subsequent calls to <code>javacall_lcd_get_screen()</code> will return
 * a pointer to the relevant offscreen pixel buffer of the corresponding screen
 * mode as well s the corresponding screen dimensions, after the screen mode has
 * changed.
 *
 * @param useFullScreen if <code>JAVACALL_TRUE</code>, turn on full screen mode.
 *                      if <code>JAVACALL_FALSE</code>, use normal screen mode.

 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL failure
 */
javacall_result javacall_lcd_set_full_screen_mode(int hardwareId, javacall_bool useFullScreen) {
    (void)hardwareId;
    return JAVACALL_OK;
}

/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure or invalid screen
 */
javacall_result javacall_lcd_flush(int hardwareId) {
    (void)hardwareId;
    RefreshScreen(0, 0,
                  MAIN_WINDOW_CHILD_AREA_WIDTH,
                  MAIN_WINDOW_CHILD_AREA_HEIGHT);
    return JAVACALL_OK;
}
/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 * The following API uses partial flushing of the VRAM, thus may reduce the
 * runtime of the expensive flush operation: It should be implemented on
 * platforms that support it
 *
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result /*OPTIONAL*/ javacall_lcd_flush_partial(int hardwareId, int ystart, int yend) {
    (void)hardwareId;
    RefreshScreen(0, 0,
                  MAIN_WINDOW_CHILD_AREA_WIDTH,
                  MAIN_WINDOW_CHILD_AREA_HEIGHT); 
    return JAVACALL_OK;
}

/**
 * Changes display orientation
 */
javacall_bool javacall_lcd_reverse_orientation(int hardwareId) {
    (void)hardwareId;
    return JAVACALL_FALSE;
}
/**
 * Handles clamshell event
 */
void  javacall_lcd_handle_clamshell(){
}
 
/**
 * Returns display orientation
 */
javacall_bool javacall_lcd_get_reverse_orientation(int hardwareId) {
    (void)hardwareId;   
    return JAVACALL_FALSE;
}

/**
 * checks the implementation supports native softbutton label.
 * 
 * @retval JAVACALL_TRUE   implementation supports native softbutton layer
 * @retval JAVACALL_FALSE  implementation does not support native softbutton layer
 */
javacall_bool javacall_lcd_is_native_softbutton_layer_supported() {
    return JAVACALL_FALSE;
}


/**
 * The following function is used to set the softbutton label in the native
 * soft button layer.
 * 
 * @param label the label for the softbutton
 * @param len the length of the label
 * @param index the corresponding index of the command
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result
javacall_lcd_set_native_softbutton_label(const javacall_utf16* label,
                                         int len,
                                         int index) {
     return JAVACALL_FAIL;
}

/**
 * Returns available display width
 */
int javacall_lcd_get_screen_width(int hardwareId) {
    (void)hardwareId;
    return MAIN_WINDOW_CHILD_AREA_WIDTH;
}

/**
 * Returns available display height
 */
int javacall_lcd_get_screen_height(int hardwareId) {
    (void)hardwareId; 
    return MAIN_WINDOW_CHILD_AREA_HEIGHT;
}

/**
 * get currently enabled hardware display id
 */
  int javacall_lcd_get_current_hardwareId() {
    return 0;
  }

/**
 * Return display name
 * @param hardwareId unique id of hardware display
 */
char* javacall_lcd_get_display_name(int hardwareId) {
    (void)hardwareId;
    return NULL;
}



/**
 * Check if the display device is primary
 * @param hardwareId unique id of hardware display
 */
javacall_bool javacall_lcd_is_display_primary(int hardwareId) {
    (void)hardwareId;
    return JAVACALL_TRUE;
}

/**
 * Check if the display device is build-in
 */
javacall_bool javacall_lcd_is_display_buildin(int hardwareId) {
    (void)hardwareId;  
    return JAVACALL_TRUE;
}


/**
 * Check if the display device supports pointer events
 */
javacall_bool javacall_lcd_is_display_pen_supported(int hardwareId) {
    (void)hardwareId;
    return JAVACALL_TRUE;
}


/**
 * Check if the display device supports pointer motion  events
 */
javacall_bool javacall_lcd_is_display_pen_motion_supported(int hardwareId) {
    (void)hardwareId;
    return JAVACALL_FALSE;
}

/**
 * Get display device capabilities
 */
int javacall_lcd_get_display_capabilities(int hardwareId) {
    (void)hardwareId;
    return 255;
}

static int screen_ids[] =
{
  0
};

/**
 * Get the list of screen ids
 * @param return number of screens 
 * @return the lit of ids 
 */
int* javacall_lcd_get_display_device_ids(int* n) {
  *n = 1;
    return screen_ids;
}

}; // extern "C"
