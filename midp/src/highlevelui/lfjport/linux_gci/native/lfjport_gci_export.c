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

#include <lfjport_export.h>


/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

/**
 * Initializes the lfjport_ui_ native resources.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int lfjport_ui_init() {
    return 1;
}

/**
 * Finalize the lfjport_ui_ native resources.
 */
void lfjport_ui_finalize() {
    
}

/**
 * Bridge function to request a repaint 
 * of the area specified.
 *
 * @param hardwareId hardware ID of the display device
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void lfjport_refresh(int hardwareId, int x1, int y1, int x2, int y2)
{
    (void)hardwareId;
    (int)x1, y1, x2, y2;
}

/**
 * Porting API function to update scroll bar.
 *
 * @param scrollPosition current scroll position
 * @param scrollProportion maximum scroll position
 * @return status of this call
 */
int lfjport_set_vertical_scroll(int scrollPosition, int scrollProportion)
{

    /* Suppress unused parameter warnings */
    (void)scrollPosition;
    (void)scrollProportion;

    return 0;
}


/**
 * Turn on or off the full screen mode
 *
 * @param hardwareId hardware ID of the display device
 * @param mode true for full screen mode
 *             false for normal
 */
void lfjport_set_fullscreen_mode(int hardwareId, jboolean mode) {
    (void)hardwareId;
    (void)mode;
    return;
}

/**
 * Resets native resources when foreground is gained by a new display.
 * @param hardwareId hardware ID of the display device
 */
void lfjport_gained_foreground(int hardwareId) {
    (void)hardwareId;
}

/**
 * Change screen orientation flag
 * @param hardwareId hardware ID of the display device
 */
jboolean lfjport_reverse_orientation(int hardwareId) {
    (void)hardwareId;
    return 0;
}

/**
 * Handle clamshell event
 */
void lfjport_handle_clamshell_event() {
}

/**
 * Bridge function to get screen orientation flag
 * @param hardwareId hardware ID of the display device
 */
jboolean lfjport_get_reverse_orientation(int hardwareId)
{
    (void)hardwareId;
    return 0;
}

/**
 * Return screen width
 * @param hardwareId hardware ID of the display device
 */
int lfjport_get_screen_width(int hardwareId) {
  /* TODO need to be ported */
    (void)hardwareId;
    return 176;
}

/**
 *  Return screen height
 * @param hardwareId hardware ID of the display device
 */
int lfjport_get_screen_height(int hardwareId) {
  /* TODO need to be ported */
    (void)hardwareId;
    return 210;
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
 *
 * @param hardwareId hardware ID of the display device
 * @param h The height to be flushed
 * @return KNI_TRUE if direct_flush was successful, KNI_FALSE - otherwise
 */
jboolean lfjport_direct_flush(int hardwareId, const java_graphics *g,
		  	      const java_imagedata *offscreen_buffer, int h) {
    (void)hardwareId;
   return KNI_FALSE;
}

/**
 * Check if native softbutton is supported on platform
 * 
 * @return KNI_TRUE if native softbutton is supported, KNI_FALSE - otherwise
 */
jboolean lfjport_is_native_softbutton_layer_supported() {
    return KNI_FALSE;
}

/**
 * Request platform to draw a label in the soft button layer.
 * 
 * @param label Label to draw (UTF16)
 * @param len Length of the lable (0 will cause removal of current label)
 * @param index Index of the soft button in the soft button bar.
 */
void lfjport_set_softbutton_label_on_native_layer (unsigned short *label, 
                                                 int len, 
                                                 int index) {
    (void)label;
    (void)len;
    (void)index;
    // Not implemented
}

/**
 * get currently enabled hardware display id
 */
int lfjport_get_current_hardwareId() {
    return  0;  // just one display  is supported
}

/**
 * Get display device name by id
 */
char * lfjport_get_display_name(int hardwareId) {
    (void)hardwareId;
    // Not implemented
    return 0;
}


/**
 * Check if the display device is primary
 */
jboolean lfjport_is_display_primary(int hardwareId) {
    (void)hardwareId;
    // Not implemented
    return KNI_TRUE;
}
/**
 * Check if the display device is build-in
 */
jboolean lfjport_is_display_buildin(int hardwareId) {
    (void)hardwareId;
    // Not implemented
    return KNI_TRUE;
}
/**
 * Check if the display device supports pointer events
 */
jboolean lfjport_is_display_pen_supported(int hardwareId) {
    (void)hardwareId;
    // Not implemented
    return KNI_FALSE;
}
/**
 * Check if the display device supports pointer motion  events
 */
jboolean lfjport_is_display_pen_motion_supported(int hardwareId) {
    (void)hardwareId;
    // Not implemented
    return KNI_FALSE;
}
/**
 * Get display device capabilities
 */
int lfjport_get_display_capabilities(int hardwareId) {
    (void)hardwareId;
    // Not implemented
    return 0;
}


static jint display_device_ids[] = {0};

/**
 * Get the list of display device ids
 */

jint* lfjport_get_display_device_ids(jint* n) {
    *n = 1;
    return display_device_ids;
}

/**
 * Notify the display device state has been changed
 */
void lfjport_display_device_state_changed(int hardwareId, int state) {
  (void)hardwareId;
  (void)state;
}
