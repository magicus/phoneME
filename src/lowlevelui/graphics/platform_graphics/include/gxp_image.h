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
 * @file
 * @ingroup lowui_port
 *
 * @brief Porting interface for immutable image handling
 */

#ifndef _GXP_IMAGE_H_
#define _GXP_IMAGE_H_

#include <gxpport_immutableimage.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves native image handle from a Java platform ImageData object.
 * <p>
 * <b>NOTE:</b> One should be very careful with this function. We are
 * returning a pointer to the inside of an object. If the GC moves the
 * object, this pointer will be invalid. Great care must be taken to
 * not allow a GC to happen after calling this function.
 *
 * @param img a handle to the <tt>ImageData</tt> Java platform object
 *            that contains the raw image data
 * @return A pointer to the raw data associated with the given
 *         <tt>ImageData</tt> object.
 */
gxpport_image_native_handle gxp_get_imagedata(jobject img);


#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* _GXP_IMAGE_H_  */
