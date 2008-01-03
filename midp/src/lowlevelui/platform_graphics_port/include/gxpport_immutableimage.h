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

#ifndef _GXPPORT_IMMUTABLEIMAGE_H_
#define _GXPPORT_IMMUTABLEIMAGE_H_

/**
 * @file
 * @ingroup lowui_gxpport
 *
 * @brief Porting Interface for immutable image handling 
 */

#include <img_errorcodes.h> 
#include <gxpport_mutableimage.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Image's native resource handle */
typedef void* gxpport_image_native_handle;

/**
 * Creates a copy of the specified mutable image
 *
 * @param srcMutableImage   pointer to source image to copy
 * @param newImmutableImage address of pointer to return created new image
 * @param creationErrorPtr  pointer to error status.
 *                          This function sets creationErrorPtr's value.
 *
 */
void gxpport_createimmutable_from_mutable
(gxpport_mutableimage_native_handle srcMutableImage,
 gxpport_image_native_handle* newImmutableImage,
 img_native_error_codes* creationErrorPtr);

/**
 * Creates an immutable image that is a copy of a region
 * of the specified immutable image.
 *
 * @param srcImmutableImage pointer to source image
 * @param newImmutableImage address of pointer to return created new image
 * @param src_x      x-coord of the region
 * @param src_y      y-coord of the region
 * @param src_width  width of the region
 * @param src_height height of the region
 * @param transform  transform to be applied to the region
 * @param creationErrorPtr  pointer to error status.
 *                          This function sets creationErrorPtr's value.
 *
 */
void
gxpport_createimmutable_from_immutableregion
(gxpport_image_native_handle srcImmutableImage,
 int src_x, int src_y, 
 int src_width, int src_height,
 int transform,
 gxpport_image_native_handle* newImmutableImage,
 img_native_error_codes* creationErrorPtr);

/**
 * Creates an immutable image that is a copy of a region
 * of the specified mutable image.
 *
 * @param srcMutableImage   pointer to source image
 * @param newImmutableImage address of pointer to return created new image
 * @param src_x             x-coord of the region
 * @param src_y             y-coord of the region
 * @param src_width         width of the region
 * @param src_height        height of the region
 * @param transform         transform to be applied to the region
 * @param creationErrorPtr  pointer to error status.
 *                          This function sets creationErrorPtr's value.
 *
 */
void
gxpport_createimmutable_from_mutableregion
(gxpport_mutableimage_native_handle srcMutableImage,
 int src_x, int src_y, 
 int src_width, int src_height,
 int transform,
 gxpport_image_native_handle* newImmutableImage,
 img_native_error_codes* creationErrorPtr);

/**
 * Decodes the given input data into a storage format used by immutable
 * images.  The input data should be in a self-identifying format; that is,
 * the data must contain a description of the decoding process.
 * 
 *  @param newImmutableImage address of pointer to return created new image
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param ret_imgWidth pointer to the width of the decoded image when the
 *         function runs successfully. This function sets ret_imgWidth's
 *         value.
 *  @param ret_imgHeight pointer to the height of the decoded image when the
 *         function runs successfully. This function sets ret_imgHeight's
 *         value.
 *  @param creationErrorPtr pointer to the status of the decoding
 *         process. This function sets creationErrorPtr's value.
 */
void
gxpport_decodeimmutable_from_selfidentifying
(unsigned char* srcBuffer, int length, 
 int* ret_imgWidth, int* ret_imgHeight,
 gxpport_image_native_handle* newImmutableImage,
 img_native_error_codes* creationErrorPtr);

/**
 * Decodes the ARGB input data into a storage format used by immutable images.
 * The array consists of values in the form of 0xAARRGGBB.
 * 
 * @param srcBuffer input data to be decoded.
 * @param width width of the image, in pixels.
 * @param height height of the image, in pixels.
 * @param processAlpha if true Alpha channel should be processed
 * @param newImmutableImage address of pointer to return created new image
 * @param creationErrorPtr pointer to the status of the decoding
 *        process. This function sets creationErrorPtr's value.
 */
