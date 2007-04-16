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

#ifndef _IMGDCD_IMAGE_H_
#define _IMGDCD_IMAGE_H_

#include <gxutl_image_errorcodes.h>

/**
 * @file
 * @ingroup lowui_gx
 *
 * @brief Graphics utility api for image decoding
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * 16-bit pixel.
 * The color encoding used in pixels is 565, that is,
 * 5+6+5=16 bits for red, green, blue.
 */
typedef unsigned short imgdcd_pixel_type;

/** 8-bit alpha */
typedef unsigned char imgdcd_alpha_type;

/**
 * Decodes the given input data into a storage format used by immutable
 * images.  The input data should be a PNG image.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param creationErrorPtr pointer to the status of the decoding
 *         process. This function sets creationErrorPtr's value.
 */
int decode_png(unsigned char* srcBuffer, int length, 
	       int  width, int height,
	       imgdcd_pixel_type *pixelData, 
	       imgdcd_alpha_type *alphaData,
	       gxutl_native_image_error_codes* creationErrorPtr);
  

/**
 * Decodes the given input data into a storage format used by
 * images.  The input data should be a JPEG image.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param creationErrorPtr pointer to the status of the decoding
 *         process. This function sets creationErrorPtr's value.
 */
void decode_jpeg (unsigned char* srcBuffer, int length,
		  int width, int height,
		  imgdcd_pixel_type *pixelData, 
		  imgdcd_alpha_type *alphaData,
		  gxutl_native_image_error_codes* creationErrorPtr);



#ifdef __cplusplus
}
#endif

#endif /* _IMGDCD_IMAGE_H_ */
