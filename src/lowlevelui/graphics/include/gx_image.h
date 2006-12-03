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

#ifndef _GX_IMAGE_H_
#define _GX_IMAGE_H_

#include <commonKNIMacros.h>
#include <midpError.h>

#include <gxapi_graphics.h>
#include <gxutl_image_errorcodes.h>


/**
 * @file
 * @ingroup lowui_gx
 *
 * @brief Porting api for graphics library
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets an ARGB integer array from this image. The
 * array consists of values in the form of 0xAARRGGBB.
 *
 * @param imageData The ImageData to read the ARGB data from
 * @param rgbBuffer The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlen Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
extern void gx_get_argb(const java_imagedata * srcImageDataPtr, 
			jint * rgbBuffer, 
			jint offset,
			jint scanlen,
			jint x, jint y, jint width, jint height,
			gxutl_native_image_error_codes * errorPtr);

/**
 * Draws the source image onto the destination image at the given coordinates.
 *
 * <p>If the source image contains transparent pixels, the corresponding
 * pixels in the destination image must be left untouched.  If the source
 * image contains partially transparent pixels, a compositing operation 
 * must be performed with the destination pixels, leaving all pixels of 
 * the destination image fully opaque.</p>
 *
 * @param srcImageDataPtr the source image to be rendered
 * @param dstMutableImageDataPtr the mutable destination image to be rendered to
 * @param clip the clip of the target image
 * @param x the x coordinate of the anchor point
 * @param y the y coordinate of the anchor point
 */
extern void gx_render_image(const java_imagedata * srcImageDataPtr,
			    const java_imagedata * dstMutableImageDataPtr,
			    const jshort * clip,
			    jint x, jint y);

/**
 * Renders the given region of the source image onto the destination image
 * at the given coordinates.
 *
 * @param srcImageDataPtr the source image to be rendered
 * @param dstMutableImageDataPtr the mutable destination image to be rendered to
 * @param x_src The x coordinate of the upper-left corner of the
 *              source region
 * @param y_src The y coordinate of the upper-left corner of the
 *              source region
 * @param width The width of the source region
 * @param height The height of the source region
 * @param x_dest The x coordinate of the upper-left corner of the destination region
 * @param y_dest The y coordinate of the upper-left corner of the destination region
 * @param transform The transform to apply to the selected region.
 */
extern void gx_render_imageregion(const java_imagedata * srcImageDataPtr,
				  const java_imagedata * dstMutableImageDataPtr,
				  const jshort * clip,
				  jint x_src, jint y_src, 
				  jint width, jint height,
				  jint x_dest, jint y_dest, 
				  jint transform);


/**
 * Decodes the given input data into a cache representation that can
 * be saved and reload quickly. 
 * The input data should be in a self-identifying format; that is,
 * the data must contain a description of the decoding process.
 *
 * @param srcBuffer input data to be decoded.
 * @param length length of the input data.
 * @param ret_dataBuffer pointer to the platform representation data that
 *         		 be saved.
 * @param ret_length pointer to the length of the return data. 
 *
 * @return one of error codes:
 *		MIDP_ERROR_NONE,
 *		MIDP_ERROR_OUT_MEM,
 *		MIDP_ERROR_UNSUPPORTED,
 *		MIDP_ERROR_OUT_OF_RESOURCE,
 *		MIDP_ERROR_IMAGE_CORRUPTED
 */
MIDP_ERROR gx_decode_data2cache(unsigned char* srcBuffer,
				unsigned int length,
				unsigned char** ret_dataBuffer,
				unsigned int* ret_length);

#ifdef __cplusplus
}
#endif

#endif /* _GX_IMAGE_H_ */
