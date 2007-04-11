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

#include <string.h>
#include <kni.h>
#include <midpMalloc.h>
#include <midp_logging.h>

#include "gxj_screen_buffer.h"

/**
 * Initialize screen buffer for a screen with specified demension,
 * allocate memory for pixel data.
 *
 * @param width width of the screen to initialize buffer for
 * @param height height of the screen to initialize buffer for
 * @return ALL_OK if successful, OUT_OF_MEMORY in the case of
 *   not enough memory to allocate the buffer
 */
MIDPError gxj_init_screen_buffer(int width, int height) {
    MIDPError stat = ALL_OK;
    int size = sizeof(gxj_pixel_type) * width * height;
    gxj_system_screen_buffer.width = width;
    gxj_system_screen_buffer.height = height;
    gxj_system_screen_buffer.alphaData = NULL;

    gxj_system_screen_buffer.pixelData =
        (gxj_pixel_type *)midpMalloc(size);
    if (gxj_system_screen_buffer.pixelData != NULL) {
        memset(gxj_system_screen_buffer.pixelData, 0, size);
    } else {
        stat = OUT_OF_MEMORY;
    }

    return stat;
}

/**
 * Resizes screen buffer to fit new screen geometry.
 * Call on screen change events like rotation. 
 *
 * @param width new width of the screen buffer
 * @param height new height of the screen buffer
 * @return ALL_OK if successful, OUT_OF_MEMORY in the case of
 *   not enough memory to reallocate the buffer
 */
MIDPError gxj_resize_screen_buffer(int width, int height) {

    if (gxj_system_screen_buffer.pixelData != NULL) {
        if (gxj_system_screen_buffer.width == width &&
        	gxj_system_screen_buffer.height == height) {

	    gxj_reset_screen_buffer();
            // no need to reallocate buffer, return
            return ALL_OK;

        } else {
    	    // IMPL_NOTE: It is up to caller to check that
    	    //   new screen buffer size is not bigger than 
    	    //   the screen itself, and to clear screen
    	    //   content on resizing

            // free screen buffer
            gxj_free_screen_buffer();
        }
    }
    // allocate resized screen buffer
    return gxj_init_screen_buffer(width, height);
}

/** Reset pixel data of screen buffer to zero */
void gxj_reset_screen_buffer() {
    if (gxj_system_screen_buffer.pixelData != NULL) {
	int size = sizeof(gxj_pixel_type) *
	    gxj_system_screen_buffer.width *
    	    gxj_system_screen_buffer.height;

	memset(gxj_system_screen_buffer.pixelData, 0, size);
    }
}

/**
 * Change screen buffer geometry according to screen rotation from
 * landscape to portrait mode and vice versa.
 */
void gxj_rotate_screen_buffer() {
    int width = gxj_system_screen_buffer.width;
    gxj_system_screen_buffer.width = gxj_system_screen_buffer.height;
    gxj_system_screen_buffer.height = width;
}

/** Free memory allocated for screen buffer */
void gxj_free_screen_buffer() {
    if (gxj_system_screen_buffer.pixelData != NULL) {
        midpFree(gxj_system_screen_buffer.pixelData);
        gxj_system_screen_buffer.pixelData = NULL;
    }
    if (gxj_system_screen_buffer.alphaData != NULL) {
        midpFree(gxj_system_screen_buffer.alphaData);
        gxj_system_screen_buffer.alphaData = NULL;
    }
}
