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

#ifndef _GXJ_SCREEN_BUFFER_H
#define _GXJ_SCREEN_BUFFER_H

/**
 * @file
 * @ingroup lowui_port
 *
 * @brief Generic operations typical for screen buffer
 */

#include <gxj_putpixel.h>
#include <midp_global_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * IMPL_NOTE: Each port must define one system screen buffer
 *   from where pixels are copied to physical screen. However
 *   many ports have very similar logic to work with their screen
 *   buffers, e.g. memory allocation and release, updating screen
 *   buffer according to screen rotation or resizing and so on.
 *   This API is designed to provide generic implementations for
 *   screen buffer manipulations, any port can choose to use it
 *   or not.
 */

/**
 * Initialize screen buffer for a screen with specified demension,
 * allocate memory for pixel data.
 *
 * @param width width of the screen to initialize buffer for
 * @param height height of the screen to initialize buffer for
 * @return ALL_OK if successful, OUT_OF_MEMORY in the case of
 *   not enough memory to allocate the buffer
 */
MIDPError gxj_init_screen_buffer(int width, int height);

/**
 * Resizes screen buffer to fit new screen geometry.
 * Call on screen change events like rotation.
 *
 * IMPL_NOTE: It is up to caller to check that new screen
 *   buffer size is not bigger than the screen itself,
 *   and to clear screen content on resizing.
 *
 * @param width new width of the screen buffer
 * @param height new height of the screen buffer
 * @return ALL_OK if successful, OUT_OF_MEMORY in the case of
 *   not enough memory to reallocate the buffer
 */
MIDPError gxj_resize_screen_buffer(int width, int height);

/** Reset pixel data of screen buffer to zero */
void gxj_reset_screen_buffer();

/**
 * Change screen buffer geometry according to screen rotating
 * from landscape to portrait mode and vice versa.
 * On demand screen buffer content is rotated with proper
 * clipping according to new geometry.
 *
 * @param keepContent true to preserve current buffer content
 */
void gxj_rotate_screen_buffer(jboolean keepContent);

/** Free memory allocated for screen buffer */
void gxj_free_screen_buffer();

#ifdef __cplusplus
}
#endif

#endif /* _GXJ_SCREEN_BUFFER_H */
