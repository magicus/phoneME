/*
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

#include "JSR239-KNIInterface.h"

#include <string.h>
#include <gxapi_graphics.h>
#include <gxpportqt_image.h>
#include <qteapp_export.h>


/*
 * Structure representing the <tt>Image</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_Image java_image;


#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

/**
 * Helper function.
 * Retrieve buffer for the specified graphics.
 *
 * @param graphicshandle   A KNI handle to a graphics object.
 * @return QPixmap that represents the graphics buffer.
 */
static QPixmap* getGraphicsBuffer(jobject graphicsHandle) {
    QPixmap* pixmap;
    java_image* image;

    image = GXAPI_GET_GRAPHICS_PTR(graphicsHandle)->img;
    if (image != NULL) {
        /*
         * If the graphics context has a parent image bound to it,
         * pick up the pixmap.
         */
        pixmap = gxpportqt_get_mutableimage_pixmap(
            (gxpport_mutableimage_native_handle)
                (image->imageData->nativeImageData));

    } else {
        /* If no image is bound, render to back buffer */
        pixmap = qteapp_get_mscreen()->getBackBuffer();
    }

    return pixmap;
}

/* Copy MIDP screen buffer */

extern "C"
void
JSR239_getWindowContents(jobject graphicsHandle, jint deltaHeight,
    JSR239_Pixmap *dst) {

    QPixmap* pixmap;
    void* src;

    KNI_StartHandles(1);
    KNI_DeclareHandle(GraphicsClassHandle);

#ifdef DEBUG
    printf("JSR239_getWindowContents >>\n");
#endif

    KNI_FindClass("javax/microedition/lcdui/Graphics", GraphicsClassHandle);

    if (!KNI_IsInstanceOf(graphicsHandle, GraphicsClassHandle)) {
#ifdef DEBUG
        printf("JSR239_getWindowContents only implemented for graphicsHandle "
                "instanceof Graphics!\n");
#endif
    } else {
        pixmap = getGraphicsBuffer(graphicsHandle);

#ifdef DEBUG
        printf("JSR239_getWindowContents: pixmap=%p\n", pixmap);
        printf("  pixmap Bpp = %d\n", pixmap->depth()/8);
        printf("  pixmap width  = %d\n", pixmap->width());
        printf("  pixmap height = %d\n", pixmap->height());
        printf("  dst Bpp = %d\n", dst->pixelBytes);
        printf("  dst width  = %d\n", dst->width);
        printf("  dst height = %d\n", dst->height);
#endif

        src = (void*)pixmap->scanLine(0);

        /* IMPL_NOTE: get clip sizes into account. */
        copyFromScreenBuffer(dst, src, 0, 0, dst->width, dst->height,
            deltaHeight);
    }

#ifdef DEBUG
    printf("JSR239_getWindowContents <<\n");
#endif

    KNI_EndHandles();
}

/* Copy engine buffer back to MIDP */

extern "C"
void
JSR239_putWindowContents(jobject graphicsHandle,
                         jint delta_height,
                         JSR239_Pixmap *src, 
                         jint clipX, jint clipY, jint clipWidth, jint clipHeight,
                         jint flipY) {

    void* s;
    void* d;
    QPixmap* pixmap;

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
        const jint bytes_for_depth = 2;
        const jint bits_per_byte = 8;
        jint min_height;

        pixmap = getGraphicsBuffer(graphicsHandle);

        min_height = (pixmap->height() > src->height) ? src->height :
            pixmap->height();

#ifdef DEBUG
        printf("JSR239_putWindowContent: pixmap=%p\n", pixmap);
        printf("  pixmap Bpp = %d\n",  pixmap->depth()/bits_per_byte);
        printf("  pixmap width  = %d\n", pixmap->width());
        printf("  pixmap height = %d\n", pixmap->height());
        printf("  src Bpp = %d\n", src->pixelBytes);
        printf("  src width = %d\n", src->width);
        printf("  src height = %d\n", src->height);
        printf("  min height = %d\n", min_height);
#endif

        /* IMPL_NOTE: get clip sizes into account. */
        copyToScreenBuffer(src, delta_height, 
                           clipX, clipY, 
                           clipWidth, clipHeight, 
                           flipY);

        /* src->screen_buffer is an output of copyToScreenBuffer function. */
        s = (void*)src->screen_buffer;
        d = (void*)pixmap->scanLine(0);

        if ((pixmap->width() != src->width) ||
            (pixmap->depth() != bits_per_byte * bytes_for_depth)) {
#ifdef DEBUG
            printf("JSR239: offscreen buffer data is incorrect.\n");
#endif
        } else {
            /* Source data must be in 16bit 565 format. */
            JSR239_memcpy(
                d, s, pixmap->width() * min_height * bytes_for_depth);
        }
    }

#ifdef DEBUG
    printf("JSR239_putWindowContents <<\n");
#endif

    KNI_EndHandles();
}
