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
#ifndef __JAVACALL_LCD_H
#define __JAVACALL_LCD_H

/**
 * @file javacall_lcd.h
 * @ingroup LCD
 * @brief Javacall interfaces for LCD
 */

#include "javacall_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup LCD LCD API
 * @ingroup JTWI
 *
 * The mandatory API functions are the following : 
 * - LCD initialization and termination functions
 * - Function for getting a pointer to LCD off-screen raster 
 * - Function for flushing LCD off-screen raster to LCD display
 *
 * @{
 */

/** @defgroup MandatoryLCD Mandatory LCD API
 *  @ingroup LCD
 *
 * @{
 */

/**
 * @def RGB2PIXELTYPE
 *
 * Conversion between 3 RGB octets to a pixel type is defined for the following
 * #define: For example RGB conversion of RGB to 16bpp can be of the form 6-5-6
 */

#ifndef RGB2PIXELTYPE
#define RGB2PIXELTYPE(r,g,b)  (( ((javacall_pixel)b)>>3)&0x1f) | ( (( ((javacall_pixel)g)>>2)&0x3f) << 5) | ((( ((javacall_pixel)r)>>3)&0x1f)<<11)
#endif

/**
 * @enum javacall_lcd_color_encoding_type
 * @brief Color encoding format
 */
typedef enum {
    /** RGB565 color format */
    JAVACALL_LCD_COLOR_RGB565 =200,
    /** ARGB (Alpha + RGB) color format */
    JAVACALL_LCD_COLOR_ARGB   =201,
    /** RGB888 color format */
    JAVACALL_LCD_COLOR_RGB888 =202,
    /** Other color format */
    JAVACALL_LCD_COLOR_OTHER  =199
}javacall_lcd_color_encoding_type;

/**
 * @enum javacall_lcd_screen_type
 * @brief LCD screen type
 */
typedef enum {
    /** Primary(usually internal) LCD type */
    JAVACALL_LCD_SCREEN_PRIMARY = 1600,
    /** External LCD type */
    JAVACALL_LCD_SCREEN_EXTERNAL = 1601
} javacall_lcd_screen_type;

/**
 * The function javacall_lcd_init is called during Java VM startup, allowing the
 * platform to perform device specific initializations.
 *
 * Once this API call is invoked, the VM will receive display focus.\n
 * <b>Note:</b> Consider the case where the platform tries to assume control
 * over the display while the VM is running by pausing the Java platform.
 * In this case, the platform is required to save the VRAM screen buffer:
 * Whenever Java is resumed, the stored screen buffers must be restored to
 * original state.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_init(void);

/**
 * The function javacall_lcd_finalize is called during Java VM shutdown,
 * allowing the platform to perform device specific lcd-related shutdown
 * operations.
 * The VM guarantees not to call other lcd functions before calling
 * javacall_lcd_init( ) again.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_finalize(void);

/**
 * This function is used to query the supported screen capabilities
 * - Display Width
 * - Display Height
 * - Color encoding: Either 32bit ARGB format, 15 bit 565 encoding
 *                   or 24 bit RGB encoding
 *
 * This function is also used to get the screen raster pointer.
 * Subsequent calls to this function are required to return the same
 * address for a given LCD screen type.
 *
 * If full screen mode is supported, the function should return the
 * screen size of the curently active mode, as set by the function
 * javacall_lcd_set_full_screen_mode().
 * In case the screen switch to/from full screen mode, this function must
 * return the a pointer to the top-left pixel of the screen in the
 * corresponding mode.
 *
 * @param screenType can be any of the following types:
 *    - JAVACALL_LCD_SCREEN_PRIMARY :
 *      return primary screen size information
 *    - JAVACALL_LCD_SCREEN_EXTERNAL :
 *      return external screen size information if supported
 * @param screenWidth output paramter to hold width of screen
 * @param screenHeight output paramter to hold height of screen
 * @param colorEncoding output paramenter to hold color encoding,
 *        which can take one of the following:
 *    - JAVACALL_LCD_COLOR_RGB565
 *    - JAVACALL_LCD_COLOR_ARGB (reserved)
 *    - JAVACALL_LCD_COLOR_RGB888 (reserved)
 *    - JAVACALL_LCD_COLOR_OTHER (reserved)
 *   NOTE: As with phoneME Feature MR2 and earlier only JAVACALL_LCD_COLOR_RGB565
 *     encoding is supported by the implementation. Other values are reserved for
 *     future use. Returning the buffer in other encoding might result in erroneus
 *     behaviour or termination of phoneME Feature software application.
 *
 * @return pointer to video ram mapped memory region of size
 *         ( screenWidth * screenHeight )
 *         or <code>NULL</code> in case of failure
 */
javacall_pixel* javacall_lcd_get_screen(javacall_lcd_screen_type screenType,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding);
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
javacall_result javacall_lcd_set_full_screen_mode(javacall_bool useFullScreen);

/**
 * The following function is used to flush the image from the Video RAM raster
 * to the LCD display. \n
 * The function call should not be CPU time expensive, and should return
 * immediately. It should avoid memory bulk copying of the entire raster.
 *
 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL fail
 */
javacall_result javacall_lcd_flush(void);

/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 * The following API uses partial flushing of the VRAM, thus may reduce the
 * runtime of the expensive flush operation.
 *
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_flush_partial(int ystart, int yend);

/**
  * Reverse flag of rotation
  */
javacall_bool javacall_lcd_reverse_orientation();
 
/**
  * Get flag of rotation
  */
javacall_bool javacall_lcd_get_reverse_orientation();
  
/**
  * Return width of screen
  */
int javacall_lcd_get_screen_width();
 
/**
  * Return height of screen
  */
int javacall_lcd_get_screen_height();

/** @} */


/** @} */

#ifdef __cplusplus
} //extern "C"
#endif

#endif
