/*
 *
 *
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

#include <gxpport_mutableimage.h>
#include <midp_logging.h>
#include <syslog.h>
#include "lfpport_gtk.h"

extern GtkWidget *main_window;
GdkPixmap *back_buffer;


#define BITS_PER_PIXEL 8


/**
 * Initializes the internal members of the native image structure, as required
 * by the platform.
 *
 * @param newImagePtr structure to hold the image's native representation.
 * @param width width of the image, in pixels.
 * @param height height of the image, in pixels.
 * @param creationErrorPtr pointer for the status of the decoding
 *        process. This function sets creationErrorPtr's value.
 */
void gxpport_create_mutable(gxpport_mutableimage_native_handle *newImagePtr,
                                            int width,
                                            int height,
                                            img_native_error_codes*
                                            creationErrorPtr) {
    GdkPixmap *gdk_pix_map;
    int nwidth, nheight;
    LIMO_TRACE(">>>%s width=%d height=%d\n", __FUNCTION__, width, height);

    /* Suppress unused parameter warnings */
    gdk_pix_map = gdk_pixmap_new(NULL, width, height, 24);
    *newImagePtr = gdk_pix_map;
    back_buffer = gdk_pix_map;

    gdk_drawable_get_size(gdk_pix_map, &nwidth, &nheight);
    LIMO_TRACE("%s gdk_pix_map size: width=%d height=%d\n", __FUNCTION__, nwidth, nheight);

    /* Not yet implemented */
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
    LIMO_TRACE("<<<%s newImagePtr=%x\n", __FUNCTION__, *newImagePtr);
}

/**
 * Renders the contents of the specified mutable image
 * onto the destination specified.
 *
 * @param srcImagePtr         pointer to mutable source image
 * @param dstImagePtr         pointer to mutable destination image or
 *                            NULL for the screen
 * @param clip                pointer to structure holding the clip
 *                                [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 */
void gxpport_render_mutableimage(gxpport_mutableimage_native_handle srcImagePtr,
				 gxpport_mutableimage_native_handle dstImagePtr,
				 const jshort *clip,
				 int x_dest, int y_dest) {
    guchar *pixels;
    int rowstride;
    GdkPixbuf *gdkPixBuf;
    GtkWidget *form;
    GtkWidget *da;


    LIMO_TRACE(">>>%s srcImagePtr=%x dstImagePtr=%x clip[0]=%d clip[1]=%d clip[2]=%d clip[3]=%d\n",
               __FUNCTION__, srcImagePtr, dstImagePtr, clip[0], clip[1], clip[2], clip[3]);

    //if src is GdkPixbuf
    if (GDK_IS_PIXBUF(srcImagePtr)) {
        LIMO_TRACE("%s rendering pixbuf\n", __FUNCTION__);
        gdkPixBuf = srcImagePtr;
        rowstride = gdk_pixbuf_get_rowstride(gdkPixBuf);
        pixels = gdk_pixbuf_get_pixels(gdkPixBuf);

        gdk_draw_rgb_image_dithalign(dstImagePtr,    /* drawable */
                                    main_window->style->black_gc, /*gc*/
                                    clip[0], clip[1],   /*top left x, y*/
                                    clip[2], clip[3], /*width, height */
                                    GDK_RGB_DITHER_NORMAL,
                                    pixels, rowstride,
                                    0, 0);/*offset for dither*/

    }
    else if (GDK_IS_PIXMAP(srcImagePtr)) {
        LIMO_TRACE("%s rendering pixmap\n", __FUNCTION__);
        /* TODO:  replace main_window with something real */

        if (!dstImagePtr) {

            form = gtk_main_window_get_current_form(main_window);
            da = gtk_object_get_user_data(form);

            gdk_draw_drawable(da->window,
                 main_window->style->black_gc,
                 srcImagePtr,
                 clip[0],   /* x */
                 clip[1],   /* y */
                 x_dest,
                 y_dest,
                 clip[2],   /* width */
                 clip[3]);  /* height */
        }
        else {
            //TODO
        }

    } else {
        LIMO_TRACE("%s unknown source\n", __FUNCTION__);
    }

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param srcImagePtr         pointer to mutable source image
 * @param dstImagePtr         pointer to mutable destination image or
 *                            NULL for the screen
 * @param clip                pointer to structure holding the clip
 *                                [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 */
void
gxpport_render_mutableregion(gxpport_mutableimage_native_handle srcImagePtr,
			     gxpport_mutableimage_native_handle dstImagePtr,
			     const jshort *clip,
			     int x_dest, int y_dest,
			     int width, int height,
			     int x_src, int y_src,
			     int transform) {
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);

    /* Suppress unused parameter warnings */
    (void)srcImagePtr;
    (void)dstImagePtr;
    (void)clip;
    (void)x_dest;
    (void)y_dest;
    (void)width;
    (void)height;
    (void)x_src;
    (void)y_src;
    (void)transform;
}

/**
 * Gets ARGB representation of the specified mutable image
 *
 * @param imagePtr      pointer to the mutable source image
 * @param rgbBuffer     pointer to buffer to write with the ARGB data
 * @param offset        offset in the buffer at which to start writing
 * @param scanLength    the relative offset within the array
 *                      between corresponding pixels of consecutive rows
 * @param x             x-coordinate of region
 * @param y             y-coordinate of region
 * @param width         width of region
 * @param height        height of region
 * @param errorPtr Error status pointer to the status
 *                 This function sets creationErrorPtr's value.
 */
void gxpport_get_mutable_argb(gxpport_mutableimage_native_handle imagePtr,
			      jint* rgbBuffer, int offset, int scanLength,
			      int x, int y, int width, int height,
			      img_native_error_codes* errorPtr) {

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);

    /* Suppress unused parameter warning */
    (void)imagePtr;
    (void)rgbBuffer;
    (void)offset;
    (void)scanLength;
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    /* Not yet implemented */
    *errorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
}

/**
 * Cleans up any native resources to prepare the image to be garbage collected.
 *
 * @param imagePtr the mutable image.
 * @param errorPtr pointer for a status code set on return
 */
void gxpport_destroy_mutable(gxpport_mutableimage_native_handle imagePtr) {
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);

    /* Suppress unused parameter warning */
    (void)imagePtr;
}
