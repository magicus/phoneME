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

/**
 * @file
 *
 * Implementation of Java native methods for the <tt>ImageData</tt> class.
 */

#include <commonKNIMacros.h>

#include <midpEventUtil.h>

#include <gx_image.h>
#include <gxpport_immutableimage.h>

/**
 * Releases any native resources used by this immutable <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     finalize()V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageData_finalize() {
    java_imagedata * imageDataPtr = NULL;
    gxpport_image_native_handle h;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    imageDataPtr = GXAPI_GET_IMAGEDATA_PTR(thisObject);

    /*
     * Image objects with NULL nativeImageData could exist when loading
     * romized image but failed.
     */
    h = (gxpport_image_native_handle)imageDataPtr->nativeImageData;
    if (h != NULL) {
	if (imageDataPtr->isMutable) {
	    gxpport_destroy_mutable(h);
	} else {
	    gxpport_destroy_immutable(h);
	}
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