void
gxpport_decodeimmutable_from_argb
(jint* srcBuffer,
 int width, int height,
 jboolean processAlpha,
 gxpport_image_native_handle* newImmutableImage,
 img_native_error_codes* creationErrorPtr);

/**
 * Renders the contents of the specified immutable image
 * onto the destination specified.
 *
 * @param immutableImage      pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to structure holding the clip
 *                              [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * 
 */
void
gxpport_render_immutableimage
(gxpport_image_native_handle immutableImage,
 void* graphicsDestination,
 const jshort *clip,
 jint x_dest, jint y_dest);

/**
 * Renders the contents of the specified region of this
 * immutable image onto the destination specified.
 *
 * @param srcImmutableImage   pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to structure holding the clip
 *                                [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 *
 */
void
gxpport_render_immutableregion
(gxpport_image_native_handle srcImmutableImage,
 void* graphicsDestination,
 const jshort *clip,
 jint x_dest, jint y_dest, 
 jint width, jint height,
 jint x_src, jint y_src,
 jint transform);

/**
 * Gets ARGB representation of the specified immutable image
 * @param imutableImage pointer to the source image
 * @param rgbBuffer     pointer to buffer to write with the ARGB data
 * @param offset        offset in the buffer at which to start writing
 * @param scanLength    the relative offset within the array
 *                      between corresponding pixels of consecutive rows
 * @param x             x-coordinate of region
 * @param y             y-coordinate of region
 * @param width         width of region
 * @param height        height of region
 * @param errorPtr Error status pointer to the status.
 *                 This function sets creationErrorPtr's value.
 */
void
gxpport_get_immutable_argb
(gxpport_image_native_handle imutableImage,
 jint* rgbBuffer, int offset, int scanLength,
 int x, int y, int width, int height,
 img_native_error_codes* errorPtr);
  
/**
 * Cleans up any native resources to prepare the image to be garbage collected.
 *
 * @param immutableImage pointer to the platform immutable image to destroy.
 */
void gxpport_destroy_immutable(gxpport_image_native_handle immutableImage);

/**
 * Decodes the given input data into a native platform representation that can
 * be saved.  The input data should be in a self-identifying format; that is,
 * the data must contain a description of the decoding process.
 *
 *  @param srcBuffer input data to be decoded. 
 *  @param length length of the input data.
 *  @param ret_dataBuffer pointer to the platform representation data that
 *         be saved.
 *  @param ret_length pointer to the length of the return data. 
 *  @param creationErrorPtr pointer to the status of the decoding
 *         process. This function sets creationErrorPtr's value.
 */
void gxpport_decodeimmutable_to_platformbuffer
(unsigned char* srcBuffer, long length,
 unsigned char** ret_dataBuffer, long* ret_length,
 img_native_error_codes* creationErrorPtr);

/**
 * Loads the given input data into a storage format used by immutable
 * images.  The input data should be the native platform representation.
 * 
 *  @param newImmutableImage address of pointer to return created new image
 *  @param srcBuffer input data to be loaded.
 *  @param length length of the input data.
 *  @param ret_imgWidth pointer to the width of the loaded image when the
 *         function runs successfully. This function sets ret_imgWidth's
 *         value.
 *  @param ret_imgHeight pointer to the height of the loaded image when the
 *         function runs successfully. This function sets ret_imgHeight's
 *         value.
 *  @param creationErrorPtr pointer to the status of the loading
 *         process. This function sets creationErrorPtr's value.
 */
void
gxpport_loadimmutable_from_platformbuffer
(unsigned char* srcBuffer, int length, jboolean isStatic,
 int* ret_imgWidth, int* ret_imgHeight,
 gxpport_image_native_handle* newImmutableImage,
 img_native_error_codes* creationErrorPtr);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* _GXPPORT_IMMUTABLEIMAGE_H_  */
