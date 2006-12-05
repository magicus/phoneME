/*
 *  
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

#include <java_types.h>

/**
 * Refresh the given area.  For double buffering purposes.
 */
void lfpport_refresh(int x, int y, int w, int h);

/**
 * set the screen mode either to fullscreen or normal.
 *
 * @param mode The screen mode
 */
void lfpport_set_fullscreen_mode(jboolean mode);

/**
 * Change screen orientation flag
 */
jboolean lfpport_reverse_orientation();

/**
 * Get screen orientation flag
 */
jboolean lfpport_get_reverse_orientation();

/**
 * Return screen width
 */
int lfpport_get_screen_width();

/**
 *  Return screen height
 */
int lfpport_get_screen_height();

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lfpport_gained_foreground();

/**
 * Initializes the window system.
 */
void lfpport_ui_init();

/**
 * Finalize the window system.
 */
void lfpport_ui_finalize();

#ifdef __cplusplus
}
#endif

#endif /* _LFPPORT_EXPORT_H_ */
