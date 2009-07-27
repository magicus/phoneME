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
 * All native functions related to com.sun.midp.lcdui.OpenGLEnvironment class
 */

#include <sni.h>
#include <kni.h>
#include <jvm.h>
#include <commonKNIMacros.h>
#include <midpEventUtil.h>
#include "javacall_memory.h"
#include "javacall_defs.h"
#include "gxj_putpixel.h"
#include "gxapi_graphics.h"
#include "imgapi_image.h"
#include "jcapp_export.h"

typedef struct {
    javacall_pixel *buffer; // associated image data for this surface
    int surface;
    int context;
    int jsrContext;
    int textureName;
} surface_info;

extern surface_info* midpGL_getSurface(int handle);
extern void midpGL_flush(int dirtyRegions[], int numRegions);
extern int midpGL_createPbufferSurface();
extern void midpGL_flushPbufferSurface(int surfaceId, javacall_pixel *buffer,
                                       int x, int y, int width, int height);
extern void midpGL_setSoftButtonHeight(int nSoftButtonHeight);                                
/**
 *
 * Calls openGL function to prepare for switching from lcdui rendering
 * to rendering with some external API
 * <p>
 * Java declaration:
 * <pre>
 *    gainedForeground0(I)V
 * </pre>
 * Java parameters:
 * <pre>
 *    externalAPI the external API which is preparing to render
 *    dirtyRegions regions of the screen which need to be flushed
 *    numberOfRegions number of dirty regions to be processed
 *    displayId the display ID associated with the Display object
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_flushOpenGL0) {
    jbyte *regionArray;
    jsize arrayLength;

    jint numRegions = KNI_GetParameterAsInt(2);
    jint displayId = KNI_GetParameterAsInt(3);

    if (midpHasForeground(displayId)) {
        // do processing
        KNI_StartHandles(1);
        KNI_DeclareHandle(dirtyRegions);

        KNI_GetParameterAsObject(1, dirtyRegions);
        if (KNI_IsNullHandle(dirtyRegions)) {
            KNI_ReturnVoid();
        }
        /* convert dirty regions to a regular integer array */
        arrayLength = KNI_GetArrayLength(dirtyRegions);
        regionArray = javacall_malloc(sizeof(int)*arrayLength);
        KNI_GetRawArrayRegion(dirtyRegions, 0, arrayLength*sizeof(int), 
                              regionArray);
        /* here we need to call midpGL_flush() */
        midpGL_flush(regionArray, numRegions);
        KNI_EndHandles();
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_createPbufferSurface0) {
    gxj_screen_buffer srcSBuf;
    gxj_screen_buffer *psrcSBuf;
    const java_imagedata *srcImageDataPtr;

    // call into midpGL_createPbufferSurface();
    KNI_StartHandles(1);
    KNI_DeclareHandle(imgHandle);
    KNI_GetParameterAsObject(1, imgHandle);
    srcImageDataPtr = IMGAPI_GET_IMAGE_PTR(imgHandle)->imageData;
    psrcSBuf = gxj_get_image_screen_buffer_impl(srcImageDataPtr,
                                                &srcSBuf, NULL);
    IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId = 
        midpGL_createPbufferSurface();
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_createPixmapSurface0) {
    java_graphics * gr;
    gxj_screen_buffer *pImageData;
    gxj_screen_buffer screen_buffer;

    // call into midpGL_createPixmapSurface();
    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(1, graphicsHandle);
    if (!KNI_IsNullHandle(graphicsHandle)) {
        java_graphics * gr;
        gr = GXAPI_GET_GRAPHICS_PTR(graphicsHandle);
        if (gr != NULL) {
          pImageData = gxj_get_image_screen_buffer_impl(
                        (gr != NULL && gr->img != NULL)?gr->img->imageData:NULL,
                        &screen_buffer, graphicsHandle);
        }
    }
    midpGL_createPixmapSurface(pImageData);
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_flushPbufferSurface0) {
    byte *imgByteArray;
    jsize arrayLength;
    static jfieldID image_data_fid = 0;
    gxj_screen_buffer srcSBuf;
    gxj_screen_buffer *psrcSBuf;
    const java_imagedata *srcImageDataPtr;
    jint surfaceId;
    static jfieldID pixel_data_fid = 0;
    jint x = KNI_GetParameterAsInt(2);
    jint y = KNI_GetParameterAsInt(3);
    jint width = KNI_GetParameterAsInt(4);
    jint height = KNI_GetParameterAsInt(5);

    KNI_StartHandles(1);
    KNI_DeclareHandle(imgHandle);
    KNI_GetParameterAsObject(1, imgHandle);
    srcImageDataPtr = IMGAPI_GET_IMAGE_PTR(imgHandle)->imageData;
    psrcSBuf = gxj_get_image_screen_buffer_impl(srcImageDataPtr, 
                                                &srcSBuf, NULL);
    surfaceId = IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId;
    if (surfaceId == 0)
         surfaceId = midpGL_createPbufferSurface();
    IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId = surfaceId;
    KNI_EndHandles();
    midpGL_flushPbufferSurface(surfaceId, srcSBuf.pixelData, x, y, width, height);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_hasBackingSurface) {
    static jfieldID image_fid = 0;
    jint surfaceId;
    const java_imagedata *srcImageDataPtr;
    gxj_screen_buffer srcSBuf;
    gxj_screen_buffer *psrcSBuf;
    jboolean retval;

    jint width = KNI_GetParameterAsInt(2);
    jint height = KNI_GetParameterAsInt(3);

    KNI_StartHandles(3);
    KNI_DeclareHandle(graphicsHandle);
    KNI_DeclareHandle(tmpHandle);
    KNI_DeclareHandle(imgHandle);
    KNI_GetParameterAsObject(1, graphicsHandle);
    KNI_GetObjectClass(graphicsHandle, tmpHandle);
    image_fid = KNI_GetFieldID(tmpHandle, "img", "Ljavax/microedition/lcdui/Image;");
    if (image_fid == 0) {
        retval = KNI_FALSE;
    } else {
        KNI_GetObjectField(graphicsHandle, image_fid, imgHandle);
        if (KNI_IsNullHandle(imgHandle)) {
            retval = KNI_FALSE;
        }
        else { // has a backing surface
            srcImageDataPtr = IMGAPI_GET_IMAGE_PTR(imgHandle)->imageData;
            psrcSBuf = gxj_get_image_screen_buffer_impl(srcImageDataPtr, 
                                                       &srcSBuf, NULL);
            surfaceId = IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId;     
            if (surfaceId == 0)
                surfaceId = midpGL_createPbufferSurface();
            IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId = surfaceId;    
            retval = KNI_TRUE;
        }
    }
    KNI_EndHandles();
    if (retval == KNI_TRUE)
        midpGL_flushPbufferSurface(surfaceId, srcSBuf.pixelData, 0, 0,
                                   srcSBuf.width, srcSBuf.height);
    KNI_ReturnBoolean(retval);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_getDrawingSurface0) {
    static jfieldID image_fid = 0;
    jint surfaceId = 0;
    jint retval=0;

    jint api = KNI_GetParameterAsInt(2);

    KNI_StartHandles(3);
    KNI_DeclareHandle(graphicsHandle);
    KNI_DeclareHandle(tmpHandle);
    KNI_DeclareHandle(imgHandle);
    KNI_GetParameterAsObject(1, graphicsHandle);
    KNI_GetObjectClass(graphicsHandle, tmpHandle);
    image_fid = KNI_GetFieldID(tmpHandle, "img", "Ljavax/microedition/lcdui/Image;");
    if (image_fid == 0) {
        /* no image fid???  Error */
        retval = 0;
    } else {
        KNI_GetObjectField(graphicsHandle, image_fid, imgHandle);
        if (KNI_IsNullHandle(imgHandle)) {
            retval = midpGL_getWindowSurface(api);
        }
        else { 
            /* the graphics object has an image associated with it, so
             * get the suface associated with that image
             */
            surfaceId = IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId;
            if (surfaceId != 0)
                retval = surfaceId;
            else {
                retval = midpGL_createPbufferSurface();
                IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId = retval;
            }
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(retval);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_initMidpGL) {
    static jfieldID pixel_size_fid;

    jint displayWidth = KNI_GetParameterAsInt(1);
    jint displayHeight = KNI_GetParameterAsInt(2);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
    jcapp_switch_color_depth(1);
    // reset pixel size in java ImageData
    KNI_StartHandles(1);
    KNI_DeclareHandle(imageDataHandle);
    KNI_FindClass("javax/microedition/lcdui/ImageData",
                   imageDataHandle);
    if (imageDataHandle != NULL) {
        pixel_size_fid = KNI_GetStaticFieldID(imageDataHandle, "sizeOfPixel",
                                              "I");
        if (pixel_size_fid != NULL) {
BREWprintf("resetting pixel size to 4\n");
            KNI_SetStaticIntField(imageDataHandle, pixel_size_fid, 4);
        }
    }
    KNI_EndHandles();
#endif

    midpGL_init(displayWidth, displayHeight);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_disableOpenGL0) {

    midpGL_disableOpenGL();
#if ENABLE_DYNAMIC_PIXEL_FORMAT
    jcapp_switch_color_depth(0);
#endif
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_raiseOpenGL0) {

    midpGL_raiseOpenGL();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_lowerOpenGL0) {

    midpGL_lowerOpenGL();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_switchColorDepth0) {
    static jfieldID pixel_size_fid;

    jint param = KNI_GetParameterAsInt(1);
    jint size = param?4:2;
#if ENABLE_DYNAMIC_PIXEL_FORMAT
BREWprintf("calling switchColorDepth with %d\n", param);
    jcapp_switch_color_depth(param);
    // reset pixel size in java ImageDataFactory
    KNI_StartHandles(1);
    KNI_DeclareHandle(imageDataHandle);
    KNI_FindClass("javax/microedition/lcdui/ImageData",
                   imageDataHandle);
    if (imageDataHandle != NULL) {
        pixel_size_fid = KNI_GetStaticFieldID(imageDataHandle, "sizeOfPixel",
                                        "I");
        if (pixel_size_fid != NULL) {
BREWprintf("resetting pixel size to %d\n", size);
            KNI_SetStaticIntField(imageDataHandle, pixel_size_fid, size);
        }
    }
    KNI_EndHandles();
#endif
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_setSoftButtonHeight0) {
    jint softButtonHeight = KNI_GetParameterAsInt(1);
    midpGL_setSoftButtonHeight(softButtonHeight);
    KNI_ReturnVoid();
}
