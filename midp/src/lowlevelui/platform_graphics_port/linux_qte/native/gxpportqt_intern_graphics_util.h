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
 * 
 * This source file is specific for Qt-based configurations.
 */

#ifndef _GXPPORTQT_INTERN_GRAPHICS_UTIL_H_
#define _GXPPORTQT_INTERN_GRAPHICS_UTIL_H_

/**
 * @file
 * Utility functions that are used by the graphics and image code.
 */
#include <qpixmap.h>
#include <qimage.h>

/**
 * Transforms the specified section of the given Pixmap
 * @param originalPixmap pointer to the source pixmap
 * @param destPixmap     pointer to the resulting pixmap
 * @param src_x          x offset of the region to be transformed,
 *                       within the source pixmap
 * @param src_y          y offset of the region to be transformed,
 *                       within the source pixmap
 * @param src_width      width of the region to be transformed
 * @param src_height     height of the region to be transformed
 * @param transform      transformation to apply
 * @param hasAlpha       whether the source's alpha channel is to be
 *                       processed
 *
 */
extern
void get_transformed_pixmap(QPixmap* originalPixmap,
			    QPixmap* destPixmap,
			    int src_x, int src_y, 
			    int src_width, int src_height,
			    int transform,
			    bool hasAlpha);

/**
 * Applies the given transform to the passed QImage
 */
extern
void transform_image(QImage* srcQImage, int transform);

#endif /*_GXPPORTQT_INTERN_GRAPHICS_UTIL_H__*/
