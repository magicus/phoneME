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


#include <PiscesDefs.h>
#include <PiscesUtil.h>
#include <PiscesSurface.h>
#include <KNIUtil.h>
#include <JAbstractSurface.h>

#include <pcsl_memory.h>

#include <gxapi_graphics.h>
#include <gx_graphics.h>

#include <sni.h>
#include <commonKNIMacros.h>

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

static jboolean pisces_drawRGB(jobject graphicsHandle,
                               jint* argb, jint scanLength,
                               jint x, jint y,
                               jint width, jint height,
                               jfloat opacity);

static void premulOpacity(jint* srcArray, jint scanLength,
                          jint width, jint height,
                          jfloat opacity,
                          jint* dstArray);

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_GraphicsSurfaceDestination_initialize() {

    /* don't do anything here (see the throw above)! */

    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_GraphicsSurfaceDestination_drawSurfaceImpl() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(graphicsHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint srcX      = KNI_GetParameterAsInt(3);
    jint srcY      = KNI_GetParameterAsInt(4);
    jint dstX      = KNI_GetParameterAsInt(5);
    jint dstY      = KNI_GetParameterAsInt(6);
    jint width     = KNI_GetParameterAsInt(7);
    jint height    = KNI_GetParameterAsInt(8);
    jfloat opacity = KNI_GetParameterAsFloat(9);

    Surface* surface;

    KNI_GetParameterAsObject(1, graphicsHandle);
    KNI_GetParameterAsObject(2, surfaceHandle);

    surface = &surface_get(surfaceHandle)->super;

    CORRECT_DIMS(surface, srcX, srcY, width, height, dstX, dstY);

    if ((width > 0) && (height > 0) && (opacity > 0)) {
        jboolean retVal;

        ACQUIRE_SURFACE(surface, surfaceHandle);
        retVal =
            pisces_drawRGB(graphicsHandle,
                           (jint*)surface->data + srcY * surface->width + srcX,
                           surface->width,
                           dstX, dstY,
                           width,
                           height,
                           opacity);
        RELEASE_SURFACE(surface, surfaceHandle);

        if (KNI_FALSE == retVal) {
            KNI_ThrowNew("java/lang/RuntimeError",
                         "Renderer error : drawRGB failed.");
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_GraphicsSurfaceDestination_drawRGBImpl() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(graphicsHandle);
    KNI_DeclareHandle(arrayHandle);

    jint offset     = KNI_GetParameterAsInt(3);
    jint scanLength = KNI_GetParameterAsInt(4);
    jint x          = KNI_GetParameterAsInt(5);
    jint y          = KNI_GetParameterAsInt(6);
    jint width      = KNI_GetParameterAsInt(7);
    jint height     = KNI_GetParameterAsInt(8);
    jfloat opacity  = KNI_GetParameterAsFloat(9);

    KNI_GetParameterAsObject(1, graphicsHandle);
    KNI_GetParameterAsObject(2, arrayHandle);

    if ((width > 0) && (height > 0) && (opacity > 0)) {
        jint* tempArray;

        SNI_BEGIN_RAW_POINTERS;

        tempArray = &JavaIntArray(arrayHandle)[offset];

        pisces_drawRGB(graphicsHandle, tempArray, scanLength,
                       x, y, width,
                       height, opacity);

        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * A wrapper for MIDP's lowlevelui/graphics/drawRGB()
 * Premultiplies Alpha by 'opacity'
 * Translates by destination Graphics object's translation before drawing.
 *
 */
static jboolean pisces_drawRGB(jobject graphicsHandle,
                               jint* argb, jint scanLength,
                               jint x, jint y,
                               jint width, jint height,
                               jfloat opacity) {

    jboolean retVal = KNI_FALSE; //assume failure
    jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

    GXAPI_TRANSLATE(graphicsHandle, x, y);
    GXAPI_GET_CLIP(graphicsHandle, clip);

    if (opacity < 1.0) {
        jint* tempArray = (jint*)pcsl_mem_malloc(width * height * sizeof(jint));

        if (NULL == tempArray) {
            //Failed!
            //retVal = KNI_FALSE;
        } else {

            /* Premultiply the alpha value in the pixels with opacity */
            /* IMPL_NOTE : this is ineffcient and allocates memory. 
               Make efficient. */
            premulOpacity(argb, scanLength,
                          width, height,
                          opacity,
                          tempArray);

            gx_draw_rgb(clip,
                        GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(graphicsHandle),
                        tempArray,
                        0,
                        scanLength,
                        x,
                        y,
                        width, height,
                        KNI_TRUE);

            pcsl_mem_free(tempArray);

            retVal = KNI_TRUE;
        }

    } else {
        gx_draw_rgb(clip,
                    GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(graphicsHandle),
                    argb,
                    0,
                    scanLength,
                    x,
                    y,
                    width, height,
                    KNI_TRUE);

        retVal = KNI_TRUE;
    }

    return retVal;

}

/**
 * Premultiplies the pixels in ARGB8888 srcArray
 * by the value opacity 0<=opacity<=1.0
 * and writes the output into the ARGB8888 dstArray
 *
 */
static void premulOpacity(jint* srcArray, jint scanLength,
                          jint width, jint height,
                          jfloat opacity,
                          jint* dstArray) {

    jint op = (jint)(0x100 * opacity);
    jint scanRest = scanLength - width;
    jint sidx = 0; /* offset is 0 */
    jint didx = 0;
    jint h, sidx2;

    for (h = height; h > 0; --h) {
        for (sidx2 = sidx + width; sidx < sidx2; ++sidx) {
            unsigned int pixel = srcArray[sidx];
            unsigned int alpha = ((pixel >> 24) * op + 128) >> 8;
            dstArray[didx++] = (jint)((pixel & 0xffffff) | (alpha << 24));
        }
        sidx += scanRest;
    }

}
