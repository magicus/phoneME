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

/**
 * @file
 * @defgroup LCDUI Look and Feel 
 * @ingroup subsystems
 *
 * @brief LCDUI Look and Feel exported native interface
 */

#include <lcdlf_export.h>
#include <lfjport_export.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Refresh the given area.  For double buffering purposes.
 */
void lcdlf_refresh(int hardwareId, int x, int y, int w, int h) {
  lfjport_refresh(hardwareId, x, y, w, h);
}

/**
 * Change screen orientation flag
 */
jboolean lcdlf_reverse_orientation(int hardwareId) {
  return lfjport_reverse_orientation(hardwareId);
}

/**
* Handle clamshell event
*/
void lcdlf_handle_clamshell_event() {
    lfjport_handle_clamshell_event();
}

/**
 * Get screen orientation flag
 */
jboolean lcdlf_get_reverse_orientation(int hardwareId) {
  return lfjport_get_reverse_orientation(hardwareId);
}

/**
 * Return screen width
 */
int lcdlf_get_screen_width(int hardwareId) {
  return lfjport_get_screen_width(hardwareId);
}

/**
 *  Return screen height
 */
int lcdlf_get_screen_height(int hardwareId) {
  return lfjport_get_screen_height(hardwareId);
}

/**
 * set the screen mode either to fullscreen or normal.
 *
 * @param mode The screen mode
 */
void lcdlf_set_fullscreen_mode(int hardwareId, jboolean mode) {
  lfjport_set_fullscreen_mode(hardwareId, mode);
}

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lcdlf_gained_foreground(int hardwareId) {
  lfjport_gained_foreground(hardwareId);
}

/**
 * Initializes the window system.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int lcdlf_ui_init() {
  return lfjport_ui_init();
}

/**
 * Finalize the window system.
 */
void lcdlf_ui_finalize() {
  lfjport_ui_finalize();
}

/**
 * Flushes the offscreen buffer directly to the device screen.
 * The size of the buffer flushed is defined by offscreen buffer width
 * and passed in height. 
 * Offscreen_buffer must be aligned to the top-left of the screen and
 * its width must be the same as the device screen width.
 * @param graphics The Graphics handle associated with the screen.
 * @param offscreen_buffer The ImageData handle associated with 
 *                         the offscreen buffer to be flushed
 * @param height The height to be flushed
 * @return KNI_TRUE if direct_flush was successful, KNI_FALSE - otherwise
 */
jboolean lcdlf_direct_flush(int hardwareId, const java_graphics *g, 
			    const java_imagedata *offscreen_buffer, int h) {
  return lfjport_direct_flush(hardwareId, g, offscreen_buffer, h);
}

void
initMenus() {
  /* Do nothing. No native menus in Chameleon. */
}

/**
 * Check if native softbutton is supported on platform
 * 
 * @return KNI_TRUE if native softbutton is supported, KNI_FALSE - otherwise
 */
jboolean lcdlf_is_native_softbutton_layer_supported() {
    return lfjport_is_native_softbutton_layer_supported();
}

/**
 * Request platform to draw a label in the soft button layer.
 * 
 * @param label Label to draw (UTF16)
 * @param len Length of the lable (0 will cause removal of current label)
 * @param index Index of the soft button in the soft button bar.
 */
void lcdlf_set_softbutton_label_on_native_layer (unsigned short *label, 
                                                 int len, 
                                                 int index) {
    lfjport_set_softbutton_label_on_native_layer (label, 
                                                  len, 
                                                  index);
}

/**
 * get currently enabled hardware display id
 */
int lcdlf_get_current_hardwareId() {
    return lfjport_get_current_hardwareId();
}


/** 
 * Get display device name by id
 */
char * lcdlf_get_display_name(int hardwareId) {
    return lfjport_get_display_name(hardwareId);
}

/**
 * Check if the display device is primary
 */
jboolean lcdlf_is_display_primary(int hardwareId) {
    return lfjport_is_display_primary(hardwareId);
}

/**
 * Check if the display device is build-in
 */
jboolean lcdlf_is_display_buildin(int hardwareId) {
    return lfjport_is_display_buildin(hardwareId);
}

/**
 * Check if the display device supports pointer events
 */
jboolean lcdlf_is_display_pen_supported(int hardwareId) {
    return lfjport_is_display_pen_supported(hardwareId);
}


/**
 * Check if the display device supports pointer motion  events
 */
jboolean lcdlf_is_display_pen_motion_supported(int hardwareId) {
  return lfjport_is_display_pen_motion_supported(hardwareId);
}
/**
 * Get display device capabilities
 */
int lcdlf_get_display_capabilities(int hardwareId) {
    return lfjport_get_display_capabilities(hardwareId);
}

/**
 * Get the list of display device ids
 */
jint* lcdlf_get_display_device_ids(jint* n) {
    return lfjport_get_display_device_ids(n);
}
/**
* Notify that display device state has ben changed
*/
void lcdlf_display_device_state_changed(int hardwareId, int state) {
    lfjport_display_device_state_changed(hardwareId, state);
}
#ifdef __cplusplus
}
#endif
