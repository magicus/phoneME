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

#ifndef _IMG_ERRORCODES_H_
#define _IMG_ERRORCODES_H_

/**
 * @file
 * @ingroup lowui_img
 *
 * @brief Porting api for image library
 */

/**
 * Status codes returned by image handling and processing functions.
 */
typedef enum {
    IMG_NATIVE_IMAGE_NO_ERROR            = 0, /** Success */
    IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR = 1, /** Out of memory */
    IMG_NATIVE_IMAGE_DECODING_ERROR      = 2, /** Problem decoding the image */
    IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR = 3,/** Image format not supported */  
    IMG_NATIVE_IMAGE_RESOURCE_LIMIT = 4 /** Image resource limit exceeded */
}  img_native_error_codes;

#endif /* _IMG_ERRORCODES_H_ */
