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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <qpixmap.h>
#include <qbitmap.h>
#include <qteapp_export.h>
#include <midpResourceLimit.h>
#include <midp_logging.h>

#include <gxpport_mutableimage.h>
#include <gxpportqt_image.h>
#include "gxpportqt_intern_graphics_util.h"
#include <qpainter.h>
#include <qimage.h>
#include <qpaintdevice.h>

#include <imgapi_image.h>
#include <gxapi_graphics.h>
#include <gx_image.h>

/**
 * Pisces help functions. Used in Pisces to enable direct rendering into
 * midp mutable image.
 *
 */

/**
 * Type imdata. used to exchange image data buffer info between PISCES and phoneME.
 *
 * @struct _imdata
 */
typedef struct _imdata {
    int w;
    int h;
    int d;
    void * data;
} imdata;

/**
 * Fill idata with pointer to pixel data buffer, width, height and bit depth info.
 * Idata must be preallocated. Pisces use this function to get acces to image data for
 * direct rendering. imagedata parameter is nativeImageData field of midp mutable image data.
 *
 * @param imagedata MIDP mutable-image data nativeImageData field
 * @param idata output variable
 */
extern "C" void
get_pixmap(int imagedata, imdata * idata ) {
    QPixmap *pmap = (QPixmap *) imagedata;
    if (idata == NULL) return;
    if (pmap != NULL) {
        idata->w = pmap->width();
        idata->h = pmap->height();
        idata->d = pmap->depth();
        idata->data = (void *) pmap->scanLine(0);
    } else {
        idata->w = 0;
        idata->h = 0;
        idata->d = 0;
        idata->data = NULL;
    }
}

/**
 * Convenience function. Does the same as get_pixmap(...) with the distinction, that it takes
 * _MidpImage pointer as the first parameter.
 *
 * @param image pointer to MIDP mutable image
 * @param idata output variable
 */
extern "C" void get_pixmap_from_midp_image(jobject * image, imdata * idata) {
    QPixmap *pmap = NULL;
    if (idata == NULL) return;
    if(image != NULL) {
        pmap = (QPixmap *) ((_MidpImage *) image)->imageData->nativeImageData;
    } else {
        pmap = qteapp_get_mscreen()->getBackBuffer();
    }

    if (pmap != NULL) {
        idata->w = pmap->width();
        idata->h = pmap->height();
        idata->d = pmap->depth();
        idata->data = (void *) pmap->scanLine(0);
    } else {
        idata->w = 0;
        idata->h = 0;
        idata->d = 0;
        idata->data = NULL;
    }
}

