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

#ifndef _LFPPORT_EXPORT_H_
#define _LFPPORT_EXPORT_H_


/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Platform Look and Feel Port exported native interface
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <gxapi_graphics.h>
#include <imgapi_image.h>

/**
 * Refresh the given area.  For double buffering purposes.
 */
void lfpport_refresh(int hardwareId, int x, int y, int w, int h);

/**
 * set the screen mode either to fullscreen or normal.
 *
 * @param mode The screen mode
 */
void lfpport_set_fullscreen_mode(int hardwareId, jboolean mode);

/**
 * Change screen orientation flag
 */
jboolean lfpport_reverse_orientation(int hardwareId);

/**
 * Handle clamshell event
 */
void lfpport_handle_clamshell_event();
/**
 * Get screen orientation flag
 */
jboolean lfpport_get_reverse_orientation(int hardwareId);

/**
 * Return screen width
 */
int lfpport_get_screen_width(int hardwareId);

/**
 *  Return screen height
 */
int lfpport_get_screen_height(int hardwareId);

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lfpport_gained_foreground(int hardwareId);

/**
 * Initializes the window system.
 */
void lfpport_ui_init();

/**
 * Finalize the window system.
 */
void lfpport_ui_finalize();

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
		  	      const java_imagedata *offscreen_buffer, int h);

/** 
 * Get display device name by id
 */
char * lfpport_get_display_name(int hardwareId);


/**
 * Check if the display device is primary
 */
jboolean lfpport_is_display_primary(int hardwareId);

/**
 * Check if the display device is build-in
 */
jboolean lfpport_is_display_buildin(int hardwareId);

/**
 * Check if the display device supports pointer events
 */
jboolean lfpport_is_display_pen_supported(int hardwareId);

/**
 * Check if the display device supports pointer motion  events
 */
jboolean lfpport_is_display_pen_motion_supported(int hardwareId);

/**
 * Get display device capabilities
 */
int lfpport_get_display_capabilities(int hardwareId);

/**
 * Get the list of display device ids
 */
jint* lfpport_get_display_device_ids(jint* n);

void lfpport_display_device_state_changed(int hardwareId, int state);

#ifdef __cplusplus
}
#endif

#endif /* _LFPPORT_EXPORT_H_ */
