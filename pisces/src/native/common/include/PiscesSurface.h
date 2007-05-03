/*
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


#ifndef PISCES_SURFACE_H
#define PISCES_SURFACE_H

#include <PiscesDefs.h>

//Color models - supported surfaces 
/** 
 * @defgroup SurfaceTypes Surface types supported in PISCES
 * Surface type. There are many color models used in computer graphics. This is
 * because of finite color count, that can be displayed on different graphic-
 * cards/displays.    
 * 
 * @def TYPE_INT_RGB
 * @ingroup SurfaceTypes 
 * Surface type TYPE_INT_RGB. There is one byte for every
 * RGB component. Data are stored in array of int. There is no native alpha
 * support in this model.
 * @def TYPE_INT_ARGB
 * @ingroup SurfaceTypes
 * Surface type TYPE_INT_ARGB. There is one byte for every RGB component and 
 * alpha. Data are stored in array of int.   
 * @def TYPE_INT_ARGB_PRE
 * @ingroup SurfaceTypes 
 * Surface type TYPE_INT_ARGB_PRE. Every color component is premultiplied by its
 * alpha value. Four bytes are used to store one pixel value.
 * @def TYPE_USHORT_565_RGB
 * @ingroup SurfaceTypes 
 * Surface type TYPE_USHORT_565_RGB. This color model uses 2 bytes to store
 * RGB color. 5bits for Red and Blue component and 6 bits for Green component.
 * We use unsigned short array to store the data.
 * @def TYPE_BYTE_GRAY
 * @ingroup SurfaceTypes 
 * Surface type TYPE_BYTE_GRAY. This color model uses 256 grades of shade to
 * view data. We use one byte to represent one pixel.        
 */
#define TYPE_INT_RGB 1
#define TYPE_INT_ARGB 2
#define TYPE_INT_ARGB_PRE 3
#define TYPE_USHORT_565_RGB 8
#define TYPE_BYTE_GRAY 10

#define CORRECT_DIMS(_surface, _x, _y, _w, _h, _x1, _y1) \
  if (_x < 0) {   \
    _x1 -= _x;    \
    _w += _x;     \
    _x = 0;       \
  }               \
  if (_y < 0) {   \
    _y1 -= _y;    \
    _h += _y;     \
    _y = 0;       \
  }               \
  if ((_x + _w) > (_surface)->width) {  \
    _w = (_surface)->width - _x;        \
  }                                   \
  if ((_y + _h) > (_surface)->height) { \
    _h = (_surface)->height - _y;       \
  }

typedef struct _Surface {
    jint width;
    jint height;
  
    jint offset;
    jint scanlineStride;
    jint pixelStride;

    jint imageType;
    void* data;
} Surface;

#endif
