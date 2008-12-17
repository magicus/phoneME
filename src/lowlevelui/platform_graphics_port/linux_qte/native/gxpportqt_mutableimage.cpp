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

/**
 * Calculate image resource size that should be used for resource limit
 * checking. Image color depth is hard-coded to 32 as it was found that 
 * color depth of resulting image was changed to 32-bit(from 16 or lower size) 
 * in most of the cases. 
 */ 
#define ImgRscSize(img) (((img)->width()*(img)->height()*32)>>3)

/**
 * Calculate image region resource size that should be used for resource limit
 * checking. Image color depth is hard-coded to 32 as it was found that 
 * color depth of resulting image was changed to 32-bit(from 16 or lower size) 
 * in most of the cases. 
 */ 
#define ImgRegionRscSize(width, height) ((width*height*32)>>3)

extern "C" void
gxpport_create_mutable(gxpport_mutableimage_native_handle *newImagePtr,
		       int width, int height,
		       img_native_error_codes* creationErrorPtr) {
  /* set optimization for all QPixmaps
     move this to startup/initialization code. */
  QPixmap::setDefaultOptimization(QPixmap::BestOptim);

  /*
   * Populate Java data pointer before calling checkResourceLimit
   * because it might trigger GC and makes the Java pointer invalid.
   */  
  QPixmap* qpixmap = new QPixmap(); /* Start with a NULL image object */
  if (NULL == qpixmap) {
     *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
     return;
  }

  /* Check resource limit before really creating image memory */
  if (midpCheckResourceLimit(RSC_TYPE_IMAGE_MUT,
                             ImgRegionRscSize(width, height)) == 0) {
     *creationErrorPtr = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
     delete qpixmap;
     return;
  }

  /* ALLOCATE MEMORY FOR QT IMAGE DATA */
  qpixmap->resize(width, height);

  /* INITIALIZE CONTENTS TO WHITE */
  qpixmap->fill();

  /* Update the resource count */
  if (midpIncResourceCount(RSC_TYPE_IMAGE_MUT, ImgRscSize(qpixmap)) == 0) {
      REPORT_INFO(LC_LOWUI,"Error in updating resource"
                  " limit for Mutable image");
  }

  /* return the qpixmap pointer */
  *newImagePtr = qpixmap;

  *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" void
gxpport_render_mutableimage(gxpport_mutableimage_native_handle srcImagePtr,
			    gxpport_mutableimage_native_handle dstImagePtr,
			    const jshort *clip,
			    jint x_dest, jint y_dest) {

    QPixmap *qpixmapDst = (QPixmap *)(dstImagePtr);

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, -1, clip, 
                                    (QPaintDevice *)qpixmapDst, 
                                    0);

    QPixmap *qpixmapSrc = (QPixmap *)(srcImagePtr);

    gc->drawPixmap(x_dest, y_dest, *qpixmapSrc);
}

extern "C" void
gxpport_render_mutableregion(gxpport_mutableimage_native_handle srcImagePtr,
			     gxpport_mutableimage_native_handle dstImagePtr,
			     const jshort *clip,
			     jint x_dest, jint y_dest, 
			     jint width, jint height,
			     jint x_src, jint y_src,
			     jint transform) {
    QPixmap *qpixmapDst = (QPixmap *)(dstImagePtr);
    
    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, -1, clip, 
                                    (QPaintDevice *)qpixmapDst, 
                                    0);

    QPixmap *qpixmapSrc = (QPixmap *)(srcImagePtr);

    if (NULL == qpixmapSrc || qpixmapSrc->isNull()) {
        /* not a valid pixmap. */
        return;
    }

    if (0 == transform) {
        gc->drawPixmap(x_dest, y_dest,
                       *qpixmapSrc,
                       x_src, y_src,
                       width, height);
        return;
    }

    QPixmap *qpixmap = new QPixmap();

    get_transformed_pixmap(qpixmapSrc, qpixmap,
			   x_src, y_src, width, height,
			   transform, FALSE);

    gc->drawPixmap(x_dest, y_dest, *qpixmap);
    delete qpixmap;
}

extern "C" void
gxpport_destroy_mutable(gxpport_mutableimage_native_handle imagePtr) {

    /* we know : 
       non-null QPixmap object has been allocated */  
    QPixmap* qpixmap = (QPixmap *)(imagePtr);

    /* make sure there is a valid qpixmap */
    if (qpixmap == NULL) {
	return;
    }

    MScreen * mscreen = qteapp_get_mscreen();
    if (mscreen->isCurrentPaintDevice(qpixmap)) { 
        // remove pixmap as the current painting device 
        short clip[] = {0, 0, 0, 0}; 
        mscreen->setupGC(-1, -1, clip, NULL, 0); 
    } 

    /* Update the resource count */
    if (midpDecResourceCount(RSC_TYPE_IMAGE_MUT, ImgRscSize(qpixmap)) == 0) {
        REPORT_INFO(LC_LOWUI,"Error in updating resource limit for" 
                             " Mutable image");
    }

    /* RELEASE QT RESOURCES HELD */
    delete qpixmap;
}

extern "C" void
gxpport_get_mutable_argb(
 gxpport_mutableimage_native_handle imagePtr,
 jint* rgbBuffer, int offset, int scanLength,
 int x, int y, int width, int height,
 img_native_error_codes* errorPtr) {

    QPixmap *qpixmap = (QPixmap *)(imagePtr);
    QImage   image(qpixmap->convertToImage());
    int      curOffset = offset;           /* current offset in output array */
    int      curX, curY;
    unsigned int r,g,b;
    QRgb     pixel;

    for (curY = y; curY < y + height; curY++) {
        for (curX = x; curX < x + width; curX++) {

            // Obtain the R,G,B
            pixel = image.pixel(curX, curY);

            r = qRed(pixel) ;
            g = qGreen(pixel);
            b = qBlue(pixel);

	    rgbBuffer[curOffset] = (0xff << 24)
		| ((r & 0xff) << 16) 
		| ((g & 0xff) << 8) 
		|  (b & 0xff);

            curOffset++;
        }
        curOffset += (scanLength - width);
    }

    *errorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" QPixmap*
gxpportqt_get_mutableimage_pixmap
(gxpport_mutableimage_native_handle mutableImagePtr) {
    /* Simply cast it */
    return (QPixmap*)mutableImagePtr;
}
