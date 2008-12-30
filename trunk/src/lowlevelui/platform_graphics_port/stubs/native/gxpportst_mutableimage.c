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

#include <gxpport_mutableimage.h>
#include <midp_logging.h>

/**
 * Initializes the internal members of the native image structure, as required
 * by the platform.
 *
 * @param newImagePtr structure to hold the image's native representation.
 * @param width width of the image, in pixels.
 * @param height height of the image, in pixels.
 * @param creationErrorPtr pointer for the status of the decoding
 *        process. This function sets creationErrorPtr's value.
 */
void gxpport_create_mutable(gxpport_mutableimage_native_handle *newImagePtr,
                                            int width, 
                                            int height,
                                            img_native_error_codes* 
                                            creationErrorPtr) {
    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_create_mutable()\n");

    /* Suppress unused parameter warnings */
    (void)newImagePtr;
    (void)width;
    (void)height;
    
    /* Not yet implemented */
    *creationErrorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
}

/**
 * Renders the contents of the specified mutable image
 * onto the destination specified.
 *
 * @param srcImagePtr         pointer to mutable source image
 * @param dstImagePtr         pointer to mutable destination image or
 *                            NULL for the screen
 * @param clip                pointer to structure holding the clip
 *                                [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 */
void gxpport_render_mutableimage(gxpport_mutableimage_native_handle srcImagePtr,
				 gxpport_mutableimage_native_handle dstImagePtr,
				 const jshort *clip,
				 int x_dest, int y_dest) {
    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_render_mutableimage()\n");

    /* Suppress unused parameter warnings */
    (void)srcImagePtr;
    (void)dstImagePtr;
    (void)clip;
    (void)x_dest;
    (void)y_dest;
}

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param srcImagePtr         pointer to mutable source image
 * @param dstImagePtr         pointer to mutable destination image or
 *                            NULL for the screen
 * @param clip                pointer to structure holding the clip
 *                                [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 */
void
gxpport_render_mutableregion(gxpport_mutableimage_native_handle srcImagePtr,
			     gxpport_mutableimage_native_handle dstImagePtr,
			     const jshort *clip,
			     int x_dest, int y_dest, 
			     int width, int height,
			     int x_src, int y_src,
			     int transform) {
    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_render_mutableregion()\n");

    /* Suppress unused parameter warnings */
    (void)srcImagePtr;
    (void)dstImagePtr;
    (void)clip;
    (void)x_dest;
    (void)y_dest;
    (void)width;
    (void)height;
    (void)x_src;
    (void)y_src;
    (void)transform;
}

/**
 * Gets ARGB representation of the specified mutable image
 *
 * @param imagePtr      pointer to the mutable source image
 * @param rgbBuffer     pointer to buffer to write with the ARGB data
 * @param offset        offset in the buffer at which to start writing
 * @param scanLength    the relative offset within the array
 *                      between corresponding pixels of consecutive rows
 * @param x             x-coordinate of region
 * @param y             y-coordinate of region
 * @param width         width of region
 * @param height        height of region
 * @param errorPtr Error status pointer to the status
 *                 This function sets creationErrorPtr's value.
 */
void gxpport_get_mutable_argb(gxpport_mutableimage_native_handle imagePtr,
			      jint* rgbBuffer, int offset, int scanLength,
			      int x, int y, int width, int height,
			      img_native_error_codes* errorPtr) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_get_mutable_argb()\n");
    
    /* Suppress unused parameter warning */
    (void)imagePtr;
    (void)rgbBuffer;
    (void)offset;
    (void)scanLength;
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    /* Not yet implemented */
    *errorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
}

/**
 * Cleans up any native resources to prepare the image to be garbage collected.
 *
 * @param imagePtr the mutable image.
 * @param errorPtr pointer for a status code set on return 
 */
void gxpport_destroy_mutable(gxpport_mutableimage_native_handle imagePtr) {
    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_destroy_mutable()\n");
    
    /* Suppress unused parameter warning */
    (void)imagePtr;
}
