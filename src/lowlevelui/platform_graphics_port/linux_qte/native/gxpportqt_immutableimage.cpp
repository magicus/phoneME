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
#include <stddef.h>

#include <qpixmap.h>
#include <qimage.h>

#include <qteapp_export.h>
#include <midpMalloc.h>

#include <midpResourceLimit.h>

#include <imgdcd_image_util.h>
#include <img_errorcodes.h>
#include <gxpport_immutableimage.h>
#include <gxpportqt_image.h>
#include "gxpportqt_intern_graphics_util.h"

/**
 * Structure for ImmutableImage data.
 */
typedef struct {

    /** Which routine allocated this image. */
    int marker;

    /** The platform version of Immutable Image */
    QImage* qimage;

    /** Cached pixmap used by UI */
    QPixmap* qpixmap;

} _Platform_ImmutableImage ;

static
bool load_raw(QImage** qimage,
	     imgdcd_image_buffer_raw* rawImageBuffer, unsigned int length,
             jboolean isStatic, int* ret_Width, int* ret_Height);

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
#define ImgRegionRscSize(img, width, height) ((width*height*32)>>3)

extern "C" void gxpport_createimmutable_from_mutable
(gxpport_mutableimage_native_handle srcMutableImagePtr,
 gxpport_image_native_handle *newImmutableImagePtr,
 img_native_error_codes* creationErrorPtr) {

    /* Convert from source QPixmap to destination QImage */
    QPixmap* srcPixmap = gxpportqt_get_mutableimage_pixmap(srcMutableImagePtr);

    int rscSize = ImgRscSize(srcPixmap); /* new img is the same size */

    /* Check resource limit before copying */
    if (midpCheckResourceLimit(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        /* Exceeds resource limit */
        *creationErrorPtr  = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
        return;
    }

    _Platform_ImmutableImage* immutableImage =
        (_Platform_ImmutableImage*)midpMalloc(sizeof(_Platform_ImmutableImage));
    if (immutableImage == NULL) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    immutableImage->qimage = new QImage(srcPixmap->convertToImage());
    if (NULL == immutableImage->qimage) {
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    if (immutableImage->qimage->isNull()) {
        delete immutableImage->qimage;
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;    
    }

    /* Copying succeeds */
    if (midpIncResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        REPORT_ERROR(LC_LOWUI, "Error in updating resource limit"
                     " for Immutable image");
    }

    immutableImage->marker = 1;
    immutableImage->qpixmap = NULL;

    *newImmutableImagePtr = immutableImage;
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" void gxpport_createimmutable_from_immutableregion
(gxpport_image_native_handle srcImmutableImagePtr,
 int src_x, int src_y, 
 int src_width, int src_height,
 int transform,
 gxpport_image_native_handle *newImmutableImagePtr,
 img_native_error_codes* creationErrorPtr) {

    _Platform_ImmutableImage* srcImmutableImage = 
      (_Platform_ImmutableImage*)srcImmutableImagePtr;

    int rscSize = ImgRegionRscSize(srcImmutableImage->qimage,
                   src_width, src_height);

    /* Check with resource limit */
    if (midpCheckResourceLimit(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        /* Exceed Resource limit */
        *creationErrorPtr = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
        return;
    }

    _Platform_ImmutableImage* immutableImage =
      (_Platform_ImmutableImage*)midpMalloc(sizeof(_Platform_ImmutableImage));
    if (immutableImage == NULL) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    immutableImage->qimage =
        new QImage(srcImmutableImage->qimage->copy(src_x, 
                                                       src_y, 
                                                       src_width, 
                                                       src_height));
    if (NULL == immutableImage->qimage) {
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    if (immutableImage->qimage->isNull()) {
        delete immutableImage->qimage;
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;    
    }

    if (transform != 0) {
        transform_image(immutableImage->qimage, transform);
    }

    /* Copy succeeds */
    if (midpIncResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        REPORT_ERROR(LC_LOWUI, "Error in updating resource limit"
                     " for Immutable image");
    }

    immutableImage->marker =2;
    immutableImage->qpixmap = NULL;

    *newImmutableImagePtr = immutableImage;
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" void gxpport_createimmutable_from_mutableregion
(gxpport_mutableimage_native_handle srcMutableImagePtr,
 int src_x, int src_y, 
 int src_width, int src_height,
 int transform,
 gxpport_image_native_handle* newImmutableImagePtr,
 img_native_error_codes* creationErrorPtr) {

    QPixmap *srcqpixmap =
    gxpportqt_get_mutableimage_pixmap(srcMutableImagePtr);

    int rscSize = ImgRegionRscSize(srcqpixmap, src_width, src_height);

    /* Check resource limit */
    if (midpCheckResourceLimit(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        /* Exceed Resource limit */
        *creationErrorPtr = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
        return;
    }

    QImage srcQImage = srcqpixmap->convertToImage();
    if (srcQImage.isNull()) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    _Platform_ImmutableImage* immutableImage = 
      (_Platform_ImmutableImage*)midpMalloc(sizeof(_Platform_ImmutableImage));
    if (immutableImage == NULL) {
	*creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
	return;
    }

    immutableImage->qimage = new QImage(srcQImage.copy(src_x,
                                                       src_y, 
                                                       src_width, 
                                                       src_height));
    if (NULL == immutableImage->qimage) {
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    if (immutableImage->qimage->isNull()) {
        delete immutableImage->qimage;
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    if (transform != 0) {
        transform_image(immutableImage->qimage, transform);
    }

    /* Copy succeeds */
    if (midpIncResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        REPORT_ERROR(LC_LOWUI, "Error in updating resource limit"
                     " for Immutable image");
    }

    immutableImage->marker = 3;
    immutableImage->qpixmap = NULL;
    
    *newImmutableImagePtr = immutableImage;
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" void gxpport_decodeimmutable_from_selfidentifying
(unsigned char* srcBuffer, int length, 
 int* imgWidth, int* imgHeight,
 gxpport_image_native_handle *newImmutableImagePtr,
 img_native_error_codes* creationErrorPtr) {
    MIDP_ERROR err;
    imgdcd_image_format format;
    unsigned int w, h;

    err = imgdcd_image_get_info(srcBuffer, (unsigned int)length,
			       &format, &w, &h);

    if (err != MIDP_ERROR_NONE || format == IMGDCD_IMAGE_FORMAT_UNSUPPORTED) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
        return;
    }

    /* Check resource limit */
    int rscSize = ImgRegionRscSize(NULL, w, h);

    if (midpCheckResourceLimit(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        /* Exceed Resource limit */
        *creationErrorPtr = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
        return;
    }

    bool loadResult = FALSE;
    
    _Platform_ImmutableImage* immutableImage =
        (_Platform_ImmutableImage*)midpMalloc(sizeof(_Platform_ImmutableImage));
    if (immutableImage == NULL) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    switch(format) {

    case IMGDCD_IMAGE_FORMAT_JPEG:
    case IMGDCD_IMAGE_FORMAT_PNG:
        immutableImage->qimage = new QImage();
        if (NULL == immutableImage->qimage) {
            *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
            return;
        }

        if (immutableImage->qimage->loadFromData((uchar*)srcBuffer, 
                                                 (unsigned int)length)) {

            *imgWidth  = immutableImage->qimage->width();
            *imgHeight = immutableImage->qimage->height();

            if ((0 == *imgWidth) || (0 == *imgHeight)) {
                delete immutableImage->qimage;
            } else {
		immutableImage->marker = 4;
                loadResult = TRUE;
            }
        } else {
            delete immutableImage->qimage;
            immutableImage->qimage = NULL;
        }

        break;

    case IMGDCD_IMAGE_FORMAT_RAW:
        loadResult = load_raw(&immutableImage->qimage,
                             (imgdcd_image_buffer_raw*)srcBuffer, 
                             (unsigned int)length, KNI_FALSE,
                             imgWidth, imgHeight);
	immutableImage->marker = 5;
        break;

    default:
	/* Shouldn't be here */
        *creationErrorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
        return;
    }

    if (!loadResult) {
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_DECODING_ERROR;
        return;
    }
        
    /* Image creation succeeds */
    if (midpIncResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
            REPORT_ERROR(LC_LOWUI, "Error in updating resource limit"
                         " for Immutable image");
    }

    immutableImage->qpixmap = NULL;

    *newImmutableImagePtr = immutableImage;
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" void gxpport_decodeimmutable_from_argb
(jint* srcBuffer,
 int width, int height,
 jboolean processAlpha,
 gxpport_image_native_handle* newImmutableImagePtr,
 img_native_error_codes* creationErrorPtr) {

    // use bytes as is, QT images have the same binary data layout
    QImage qimage = QImage((unsigned char*)srcBuffer, 
                           width, height, 
                           IMAGE_DEPTH,
                           NULL, 0, 
                           QImage::IgnoreEndian);

    if (qimage.isNull()) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_DECODING_ERROR;
        return;
    }

    /* Check Resource limit */
    int rscSize = ImgRscSize(&qimage);

    if (midpCheckResourceLimit(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        /* Exceed Resource limit */
        *creationErrorPtr = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
        return;
    }

    qimage.setAlphaBuffer(processAlpha == KNI_TRUE?TRUE:FALSE);

    _Platform_ImmutableImage* immutableImage =
        (_Platform_ImmutableImage*)midpMalloc(sizeof(_Platform_ImmutableImage));
    if (immutableImage == NULL) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    // make a deep copy of the image
    immutableImage->qimage = new QImage(qimage.copy());
    if (NULL == immutableImage->qimage) {
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    if (immutableImage->qimage->isNull()) {
        delete immutableImage->qimage;
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    /* Image creation succeeds */
    if (midpIncResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        REPORT_ERROR(LC_LOWUI, "Error in updating "
                     "resource limit for Immutable image");
    }

    immutableImage->marker = 6;
    immutableImage->qpixmap = NULL;

    *newImmutableImagePtr = immutableImage;
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

extern "C" void gxpport_render_immutableimage
(gxpport_image_native_handle immutableImagePtr,
 gxpport_mutableimage_native_handle dstMutableImagePtr,
 const jshort *clip,
 jint x_dest, jint y_dest) {

    _Platform_ImmutableImage* immutableImage = 
        (_Platform_ImmutableImage*)immutableImagePtr;
    QPixmap *qpixmapDst =
      gxpportqt_get_mutableimage_pixmap(dstMutableImagePtr);

    if (NULL == immutableImage || immutableImage->qimage->isNull()) {
        /* not a valid image should not happen, log this */
        return;
    }

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, -1, clip, 
                                    (QPaintDevice *)qpixmapDst, 
                                    0);
    if (immutableImage->qimage->hasAlphaBuffer()) {
        gc->drawImage(x_dest, y_dest, *immutableImage->qimage);
        return;
    }

    gc->drawPixmap(x_dest, y_dest,
        *(gxpportqt_get_immutableimage_pixmap(immutableImagePtr)));
}

extern "C" void gxpport_render_immutableregion
(gxpport_image_native_handle immutableImagePtr,
 gxpport_mutableimage_native_handle dstMutableImagePtr,
 const jshort *clip,
 jint x_dest, jint y_dest, 
 jint width, jint height,
 jint x_src, jint y_src,
 jint transform) {

    _Platform_ImmutableImage* immutableImage = 
        (_Platform_ImmutableImage*)immutableImagePtr;
    QPixmap *qpixmapDst =
    gxpportqt_get_mutableimage_pixmap(dstMutableImagePtr);

    if (NULL == immutableImagePtr || immutableImage->qimage->isNull()) {
      /* not a valid image should not happen, log this */
        return;
    }

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, -1, clip, 
                                    (QPaintDevice *)qpixmapDst, 
                                    0);
    if (0 == transform) {
        gc->drawImage(x_dest, y_dest,
                      *immutableImage->qimage,
                      x_src, y_src,
                      width, height,
                      0);
        return;
    }

    QImage image = immutableImage->qimage->copy(x_src, y_src, 
                                               width, height);

    transform_image(&image, transform);
    
    if (!image.isNull()) {
        gc->drawImage(x_dest, y_dest, image);
    }
}


extern "C" void gxpport_get_immutable_argb
(gxpport_image_native_handle immutableImagePtr,
 jint* rgbBuffer, int offset, int scanLength,
 int x, int y, int width, int height,
 img_native_error_codes* errorPtr) {

    _Platform_ImmutableImage* immutableImage = 
        (_Platform_ImmutableImage*)immutableImagePtr;

    if (NULL == immutableImage || immutableImage->qimage->isNull()) {
      /* not a valid image should not happen, log this */
      return ;
    }

    QImage   image;
    int      curOffset = offset;      /* current offset in output array */
    int      curX, curY;
    unsigned int r,g,b;
    QRgb     pixel;

    if (IMAGE_DEPTH != immutableImage->qimage->depth()) {
        image = immutableImage->qimage->convertDepth(IMAGE_DEPTH);
    } else {
        image = *immutableImage->qimage;
    }

    if (image.hasAlphaBuffer() == TRUE) {

        for (curY = y; curY < y + height; curY++) {
            for (curX = x; curX < x + width; curX++) {
                
                // Obtain the R,G,B
                pixel = image.pixel(curX, curY);
                
                r = qRed(pixel) ;
                g = qGreen(pixel);
                b = qBlue(pixel);
            
                rgbBuffer[curOffset] = (qAlpha(pixel) << 24)
                    | ((r & 0xff) << 16) 
                    | ((g & 0xff) << 8) 
                    |  (b & 0xff);
                
                curOffset++;
            }

            curOffset += (scanLength - width);
        }
        
    } else {
        
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
        
    }

    *errorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}


extern "C"
void gxpport_destroy_immutable(gxpport_image_native_handle imagePtr) {

    _Platform_ImmutableImage* immutableImage = 
        (_Platform_ImmutableImage*)imagePtr;

    if (NULL != immutableImage) {
        if (NULL != immutableImage->qimage) {
            int rscSize = ImgRscSize(immutableImage->qimage);

            /* Update the resource count */
            if (midpDecResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
                REPORT_ERROR(LC_LOWUI,"Error in updating resource limit for"
                             " Immutable image");
            }

            delete immutableImage->qimage;
        }

        if (NULL != immutableImage->qpixmap) {
            delete immutableImage->qpixmap;
        }

	midpFree(immutableImage);
    }
}

extern "C" void gxpport_decodeimmutable_to_platformbuffer
(unsigned char* srcBuffer, long length, 
 unsigned char** ret_dataBuffer, long* ret_length,
 img_native_error_codes* creationErrorPtr) {

    imgdcd_image_format format;
    MIDP_ERROR err;
    unsigned int w, h;

    err = imgdcd_image_get_info(srcBuffer, (unsigned int)length,
			       &format, &w, &h);
    
    switch (err) {

    case MIDP_ERROR_NONE:
	break; /* continue */

    case MIDP_ERROR_IMAGE_CORRUPTED:
	*creationErrorPtr = IMG_NATIVE_IMAGE_DECODING_ERROR;
	return;
    
    default:
	*creationErrorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
	return;
    }
		     
    switch (format) {

    case IMGDCD_IMAGE_FORMAT_RAW:
        /* already in RAW format. make a copy */
	{
	    unsigned char* dataBuffer = (unsigned char*) midpMalloc(length);
            
	    if (NULL == dataBuffer) {
		*creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
	    } else {
                
		memcpy(dataBuffer, srcBuffer, length);
                
		*ret_dataBuffer = dataBuffer;
		*ret_length = length;

		*creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
	    }
	}
        break;

    case IMGDCD_IMAGE_FORMAT_JPEG:
    case IMGDCD_IMAGE_FORMAT_PNG:
        {
            QImage qimage;
            
            if (qimage.loadFromData((uchar*)srcBuffer, 
                                    (unsigned int)length)) {
                int imgWidth  = qimage.width();
                int imgHeight = qimage.height();
                
                if ((0 == imgWidth) || (0 == imgHeight)) {
                    *creationErrorPtr = IMG_NATIVE_IMAGE_DECODING_ERROR;
                } else {
                    QImage image;
                    if (IMAGE_DEPTH != qimage.depth()) {
                        image = qimage.convertDepth(IMAGE_DEPTH);
                    } else {
                        image = qimage;
                    }
                    
                    *ret_length = offsetof(imgdcd_image_buffer_raw, data)
				+ image.numBytes();

                    imgdcd_image_buffer_raw *dataBuffer =
			(imgdcd_image_buffer_raw *) midpMalloc(*ret_length);
                    
                    if (NULL == dataBuffer) {
                        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
                    } else {
                        dataBuffer->width    = (unsigned int)imgWidth;
                        dataBuffer->height   = (unsigned int)imgHeight;
                        dataBuffer->hasAlpha = (unsigned int)image.hasAlphaBuffer();
                        
                        memcpy(dataBuffer->header, imgdcd_raw_header, 4);
                        memcpy(dataBuffer->data, image.bits(), image.numBytes());
                        
                        *ret_dataBuffer = (unsigned char *)dataBuffer;
                        *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
                    }
                }
            } else {
                *creationErrorPtr = IMG_NATIVE_IMAGE_DECODING_ERROR;
            }
        }
        break;
    
    default:
        *creationErrorPtr = IMG_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
        break;

    } /* switch (format) */
}

extern "C" void gxpport_loadimmutable_from_platformbuffer
(unsigned char* srcBuffer, int length, jboolean isStatic,
 int* ret_imgWidth, int* ret_imgHeight,
 gxpport_image_native_handle *newImmutableImagePtr,
 img_native_error_codes* creationErrorPtr) {

    int rscSize;
    imgdcd_image_buffer_raw* dataBuffer = (imgdcd_image_buffer_raw*)srcBuffer;

    /* Check resource limit */
    rscSize = ImgRegionRscSize(NULL, dataBuffer->width, dataBuffer->height);
    if (midpCheckResourceLimit(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        /* Exceed Resource limit */
        *creationErrorPtr = IMG_NATIVE_IMAGE_RESOURCE_LIMIT;
        return;
    }

    _Platform_ImmutableImage* immutableImage =
        (_Platform_ImmutableImage*)midpMalloc(sizeof(_Platform_ImmutableImage));
    if (immutableImage == NULL) {
        *creationErrorPtr = IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
        return;
    }

    if (!load_raw(&immutableImage->qimage,
                dataBuffer,
                length, isStatic,
                ret_imgWidth, ret_imgHeight)) {
	midpFree(immutableImage);
        *creationErrorPtr = IMG_NATIVE_IMAGE_DECODING_ERROR;
        return;
    }

    /* Image creation succeeds */
    if (midpIncResourceCount(RSC_TYPE_IMAGE_IMMUT, rscSize) == 0) {
        REPORT_ERROR(LC_LOWUI, "Error in updating resource limit"
                     " for Immutable image");
    }
            
    immutableImage->marker = 7;
    immutableImage->qpixmap = NULL;

    *newImmutableImagePtr = immutableImage;
    *creationErrorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}


extern "C" QPixmap* gxpportqt_get_immutableimage_pixmap
(gxpport_image_native_handle immutableImagePtr) {

    _Platform_ImmutableImage* immutableImage = 
        (_Platform_ImmutableImage*)immutableImagePtr;

    if (NULL == immutableImage || NULL == immutableImage->qimage ||
        immutableImage->qimage->isNull()) {
        return NULL;
    }

    if (NULL != immutableImage->qpixmap) {
        return immutableImage->qpixmap;
    }

    immutableImage->qpixmap = new QPixmap();
    if (NULL == immutableImage->qpixmap) {
        return NULL;
    }
            
    immutableImage->qpixmap->convertFromImage(*immutableImage->qimage);
    if (!immutableImage->qpixmap->isNull()) {
        return immutableImage->qpixmap;
    }

    delete immutableImage->qpixmap;
    immutableImage->qpixmap = NULL;
    return NULL;
}

/*
 * loads a RAW image into a QImage 
 *
 * @param qimage pointer to destination QImage
 * @param rawImageBuffer source RAW image
 * @param length in bytes of the input image
 * @param isStatic true if the rawImageBuffer is static
 * @return ret_Width the width of the decoded image
 * @return ret_Height the height of the decoded image
 */
static 
bool load_raw(QImage** qimage, imgdcd_image_buffer_raw* rawImageBuffer, 
	      unsigned int length,
             jboolean isStatic, int* ret_Width, int* ret_Height) {

    bool retVal = FALSE;
    
    /* Check buffer size */
    if (offsetof(imgdcd_image_buffer_raw, data)
	+ rawImageBuffer->width * rawImageBuffer->height * 4
	== length) {

	QImage* localQimage = new QImage(rawImageBuffer->data,
                                    rawImageBuffer->width,
				    rawImageBuffer->height, 
                                    IMAGE_DEPTH,
                                    NULL, 0, 
                                    QImage::IgnoreEndian);
    
	if (localQimage != NULL) {
            if (!localQimage->isNull()) {
                localQimage->setAlphaBuffer(rawImageBuffer->hasAlpha);

                if (isStatic) {
                    *qimage = localQimage;
                } else {
                    *qimage = new QImage(localQimage->copy());
                    delete localQimage;
                }

                if (*qimage != NULL) {
                    *ret_Width  = (*qimage)->width();
                    *ret_Height = (*qimage)->height();
                    if ((0 != *ret_Width) && (0 != *ret_Height)) {
                        retVal = TRUE;
                    } else {
                        delete *qimage;
                    }
                }
            }
	}
    }
     
    return retVal;
}
