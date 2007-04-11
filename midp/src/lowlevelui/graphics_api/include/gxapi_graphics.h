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

#ifndef _GXAPI_GRAPHICS_H_
#define _GXAPI_GRAPHICS_H_

#include <commonKNIMacros.h>
#include <ROMStructs.h>

/**
 * @defgroup lowui Low Level UI
 * @ingroup subsystems
 */

/**
 * @defgroup lowui_gxapi Graphics External Interface
 * @ingroup lowui
 */

/**
 * @file
 * @ingroup lowui_gxapi
 *
 * @brief Porting api for graphics_api library
 */

/**
 * Structure representing the <tt>Graphics</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_Graphics   java_graphics;

/**
 * Structure representing the <tt>ImageData</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_ImageData java_imagedata;

/**
 * Get a C structure representing the given <tt>Graphics</tt> class.
 */
#define GXAPI_GET_GRAPHICS_PTR(handle)       (unhand(java_graphics,(handle)))

/**
 * Get a C structure representing the given <tt>ImageData</tt> class.
 */
#define GXAPI_GET_IMAGEDATA_PTR(handle) (unhand(java_imagedata,(handle)))

/**
 * Get a C structure representing the given <tt>ImageData</tt> class.
 */
#define GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(handle) \
  GXAPI_GET_GRAPHICS_PTR(handle)->img != NULL ? \
  GXAPI_GET_GRAPHICS_PTR(handle)->img->imageData : \
  (java_imagedata*)NULL

/**
 * Gets the clipping region of the given graphics object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 * @param ARRAY native <tt>jshort</tt> array to save the clip data
 */
#define GXAPI_GET_CLIP(G, ARRAY) \
    ARRAY[0] = GXAPI_GET_GRAPHICS_PTR(G)->clipX1, \
    ARRAY[1] = GXAPI_GET_GRAPHICS_PTR(G)->clipY1, \
    ARRAY[2] = GXAPI_GET_GRAPHICS_PTR(G)->clipX2, \
    ARRAY[3] = GXAPI_GET_GRAPHICS_PTR(G)->clipY2

/**
 * Translate the pixel location according to the translation of
 * the given graphics object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 * @param X variable representing the <tt>x</tt> coordinate to be translated;
 *        this macro sets the value of X
 * @param Y variable representing the <tt>y</tt> coordinate to be translated;
 *        this macro sets the value of Y
 */
#define GXAPI_TRANSLATE(G, X, Y)  \
    (X) += GXAPI_GET_GRAPHICS_PTR((G))->transX, \
    (Y) += GXAPI_GET_GRAPHICS_PTR((G))->transY

#endif /* _GXAPI_GRAPHICS_H_ */
