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

#ifndef _IMGAPI_IMAGE_H_
#define _IMGAPI_IMAGE_H_

#include <commonKNIMacros.h>
#include <ROMStructs.h>

/**
 * @defgroup lowui Low Level UI
 * @ingroup subsystems
 */

/**
 * @defgroup lowui_imgapi Graphics External Interface
 * @ingroup lowui
 */

/**
 * @file
 * @ingroup lowui_imgapi
 *
 * @brief Porting api for image_api library
 */

/**
 * Structure representing the <tt>Image</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_Image      _MidpImage;


/**
 * Structure representing the <tt>ImageData</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_ImageData java_imagedata;


/**
 * Get a C structure representing the given <tt>ImageData</tt> class.
 */
#define IMGAPI_GET_IMAGEDATA_PTR(handle) (unhand(java_imagedata,(handle)))


/**
 * Get a C structure representing the given <tt>Image</tt> class.
 */
#define IMGAPI_GET_IMAGE_PTR(handle)          (unhand(_MidpImage,(handle)))


#endif /* _IMGAPI_IMAGE_H_ */
