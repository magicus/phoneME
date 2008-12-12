/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include "JSR239-KNIInterface.h"

#include <string.h>
#include <gxj_putpixel.h>
#include <midp_constants_data.h>
#include <lcdlf_export.h>

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

/**
 * Get a C structure representing the given <tt>ImageData</tt> class.
 */
#define GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(handle) \
  GXAPI_GET_GRAPHICS_PTR(handle)->img != NULL ? \
  GXAPI_GET_GRAPHICS_PTR(handle)->img->imageData : \
  (java_imagedata*)NULL


/**
 * Convenient macro for getting screen buffer from a Graphics's target image.
 */
#define GXJ_GET_GRAPHICS_SCREEN_BUFFER(g,sbuf) \
   gxj_get_image_screen_buffer_impl(GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(g),sbuf,g)


/**
 * Helper function.
 * Retrieve buffer for the specified graphics.
 *
 * @param graphicshandle   A KNI handle to a graphics object.
 * @return A pointer to the graphics buffer.
 */
static gxj_pixel_type* getGraphicsBuffer(jobject graphicsHandle) {
    gxj_screen_buffer sbuf;
    gxj_screen_buffer* gimg;
    gxj_pixel_type* dbuffer;

    gimg = GXJ_GET_GRAPHICS_SCREEN_BUFFER(graphicsHandle, &sbuf);
    if (gimg == NULL) {
        dbuffer = gxj_system_screen_buffer.pixelData;
    } else {
        dbuffer = gimg->pixelData;
    }

    return dbuffer;
}

/* Copy MIDP screen buffer */

void JSR239_getWindowContents(JSR239_Pixmap *dst,
                              jobject srcGraphicsHandle, 
                              jint srcWidth, jint srcHeight,
                              jint deltaHeight) {
    void* src;

    KNI_StartHandles(1);
    KNI_DeclareHandle(GraphicsClassHandle);

#ifdef DEBUG
    printf("JSR239_getWindowContents >>\n");
#endif

    KNI_FindClass("javax/microedition/lcdui/Graphics", GraphicsClassHandle);

    if (!KNI_IsInstanceOf(srcGraphicsHandle, GraphicsClassHandle)) {
#ifdef DEBUG
        printf("JSR239_getWindowContents only implemented for graphicsHandle "
                "instanceof Graphics!\n");
#endif
    } else {
        src = (void*)getGraphicsBuffer(srcGraphicsHandle);

#ifdef DEBUG
        printf("JSR239_getWindowContents:\n");
        printf("  dst        = %d\n", dst->pixels);
        printf("  dst Bpp    = %d\n", dst->pixelBytes);
        printf("  dst width  = %d\n", dst->width);
        printf("  dst height = %d\n", dst->height);
        printf("  src        = %d\n", src);
        printf("  src width  = %d\n", srcWidth);
        printf("  src height = %d\n", srcHeight);
#endif

        /* IMPL_NOTE: get clip sizes into account. */
        copyFromScreenBuffer(dst,
                             src, srcWidth, srcHeight,
                             deltaHeight);
    }

#ifdef DEBUG
    printf("JSR239_getWindowContents <<\n");
#endif

    KNI_EndHandles();
}

/* Copy engine buffer back to MIDP */
void
JSR239_putWindowContents(jobject graphicsHandle,
                         jint delta_height,
                         JSR239_Pixmap *src, 
                         jint clipX, jint clipY, 
                         jint clipWidth, jint clipHeight,
                         jint flipY) {

    void* s;
    void* d;

    KNI_StartHandles(1);
    KNI_DeclareHandle(GraphicsClassHandle);

#ifdef DEBUG
    printf("JSR239_putWindowContents >>\n");
#endif

    KNI_FindClass("javax/microedition/lcdui/Graphics", GraphicsClassHandle);
    if (!KNI_IsInstanceOf(graphicsHandle, GraphicsClassHandle)) {
#ifdef DEBUG
        printf("JSR239_putWindowContents only implemented for graphicsHandle "
               "instanceof Graphics!\n");
#endif
    } else {
        gxj_screen_buffer sbuf;
        gxj_screen_buffer* gimg;

        // Obtain the dimensions of the destination.
        // Revisit: multiple displays support. Obtain Id of display render surfane is
        // bound to. Consider recalculations when display got changed.
        int displayId    = lcdlf_get_current_hardwareId();
        jint dest_width  = lcdlf_get_screen_width(displayId);
        jint dest_height = lcdlf_get_screen_height(displayId);
        jint min_height  = 0;
        gxj_pixel_type* srcPtr;
        gxj_pixel_type* dstPtr;
        
        gimg = GXJ_GET_GRAPHICS_SCREEN_BUFFER(graphicsHandle, &sbuf);
        if (gimg != NULL) {
            dest_width = gimg->width;
            dest_height= gimg->height;
        }                

        /* src->screen_buffer is an output of copyToScreenBuffer function. */
        s = (void*)src->screen_buffer;
        d = (void*)getGraphicsBuffer(graphicsHandle);

        min_height = (dest_height > src->height) ? src->height : dest_height;

#ifdef DEBUG
        printf("JSR239_putWindowContents:\n"); 
        printf("  src        = %d\n", src->pixels);
        printf("  src Bpp    = %d\n", src->pixelBytes);
        printf("  src width  = %d\n", src->width);
        printf("  src height = %d\n", src->height);
        printf("  min height = %d\n", min_height);
        printf("  dst        = %d\n", d);     
        printf("  dst width  = %d\n", clipWidth);
        printf("  dst height = %d\n", clipHeight);
#endif

        srcPtr = (gxj_pixel_type *)s;
        dstPtr = (gxj_pixel_type *)d;

        if ((dest_width * min_height <= src->width * src->height) && 
            (sizeof(gxj_pixel_type) == 2)) {
            /* Source data must be in 16bit 565 format. */
           JSR239_memcpy(s, d,
               dest_width * min_height * sizeof(gxj_pixel_type));

           /* IMPL_NOTE: get clip sizes into account. */
           copyToScreenBuffer(src, delta_height, 
                              clipX, clipY, clipWidth, clipHeight,
                              dest_width, dest_height,
                              flipY);

            /* Source data must be in 16bit 565 format. */
           JSR239_memcpy(d, s,
               dest_width * min_height * sizeof(gxj_pixel_type));        
        } else {
#ifdef DEBUG
        printf("JSR239: offscreen buffer data is incorrect.\n");
#endif
        }
    }

#ifdef DEBUG
    printf("JSR239_putWindowContents <<\n");
#endif

    KNI_EndHandles();
}
