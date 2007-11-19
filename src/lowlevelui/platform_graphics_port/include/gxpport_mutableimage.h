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

#ifndef _GXPPORT_MUTABLEIMAGE_H_
#define _GXPPORT_MUTABLEIMAGE_H_

/**
 * @file
 * @ingroup lowui_gxpport
 *
 * @brief Porting Interface for mutable image handling.
 */

#include <commonKNIMacros.h>
#include <img_errorcodes.h> 

/**
 * Opaque handle to mutable image native representation.
 */
typedef void* gxpport_mutableimage_native_handle;


/**
 * Initializes the internal members of the native image structure, as required
 * by the platform.
 *
 * @param newImagePtr address of pointer to return created new image
 * @param width width of the image, in pixels.
 * @param height height of the image, in pixels.
 * @param creationErrorPtr pointer for the status of the decoding
 *        process. This function sets creationErrorPtr's value.
 */
extern
#ifdef __cplusplus
"C"
#endif
void
gxpport_create_mutable(
    gxpport_mutableimage_native_handle* newImagePtr,
    int  width, int height,
    img_native_error_codes* creationErrorPtr);


/**
 * Renders the contents of the specified mutable image
 * onto the destination specified.
 *
 * @param srcImagePtr         pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to structure holding the clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * 
 */
extern
#ifdef __cplusplus
"C"
#endif
void gxpport_render_mutableimage(
	gxpport_mutableimage_native_handle srcImagePtr,
	gxpport_mutableimage_native_handle graphicsDestination,
	const jshort *clip,
	jint x_dest, jint y_dest);

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param srcImagePtr         pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to structure holding the clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 *
 */
extern
#ifdef __cplusplus
"C"
#endif
void gxpport_render_mutableregion(
    gxpport_mutableimage_native_handle srcImagePtr,
    gxpport_mutableimage_native_handle graphicsDestination,
    const jshort *clip,
    jint x_dest, jint y_dest, 
    jint width, jint height,
    jint x_src, jint y_src,
    jint transform);

/**
 * Gets ARGB representation of the specified immutable image
 * @param nativePixmap  pointer to the source image
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
extern
#ifdef __cplusplus
"C"
#endif
void gxpport_get_mutable_argb(
     gxpport_mutableimage_native_handle nativePixmap,
     jint* rgbBuffer, int offset, int scanLength,
     int x, int y, int width, int height,
     img_native_error_codes* errorPtr);


/**
 * Cleans up any native resources to prepare the image to be garbage collected.
 *
 * @param destrImagePtr pointer to the image that needs to be cleaned up
 */
extern
#ifdef __cplusplus
"C"
#endif
void gxpport_destroy_mutable(gxpport_mutableimage_native_handle destrImagePtr);


#endif /* _GXPPORT_MUTABLEIMAGE_H_  */
