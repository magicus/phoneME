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

#include <midpError.h>


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
