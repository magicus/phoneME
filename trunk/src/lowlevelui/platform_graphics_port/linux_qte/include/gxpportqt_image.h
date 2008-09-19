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

#ifndef _GXPPORTQT_IMAGE_H_
#define _GXPPORTQT_IMAGE_H_

/**
 * @file
 * Declares the platform image structures used by the shared code
 */

#include <qpixmap.h>

#include <gxpport_immutableimage.h>
#include <gxpport_mutableimage.h>

/**
 * Returns a pointer to a QPixmap that represents this ImmutableImage.
 * Note : This function may create a QPixmap object.
 */
extern
#ifdef __cplusplus
"C"
#endif
QPixmap* 
gxpportqt_get_immutableimage_pixmap
(gxpport_image_native_handle immutableImage);

/**
 * Returns a pointer to a QPixmap of a MutableImage
 */
extern
#ifdef __cplusplus
"C"
#endif
QPixmap* 
gxpportqt_get_mutableimage_pixmap
(gxpport_mutableimage_native_handle mutableImage);

#endif /*_GXPPORTQT_IMAGE_H_*/
