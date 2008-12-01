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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <kni.h>
#include <qteapp_export.h>
#include <lfpport_export.h>
#include "lfpport_qte_mainwindow.h"
#include <midp_logging.h>

/**
 * @file Platform Look and Feel Port interface implementation
 */

static jboolean inFullScreenMode;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the LCDUI native resources.
 */
void lfpport_ui_init() {
  inFullScreenMode = KNI_FALSE;
  qteapp_init(lfpport_create_main_window);
}

/**
 * Finalize the LCDUI native resources.
 */
void lfpport_ui_finalize() {
  qteapp_finalize();
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
void lfpport_refresh(int hardwareId, int x1, int y1, int x2, int y2)
{
  (void) hardwareId;
  qteapp_get_mscreen()->refresh(x1, y1, x2, y2);
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
jboolean lfpport_direct_flush(int hardwareId, const java_graphics *g, 
		  	      const java_imagedata *offscreen_buffer, int h) 
{
  (void)hardwareId;
  (void)g;
  (void)offscreen_buffer;
  (void)h;
  return KNI_FALSE;
}
/**
 * Porting API function to update scroll bar.
 *
 * @param scrollPosition current scroll position
 * @param scrollProportion maximum scroll position
 * @return status of this call
 */
int lfpport_set_vertical_scroll(
  int scrollPosition, int scrollProportion) {
  REPORT_CALL_TRACE2(LC_HIGHUI, "LF:STUB:LCDUIsetVerticalScroll(%3d, %3d)\n",
		     scrollPosition, scrollProportion);

  /* Suppress unused parameter warnings */
  (void)scrollPosition;
  (void)scrollProportion;

  return 0;
}


/**
 * Turn on or off the full screen mode
 *
 * @param mode true for full screen mode
 *             false for normal
 */
void lfpport_set_fullscreen_mode(int hardwareId, jboolean mode) {
  (void)hardwareId;
  PlatformMIDPMainWindow * mainWindow = 
    PlatformMIDPMainWindow::getMainWindow();
  mainWindow->setFullScreen(mode);
  inFullScreenMode = mode;
}

jboolean lfpport_reverse_orientation(int hardwareId)
{
  (void)hardwareId;
    jboolean res = qteapp_get_mscreen()->reverse_orientation();
    PlatformMIDPMainWindow * mainWindow =
        PlatformMIDPMainWindow::getMainWindow();
    mainWindow->resizeScreen();
    return res;
}

void lfpport_handle_clamshell_event(){
}

jboolean lfpport_get_reverse_orientation(int hardwareId)
{
  (void)hardwareId;
    return qteapp_get_mscreen()->get_reverse_orientation();
}

int lfpport_get_screen_width(int hardwareId) {
  (void)hardwareId;
    if (inFullScreenMode) {
        return qteapp_get_mscreen()->getDisplayFullWidth();
    } else {
        return qteapp_get_mscreen()->getDisplayWidth();
    }
}

int lfpport_get_screen_height(int hardwareId) {
  (void)hardwareId;
    if (inFullScreenMode) {
        return qteapp_get_mscreen()->getDisplayFullHeight();
    } else {
        return qteapp_get_mscreen()->getDisplayHeight();            
    }
}

/**
 * Bridge function to ask MainWindow object for the full screen mode
 * status.
 */
jboolean lfpport_is_fullscreen_mode(int hardwareId) {
  (void)hardwareId;
  return inFullScreenMode;
}

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lfpport_gained_foreground(int hardwareId) {
  (void)hardwareId;
  qteapp_get_mscreen()->gainedForeground();
}

/** IMPL_NOTE: Just one display is supported currently. 
 *  Need the real implementation for multiple displays support 
 */


/** 
 * Get display device name by id
 */
char * lfpport_get_display_name(int hardwareId) {
    (void)hardwareId;
    return 0;
}


/**
 * Check if the display device is primary
 */
jboolean lfpport_is_display_primary(int hardwareId) {
    (void)hardwareId;
    return KNI_TRUE;
}
/**
 * Check if the display device is build-in
 */
jboolean lfpport_is_display_buildin(int hardwareId) {
    (void)hardwareId;
    return KNI_TRUE;
}
/**
 * Check if the display device supports pointer events
 */
jboolean lfpport_is_display_pen_supported(int hardwareId) {
    (void)hardwareId;
    return KNI_TRUE;
}
/**
 * Check if the display device supports pointer motion  events
 */
jboolean lfpport_is_display_pen_motion_supported(int hardwareId) {
    (void)hardwareId;
    return KNI_TRUE;
}
/**
 * Get display device capabilities
 */
int lfpport_get_display_capabilities(int hardwareId) {
    (void)hardwareId;
    return 255;
}


static jint display_device_ids[] = {0};

/**
 * Get the list of display device ids
 */

jint* lfpport_get_display_device_ids(jint* n) {
    *n = 1; 
    return display_device_ids;
}

/** 
 * Notify the display device state has been changed 
 */  
void lfpport_display_device_state_changed(int hardwareId, int state) {
    (void)hardwareId;
    (void)state;
} 

#ifdef __cplusplus
}
#endif
