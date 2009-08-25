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


#include <stdlib.h>
#include <sni.h>
#include <midpError.h>

#include <imgapi_image.h>
#include <img_errorcodes.h>
#include "imgj_rgb.h"

#if ENABLE_DYNAMIC_PIXEL_FORMAT
/** Convert 32-bit (ABGR) color to 16bit (565) color */
#define GXJ_ABGR32TORGB16(x) ( (((x) & 0x000000F8) << 8) | \
                               (((x) & 0x0000FC00) >> 5) | \
		               (((x) & 0x00F80000) >> 19) )

#define GXJ_RGB16TOARGB32(x) ( (((x) & 0x001F) << 3) | (((x) & 0x001C) >> 2) |\
                               (((x) & 0x07E0) << 5) | (((x) & 0x0600) >> 1) |\
                               (((x) & 0xF800) << 8) | (((x) & 0xE000) << 3) |\
                               0xFF000000 )

#define GXJ_ARGB32TOABGR32(x) ( ((x) << 8) | (((x) >> 24) & 0xFF) )
#endif

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
    img_native_error_codes error;


    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetThisPointer(thisObject);


    error = IMG_NATIVE_IMAGE_NO_ERROR;

    SNI_BEGIN_RAW_POINTERS;
    
    rgbBuffer = JavaIntArray(rgbData);
    imgj_get_argb(IMGAPI_GET_IMAGEDATA_PTR(thisObject), rgbBuffer,
		  offset, scanlength,
		  x, y, width, height, &error);

    SNI_END_RAW_POINTERS;
    
    if (error != IMG_NATIVE_IMAGE_NO_ERROR) {
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Converts pixel format between 16bit (RGB, 565) and 32bit (ABGR, 8888).
 *
 * @param srcData source array of pixels
 * @param dstData target array of pixels
 * @param totalPixels how many pixels are in srcData
 * @param to32bit true if asked to convert from 16bit to 32bit,
 *                false if the coversion is 32bit->16bit
 */
/* private native void convertPixels(byte[] srcData, byte[] dstData, int totalPixels, boolean to32bit) */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_convertPixels) {
#if ENABLE_DYNAMIC_PIXEL_FORMAT
    jbyte *srcArray, *dstArray;
    int i;
    jboolean to32bit = KNI_GetParameterAsBoolean(4);
    int totalPixels  = KNI_GetParameterAsInt(3);

    KNI_StartHandles(2);
    KNI_DeclareHandle(srcData);
    KNI_DeclareHandle(dstData);

    KNI_GetParameterAsObject(2, dstData);
    KNI_GetParameterAsObject(1, srcData);

    SNI_BEGIN_RAW_POINTERS;

    srcArray = JavaByteArray(srcData);
    dstArray = JavaByteArray(dstData);

    if (to32bit == KNI_TRUE) {
        char* src = (char*)srcArray;
        int* dst = (int*)dstArray;
        for (i = 0; i < totalPixels; i++) {
           *dst = GXJ_ARGB32TOABGR32(GXJ_RGB16TOARGB32(*src));
           src++;
           dst++;
        }
    } else {
        int* src = (int*)srcArray;
        char* dst = (char*)dstArray;
        for (i = 0; i < totalPixels; i++) {
           *dst = GXJ_ABGR32TORGB16(*src);
           src++;
           dst++;
        }
    }

    SNI_END_RAW_POINTERS;

    KNI_EndHandles();
#endif
    KNI_ReturnVoid();
}
