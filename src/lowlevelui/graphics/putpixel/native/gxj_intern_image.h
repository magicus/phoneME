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

#ifndef _GXJ_INTERN_IMAGE_H__
#define _GXJ_INTERN_IMAGE_H__

#include <gx_image.h>

#include "gxj_intern_putpixel.h"

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param src                 pointer to source VDC
 * @param dest                pointer to destination VDC
 * @param clip                pointer to the dest clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 */
void copy_imageregion(gxj_screen_buffer* src, gxj_screen_buffer* dest,
		      const jshort *clip,
		      jint x_dest, jint y_dest,
		      jint width, jint height, jint x_src, jint y_src,
		      jint transform);


/**
 * Renders the contents of the specified mutable image
 * onto the destination specified.
 *
 * @param srcImagePtr         pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to the clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * 
 */
extern
#ifdef __cplusplus
"C"
#endif
void draw_image(gxj_screen_buffer *imageBuf, gxj_screen_buffer *gBuf,
		const jshort *clip,
		jint x_dest, jint y_dest);

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param srcImagePtr         pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to the clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 *
 */
extern
#ifdef __cplusplus
"C"
#endif
void draw_imageregion(gxj_screen_buffer *imageBuf,
		      gxj_screen_buffer *gBuf,
		      const jshort *clip,
		      jint x_dest, jint y_dest, 
		      jint width, jint height,
		      jint x_src, jint y_src,
		      jint transform);

#endif /* _GXJ_INTERN_IMAGE_H__  */
