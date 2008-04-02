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

#ifndef _LCDLF_EXPORT_H_
#define _LCDLF_EXPORT_H_

/**
 * @defgroup highui_lcdlf Look And Feel External Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_lcdlf
 *
 * @brief LCDUI Look and Feel exported native interface (common for platform widgets and java LF).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <gxapi_graphics.h>
#include <imgapi_image.h>

/**
 * Refresh the given area.  For double buffering purposes.
 */
void lcdlf_refresh(int x, int y, int w, int h);

/**
 * Change screen orientation flag
 */
jboolean lcdlf_reverse_orientation();

/**
 * Change screen orientation flag
 */
jboolean lcdlf_get_reverse_orientation();


/**
 * Return screen width
 */
int lcdlf_get_screen_width();

/**
 *  Return screen height
 */
int lcdlf_get_screen_height();

/**
 * set the screen mode either to fullscreen or normal.
 *
 * @param mode The screen mode
 */
void lcdlf_set_fullscreen_mode(jboolean mode);

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lcdlf_gained_foreground();

/**
 * Initializes the window system.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int lcdlf_ui_init();

/**
 * Finalize the window system.
 */
void lcdlf_ui_finalize();

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
jboolean lcdlf_direct_flush(const java_graphics *g, 
			    const java_imagedata *offscreen_buffer, int h);



/**
 * Check if native softbutton is supported on platform
 * @return KNI_TRUE if native softbutton is supported, KNI_FALSE - otherwise
 */

jboolean lcdlf_is_native_softbutton_layer_supported();

/**
 * Request platform to draw a label in the soft button layer.
 * @param label Label to draw (UTF16)
 * @param len Length of the lable (0 will cause removal of current label)
 * @param index Index of the soft button in the soft button bar.
 */
void lcdlf_set_softbutton_label_on_native_layer (unsigned short *label, 
                                                 int len, 
                                                 int index);

#ifdef __cplusplus
}
#endif

#endif /* _LCDLF_EXPORT_H_ */
