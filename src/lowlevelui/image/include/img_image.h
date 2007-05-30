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

#ifndef _IMG_IMAGE_H_
#define _IMG_IMAGE_H_

#include <commonKNIMacros.h>
#include <midpError.h>

#include <img_errorcodes.h>
#include <imgapi_image.h>


/**
 * @file
 * @ingroup lowui_img
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
extern void img_get_argb(const java_imagedata * srcImageDataPtr, 
			 jint * rgbBuffer, 
			 jint offset,
			 jint scanlen,
			 jint x, jint y, jint width, jint height,
			 img_native_error_codes * errorPtr);


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
extern MIDP_ERROR img_decode_data2cache(unsigned char* srcBuffer,
					unsigned int length,
					unsigned char** ret_dataBuffer,
					unsigned int* ret_length);


#ifdef __cplusplus
}
#endif

#endif /* _IMG_IMAGE_H_ */
