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

#ifndef _IMG_IMAGEDATA_LOAD_H_
#define _IMG_IMAGEDATA_LOAD_H_


#include <sni.h>
#include <commonKNIMacros.h>


/**
 * @file
 * @ingroup lowui_img
 *
 * @brief Porting api for image library
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load Java ImageData instance with image data in RAW format.
 * Image data is provided in native buffer.
 *
 * @param imageData Java ImageData object to be loaded with image data
 * @param buffer pointer to native buffer with raw image data
 * @param length length of the raw image data in the buffer
 *
 * @return KNI_TRUE in the case ImageData is successfully loaded with
 *    raw image data, otherwise KNI_FALSE.
 */
int img_load_imagedata_from_raw_buffer(KNIDECLARGS jobject imageData,
    unsigned char *buffer, int length);

#ifdef __cplusplus
}
#endif

#endif /* _IMG_IMAGEDATA_LOAD_H_ */
