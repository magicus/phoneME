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
 *
 * Implementation of Java native methods for the <tt>ImageData</tt> class.
 */

#include <commonKNIMacros.h>

#include <midpEventUtil.h>

#include <imgapi_image.h>
#include <gxpport_immutableimage.h>



#include <stdlib.h>
#include <sni.h>
#include <midpError.h>

#include <imgapi_image.h>
#include <img_errorcodes.h>


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

    imageDataPtr = IMGAPI_GET_IMAGEDATA_PTR(thisObject);

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


/**
 * Gets an ARGB integer array from this <tt>ImmutableImage</tt>. The
 * array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     getRGB([IIIIIII)V
 * </pre>
 *
 * @param rgbData The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlen Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_getRGB) {
    int height = KNI_GetParameterAsInt(7);
    int width = KNI_GetParameterAsInt(6);
    int y = KNI_GetParameterAsInt(5);
    int x = KNI_GetParameterAsInt(4);
    int scanlength = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int *rgbBuffer;
    java_imagedata *srcImageDataPtr;
    gxpport_mutableimage_native_handle srcImageNativeData;
    img_native_error_codes error = IMG_NATIVE_IMAGE_NO_ERROR;

    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetThisPointer(thisObject);


    SNI_BEGIN_RAW_POINTERS;
    
    rgbBuffer = JavaIntArray(rgbData);

    srcImageDataPtr = IMGAPI_GET_IMAGEDATA_PTR(thisObject);
    srcImageNativeData =
      (gxpport_mutableimage_native_handle)srcImageDataPtr->nativeImageData;    

    if (srcImageDataPtr->isMutable) {
        gxpport_get_mutable_argb(srcImageNativeData,
                                 rgbBuffer, offset, scanlength,
                                 x, y, width, height, 
                                 &error);
    } else {
        gxpport_get_immutable_argb(srcImageNativeData,
                                   rgbBuffer, offset, scanlength,
                                   x, y, width, height, 
                                   &error);
    }

    SNI_END_RAW_POINTERS;
    
    if (error != IMG_NATIVE_IMAGE_NO_ERROR) {
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
