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
 * The plain array contains pixel data for screen with width x height
 * geometry. In the case the screen is rotated to height x width and
 * we want to preserve its content, we should reformat the plain data
 * with proper clipping to new geometry. The method reformats the
 * data 'in place'.
 *
 *  @param data pointer to the plain array with screen data
 *  @param width the width in pixels before rotation
 *  @param height the height in pixels before rotation
 *  @param elem_bytes size in bytes of the data per screen pixel 
 */
static void reformat_plain_data(
    void *data, int width, int height, size_t elem_bytes) {

    int i;
    char *src, *dst;
    int width_bytes = width * elem_bytes;
    int height_bytes = height * elem_bytes;
    src = dst = data;

    if (width < height) {
        char *buf = (char *)midpMalloc(width_bytes);
        src += (width-1) * width_bytes;
        dst += (width-1) * height_bytes;
        for (i = width; i > 0 ; i--) {
            // prevent intersected memory copying
            if (dst < src + width_bytes) {
                memcpy(buf, src, width_bytes);
                memcpy(dst, buf, width_bytes);
            } else {
                memcpy(dst, src, width_bytes);
            }

            memset(dst + width_bytes, 0,
                (height-width) * elem_bytes);
            src -= width_bytes;
            dst -= height_bytes;
        }
        midpFree(buf);
    } else if (width > height) {
        for (i = 0; i < height; i++) {
            memcpy(dst, src, height_bytes);
            src += width_bytes;
            dst += height_bytes;
        }
        memset(dst, 0,
            (width-height) * height_bytes);
    }
}

/**
 * Format screen buffer content regarding rotated screen geometry
 * clipping the content by the minimal of screen dimensions.
 * It is important for plain array to contain screen lines of
 * proper scan length.
 */
static void gxj_rotate_buffer_content() {
    if (gxj_system_screen_buffer.pixelData != NULL) {
        reformat_plain_data(
            gxj_system_screen_buffer.pixelData,
            gxj_system_screen_buffer.width,
            gxj_system_screen_buffer.height,
            sizeof(gxj_pixel_type));
    }
    if (gxj_system_screen_buffer.alphaData != NULL) {
        reformat_plain_data(
            gxj_system_screen_buffer.alphaData,
            gxj_system_screen_buffer.width,
            gxj_system_screen_buffer.height,
            sizeof(gxj_alpha_type));
    }
}

/**
 * Change screen buffer geometry according to screen rotation from
 * landscape to portrait mode and vice versa.
 *
 * @param keepContent true to preserve screen buffer content
 *    clipped according to the new buffer geometry
 */
void gxj_rotate_screen_buffer(jboolean keepContent) {
    int height;

    if (keepContent)
        gxj_rotate_buffer_content();
    else gxj_reset_screen_buffer();

    height = gxj_system_screen_buffer.height;
    gxj_system_screen_buffer.height = gxj_system_screen_buffer.width;
    gxj_system_screen_buffer.width = height;
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
