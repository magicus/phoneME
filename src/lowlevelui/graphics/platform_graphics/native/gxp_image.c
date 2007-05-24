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

/**
 * \file
 * Immutable image functions that needed to be implemented for each port.
 */

#include <gx_image.h>
#include <gxpport_mutableimage.h>
#include <gxpport_immutableimage.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Draws the specified image at the given coordinates.
 *
 * <p>If the source image contains transparent pixels, the corresponding
 * pixels in the destination image must be left untouched.  If the source
 * image contains partially transparent pixels, a compositing operation 
 * must be performed with the destination pixels, leaving all pixels of 
 * the destination image fully opaque.</p>
 *
 * @param srcImageDataPtr the source image to be rendered
 * @param dstMutableImageDataPtr the mutable target image to be rendered to
 * @param clip the clip of the target image
 * @param x the x coordinate of the anchor point
 * @param y the y coordinate of the anchor point
 */
extern void gx_render_image(const java_imagedata * srcImageDataPtr,
			    const java_imagedata * dstMutableImageDataPtr,
			    const jshort * clip,
			    jint x, jint y) {
  gxpport_image_native_handle srcImageNativeData =
    (gxpport_image_native_handle)srcImageDataPtr->nativeImageData;    
  gxpport_mutableimage_native_handle dstMutableImageNativeData =
    (gxpport_mutableimage_native_handle)(dstMutableImageDataPtr ? 
				   dstMutableImageDataPtr->nativeImageData : 
				   0);

  if (srcImageDataPtr->isMutable) {
    gxpport_render_mutableimage(srcImageNativeData, 
				dstMutableImageNativeData, clip, x, y);
  } else {
    gxpport_render_immutableimage(srcImageNativeData, 
				  dstMutableImageNativeData, clip, x, y);
  }
}

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
				   jint transform) {
  gxpport_image_native_handle srcImageNativeData =
    (gxpport_image_native_handle)srcImageDataPtr->nativeImageData;    
  gxpport_mutableimage_native_handle dstMutableImageNativeData =
    (gxpport_mutableimage_native_handle)(dstMutableImageDataPtr ? 
				   dstMutableImageDataPtr->nativeImageData : 
				   0);

  if (srcImageDataPtr->isMutable) {
    gxpport_render_mutableregion(srcImageNativeData,
				   dstMutableImageNativeData,
				   clip,
				   x_dest, y_dest,
				   width, height,
				   x_src, y_src,
				   transform);
  } else {
    gxpport_render_immutableregion(srcImageNativeData,
				   dstMutableImageNativeData,
				   clip,
				   x_dest, y_dest,
				   width, height,
				   x_src, y_src,
				   transform);
  }
}

#ifdef __cplusplus
}
#endif
