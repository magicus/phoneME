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
#include "gxj_putpixel.h"
#include "gxapi_graphics.h"
#include "imgapi_image.h"

extern void midpGL_flush(int dirtyRegions[], int numRegions);

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
    static jfieldID surface_fid = 0;
    jint surfaceId;

    // call into midpGL_createPbufferSurface();
    KNI_StartHandles(1);
    KNI_DeclareHandle(imgHandle);
    KNI_GetParameterAsObject(1, imgHandle);    
    midpGL_createPbufferSurface(IMGAPI_GET_IMAGE_PTR(imgHandle));
    //IMGAPI_GET_IMAGE_PTR(imgHandle)->nativeSurfaceId = surfaceId;
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
    static jfieldID pixel_data_fid = 0;
    static jfieldID native_surface_id_fid = 0;
    const java_imagedata *srcImageDataPtr;
    gxj_screen_buffer srcSBuf;  
    gxj_screen_buffer *psrcSBuf;
    jint surfaceId;

    jint ystart = KNI_GetParameterAsInt(2);
    jint yend = KNI_GetParameterAsInt(3);

    KNI_StartHandles(1);
    KNI_DeclareHandle(imgHandle);
    KNI_GetParameterAsObject(1, imgHandle);
    srcImageDataPtr = IMGAPI_GET_IMAGE_PTR(imgHandle)->imageData;
    psrcSBuf = gxj_get_image_screen_buffer_impl(srcImageDataPtr, 
                                                &srcSBuf, NULL);
    midpGL_flushPbufferSurface(IMGAPI_GET_IMAGE_PTR(imgHandle), 
                               srcSBuf.pixelData, ystart, yend);
    KNI_EndHandles();
    KNI_ReturnVoid();
}
