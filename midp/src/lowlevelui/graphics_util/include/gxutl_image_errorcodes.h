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

#ifndef _GXUTL_IMAGE_ERRORCODES_H_
#define _GXUTL_IMAGE_ERRORCODES_H_

/**
 * @file
 * @ingroup lowui_gxutl
 *
 * @brief Porting api for graphics_util library
 */

/**
 * Status codes returned by image handling and processing functions.
 */
typedef enum {
    GXUTL_NATIVE_IMAGE_NO_ERROR            = 0, /** Success */
    GXUTL_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR = 1, /** Out of memory */
    GXUTL_NATIVE_IMAGE_DECODING_ERROR      = 2, /** Problem decoding the image */
    GXUTL_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR = 3,/** Image format not supported */  
    GXUTL_NATIVE_IMAGE_RESOURCE_LIMIT = 4 /** Image resource limit exceeded */
}  gxutl_native_image_error_codes;

#endif /* _GXUTL_IMAGE_ERRORCODES_H_ */
