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

#include <stddef.h>
#include <string.h>

#include <sni.h>
#include <commonKNIMacros.h>
#include <midpUtilKni.h>
#include <midpMalloc.h>

#include <gxapi_constants.h>

#include <imgapi_image.h>
#include <img_errorcodes.h>
#include <img_imagedata_load.h>
#include <imgdcd_image_util.h>


#define PIXEL imgdcd_pixel_type
#define ALPHA imgdcd_alpha_type

/** Convenenient for convert Java image object to screen buffer */
#define getImageData(jimgData, width, height, pixelData, alphaData)  \
        get_imagedata(IMGAPI_GET_IMAGEDATA_PTR(jimgData),   \
                      width, height, pixelData, alphaData)

/** Convert 24-bit RGB color to 16bit (565) color */
#define RGB24TORGB16(x) (((( x ) & 0x00F80000) >> 8) + \
                             ((( x ) & 0x0000FC00) >> 5) + \
                             ((( x ) & 0x000000F8) >> 3) )

/** Convert 16-bit (565) color to 24-bit RGB color */
#define RGB16TORGB24(x) ( ((x & 0x001F) << 3) | ((x & 0x001C) >> 2) |\
                              ((x & 0x07E0) << 5) | ((x & 0x0600) >> 1) |\
                              ((x & 0xF800) << 8) | ((x & 0xE000) << 3) )

/**
 * Create native representation for a image.
 *
 * @param jimg Java Image ROM structure to convert from
 * @param sbuf pointer to Screen buffer structure to populate
 * @param g optional Graphics object for debugging clip code.
 *            give NULL if don't care.
 *
 * @return the given 'sbuf' pointer for convenient usage,
 *           or NULL if the image is null.
 */
static int get_imagedata(const java_imagedata *img,
                         int *width, int *height,
                         PIXEL **pixelData,
                         ALPHA **alphaData) {


    // NOTE:
    // Since this routine is called by every graphics operations
    // We use ROMStruct directly instead of macros
    // like JavaByteArray, etc, for max performance.
    //
    if (img == NULL) {
        return KNI_FALSE;
    }

    *width  = img->width;
    *height = img->height;

    // Only use nativePixelData and nativeAlphaData if
    // pixelData is null 
    if (img->pixelData != NULL) {
        *pixelData = (PIXEL *)&(img->pixelData->elements[0]);
        *alphaData = (img->alphaData != NULL)
                            ? (ALPHA *)&(img->alphaData->elements[0])
                            : NULL;
    } else {
        *pixelData = (PIXEL *)img->nativePixelData;
        *alphaData = (ALPHA *)img->nativeAlphaData;
    }

    return KNI_TRUE;
}


/**
 * Decodes the given input data into a cache representation that can
 * be saved and loaded quickly.
 * The input data should be in a self-identifying format; that is,
 * the data must contain a description of the decoding process.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param ret_dataBuffer pointer to the platform representation data that
 *         be saved.
 *  @param ret_length pointer to the length of the return data.
 *  @return one of error codes:
 *              MIDP_ERROR_NONE,
 *              MIDP_ERROR_OUT_MEM,
 *              MIDP_ERROR_UNSUPPORTED,
 *              MIDP_ERROR_OUT_OF_RESOURCE,
 *              MIDP_ERROR_IMAGE_CORRUPTED
 */
MIDP_ERROR img_decode_data2cache(unsigned char* srcBuffer,
                                 unsigned int length,
                                 unsigned char** ret_dataBuffer,
                                 unsigned int* ret_length) {

    unsigned int pixelSize, alphaSize;
    imgdcd_image_format format;
    MIDP_ERROR err;

    int width, height;
    PIXEL *pixelData;
    ALPHA *alphaData;

    imgdcd_image_buffer_raw *rawBuffer;
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    err = imgdcd_image_get_info(srcBuffer, length,
                            &format, (unsigned int *)&width,
                            (unsigned int *)&height);
    if (err != MIDP_ERROR_NONE) {
        return err;
    }

    pixelSize = sizeof(PIXEL) * width * height;
    alphaSize = sizeof(ALPHA) * width * height;

    switch (format) {

    case IMGDCD_IMAGE_FORMAT_JPEG:
        /* JPEG does not contain alpha data */
        alphaSize = 0;
        /* Fall through */

    case IMGDCD_IMAGE_FORMAT_PNG:
        /* Decode PNG/JPEG to screen buffer format */
        rawBuffer = (imgdcd_image_buffer_raw *)
          midpMalloc(offsetof(imgdcd_image_buffer_raw, data)+pixelSize+alphaSize);

        if (rawBuffer == NULL) {
            return MIDP_ERROR_OUT_MEM;
        }

        pixelData = (PIXEL *)rawBuffer->data;

        if (format == IMGDCD_IMAGE_FORMAT_PNG) {
            alphaData = rawBuffer->data + pixelSize;

            rawBuffer->hasAlpha = imgdcd_decode_png(srcBuffer, length,
                                                    width, height,
                                               (imgdcd_pixel_type *)pixelData, 
                                               (imgdcd_alpha_type *)alphaData,
                                                    &creationError);
            if (!rawBuffer->hasAlpha) {
                alphaData = NULL;
                alphaSize = 0; /* Exclude alpha data */
            }
        } else {
            alphaData = NULL;

            rawBuffer->hasAlpha = KNI_FALSE;

            imgdcd_decode_jpeg(srcBuffer, length,
                               width, height,
                               (imgdcd_pixel_type *)pixelData,
                               (imgdcd_alpha_type *)alphaData,
                               &creationError);
        }

        if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
            midpFree(rawBuffer);
            return MIDP_ERROR_IMAGE_CORRUPTED;
        }

        memcpy(rawBuffer->header, imgdcd_raw_header, 4);
        rawBuffer->width  = width;        /* Use default endian */
        rawBuffer->height = height;        /* Use default endian */

        *ret_dataBuffer = (unsigned char *)rawBuffer;
        *ret_length = offsetof(imgdcd_image_buffer_raw, data)+pixelSize+alphaSize;

        return MIDP_ERROR_NONE;

    case IMGDCD_IMAGE_FORMAT_RAW:
        /* Already in screen buffer format, simply copy the data */
        *ret_dataBuffer = (unsigned char *)midpMalloc(length);
        if (*ret_dataBuffer == NULL) {
            return MIDP_ERROR_OUT_MEM;
        } else {
            memcpy(*ret_dataBuffer, srcBuffer, length);
            *ret_length = length;
            return MIDP_ERROR_NONE;
        }

    default:
        return MIDP_ERROR_UNSUPPORTED;
    } /* switch (image_type) */
}

/**
 * Gets an ARGB integer array from this <tt>ImageData</tt>. The
 * array consists of values in the form of 0xAARRGGBB.
 *
 * @param imageData The ImageData to read the ARGB data from
 * @param rgbBuffer The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlength Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
void imgj_get_argb(const java_imagedata * srcImageDataPtr,
                   jint * rgbBuffer,
                   jint offset,
                   jint scanlength,
                   jint x, jint y, jint width, jint height,
                   img_native_error_codes * errorPtr) {

  int srcWidth, srcHeight;
  PIXEL *srcPixelData;
  ALPHA *srcAlphaData;

  if (get_imagedata(srcImageDataPtr, &srcWidth, &srcHeight, 
                    &srcPixelData, &srcAlphaData) == KNI_TRUE) {
    // rgbData[offset + (a - x) + (b - y) * scanlength] = P(a, b);
    // P(a, b) = rgbData[offset + (a - x) + (b - y) * scanlength]
    // x <= a < x + width
    // y <= b < y + height
    int a, b, pixel, alpha;

    if (srcAlphaData != NULL) {
      for (b = y; b < y + height; b++) {
        for (a = x; a < x + width; a++) {
          pixel = srcPixelData[b*srcWidth + a];
          alpha = srcAlphaData[b*srcWidth + a];
          rgbBuffer[offset + (a - x) + (b - y) * scanlength] =
            (alpha << 24) + RGB16TORGB24(pixel);
        }
      }
    } else {
      for (b = y; b < y + height; b++) {
        for (a = x; a < x + width; a++) {
          pixel = srcPixelData[b*srcWidth + a];
          rgbBuffer[offset + (a - x) + (b - y) * scanlength] =
            RGB16TORGB24(pixel) | 0xFF000000;
        }
      }
    }
  }

  * errorPtr = IMG_NATIVE_IMAGE_NO_ERROR;
}

/**
 * Get pointer to internal buffer of Java byte array and
 * check that expected offset/length can be applied to the buffer
 *
 * @param byteArray Java byte array object to get buffer from
 * @param offset offset of the data needed in the buffer
 * @param length length of the data needed in the buffer
 *          starting from the offset
 * @return pointer to the buffer, or NULL if offset/length are
 *    are not applicable.
 */
static unsigned char *get_java_byte_buffer(KNIDECLARGS
    jobject byteArray, int offset, int length) {

    unsigned char *buffer = (unsigned char *)JavaByteArray(byteArray);
    int byteArrayLength = KNI_GetArrayLength(byteArray);
    if (offset < 0 || length < 0 || offset + length > byteArrayLength ) {
        KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
        return NULL;
    }
    return buffer;
}

/**
 * Load Java ImageData instance with image data in RAW format.
 * Image data is provided either in native buffer, or in Java
 * byte array. Java array is used with more priority.
 *
 * @param imageData Java ImageData object to be loaded with image data
 * @param nativeBuffer pointer to native buffer with raw image data,
 *          this parameter is alternative to javaBuffer
 * @param javaBuffer Java byte array with raw image data,
 *          this parameter is alternative to nativeBuffer
 * @param offset offset of the raw image data in the buffer
 * @param length length of the raw image data in the buffer
 *          starting from the offset
 *
 * @return KNI_TRUE in the case ImageData is successfully loaded with
 *    raw image data, otherwise KNI_FALSE.
 */
static int gx_load_imagedata_from_raw_buffer(KNIDECLARGS jobject imageData,
    unsigned char *nativeBuffer, jobject javaBuffer,
    int offset, int length) {

    int imageSize;
    int pixelSize, alphaSize;
    int status = KNI_FALSE;
    imgdcd_image_buffer_raw *rawBuffer = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(pixelData);
    KNI_DeclareHandle(alphaData);

    do {
        /** Check native and Java buffer parameters */
        if (!KNI_IsNullHandle(javaBuffer)) {
            if (nativeBuffer != NULL) {
                REPORT_ERROR(LC_LOWUI,
                    "Native and Java buffers should not be used together");
                break;
            }
            nativeBuffer = get_java_byte_buffer(KNIPASSARGS
                javaBuffer, offset, length);
        }
        if (nativeBuffer == NULL) {
            REPORT_ERROR(LC_LOWUI,
                "Null raw image buffer is provided");
            break;
        }

        /** Check header */
        rawBuffer = (imgdcd_image_buffer_raw *)(nativeBuffer + offset);
        if (memcmp(rawBuffer->header, imgdcd_raw_header, 4) != 0) {
            REPORT_ERROR(LC_LOWUI, "Unexpected raw image type");
            break;
        }

        imageSize = rawBuffer->width * rawBuffer->height;
        pixelSize = sizeof(PIXEL) * imageSize;
        alphaSize = 0;
        if (rawBuffer->hasAlpha) {
            alphaSize = sizeof(ALPHA) * imageSize;
        }

        /** Check data array length */
        if ((unsigned int)length !=
            (offsetof(imgdcd_image_buffer_raw, data)
                + pixelSize + alphaSize)) {
            REPORT_ERROR(LC_LOWUI, "Raw image is corrupted");
            break;
        }

        if (rawBuffer->hasAlpha) {
            /* Has alpha */
            SNI_NewArray(SNI_BYTE_ARRAY, alphaSize, alphaData);
            if (KNI_IsNullHandle(alphaData)) {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
                break;
            }
            /** Link the new array into ImageData to protect if from GC */
            midp_set_jobject_field(KNIPASSARGS imageData, "alphaData", "[B", alphaData);

            /** New array allocation could cause GC and buffer moving */
            if (!KNI_IsNullHandle(javaBuffer)) {
                nativeBuffer = get_java_byte_buffer(KNIPASSARGS
                    javaBuffer, offset, length);
                rawBuffer = (imgdcd_image_buffer_raw *)
                    (nativeBuffer + offset);
            }
            memcpy(JavaByteArray(alphaData),
                rawBuffer->data + pixelSize, alphaSize);
        }

        SNI_NewArray(SNI_BYTE_ARRAY, pixelSize, pixelData);
        if (KNI_IsNullHandle(pixelData)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }
            midp_set_jobject_field(KNIPASSARGS imageData, 
                                   "pixelData", "[B", pixelData);

        /** New array allocation could cause GC and buffer moving */
        if (!KNI_IsNullHandle(javaBuffer)) {
            nativeBuffer = get_java_byte_buffer(KNIPASSARGS
                javaBuffer, offset, length);
            rawBuffer = (imgdcd_image_buffer_raw *)
                (nativeBuffer + offset);
        }
            memcpy(JavaByteArray(pixelData), rawBuffer->data, pixelSize);

        IMGAPI_GET_IMAGEDATA_PTR(imageData)->width =
            (jint)rawBuffer->width;
        IMGAPI_GET_IMAGEDATA_PTR(imageData)->height =
            (jint)rawBuffer->height;
        status = KNI_TRUE;

    } while(0);

    KNI_EndHandles();
    return status;
}

/**
 * Load Java ImageData instance with image data in RAW format.
 * Image data is provided in native buffer.
 *
 * @param imageData Java ImageData object to be loaded with image data
 * @param buffer pointer to native buffer with raw image data
 * @param length length of the raw image data in the buffer
 *
 * @return KNI_TRUE in the case ImageData is successfully loaded with
 *    raw image data, otherwise KNI_FALSE.
 */
int img_load_imagedata_from_raw_buffer(KNIDECLARGS jobject imageData,
    unsigned char *buffer, int length) {

    int status = KNI_FALSE;

    KNI_StartHandles(1);

    /* A handle for which KNI_IsNullHandle() check is true */
    KNI_DeclareHandle(nullHandle);

    status = gx_load_imagedata_from_raw_buffer(KNIPASSARGS
                    imageData, buffer, nullHandle, 0, length);

    KNI_EndHandles();
    return status;
}

/**
 * Loads the <tt>ImageData</tt> with the given raw data array.
 * The array consists of raw image data including header info.
 * <p>
 * Java declaration:
 * <pre>
 *     loadRAW(Ljavax/microedition/lcdui/ImageData;[B)V
 * </pre>
 *
 * @param imageData The instance of ImageData to be loaded from array
 * @param imageBytes The array of raw image data
 * @param imageOffset the offset of the start of the data in the array
 * @param imageLength the length of the data in the array
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRAW) {

    int offset = KNI_GetParameterAsInt(3);
    int length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);
    KNI_DeclareHandle(imageData);
    KNI_DeclareHandle(imageBytes);

    KNI_GetParameterAsObject(1, imageData);
    KNI_GetParameterAsObject(2, imageBytes);

    /* Load ImageData content from Java byte array with raw image data */
    if (gx_load_imagedata_from_raw_buffer(KNIPASSARGS
            imageData, NULL, imageBytes, offset, length) != KNI_TRUE) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Decodes the given byte array into the <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     loadPNG(Ljavax/microedition/lcdui/ImageData;[BII)Z
 * </pre>
 *
 * @param imageData the ImageData to load to
 * @param imageBytes A byte array containing the encoded PNG image data
 * @param offset The start of the image data within the byte array
 * @param length The length of the image data in the byte array
 *
 * @return true if there is alpha data
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadPNG) {
    int            length = KNI_GetParameterAsInt(4);
    int            offset = KNI_GetParameterAsInt(3);
    int            status = KNI_TRUE;
    unsigned char* srcBuffer = NULL;
    PIXEL *imgPixelData;
    ALPHA *imgAlphaData;
    java_imagedata * midpImageData = NULL;

    /* variable to hold error codes */
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    KNI_StartHandles(4);
    KNI_DeclareHandle(alphaData);
    KNI_DeclareHandle(pixelData);
    KNI_DeclareHandle(pngData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, pngData);
    KNI_GetParameterAsObject(1, imageData);

    midpImageData = IMGAPI_GET_IMAGEDATA_PTR(imageData);

    /* assert
     * (KNI_IsNullHandle(pngData))
     */

    srcBuffer = (unsigned char *)JavaByteArray(pngData);
    /*
     * JAVA_TRACE("loadPNG pngData length=%d  %x\n",
     *            JavaByteArray(pngData)->length, srcBuffer);
     */


    unhand(jbyte_array, pixelData) = midpImageData->pixelData;
    if (!KNI_IsNullHandle(pixelData)) {
        imgPixelData = (PIXEL *)JavaByteArray(pixelData);
        /*
         * JAVA_TRACE("loadPNG pixelData length=%d\n",
         *            JavaByteArray(pixelData)->length);
         */
    } else {
        imgPixelData = NULL;
    }

    unhand(jbyte_array, alphaData) = midpImageData->alphaData;
    if (!KNI_IsNullHandle(alphaData)) {
        imgAlphaData = (ALPHA *)JavaByteArray(alphaData);
        /*
         * JAVA_TRACE("decodePNG alphaData length=%d\n",
         *            JavaByteArray(alphaData)->length);
         */
    } else {
        imgAlphaData = NULL;
    }

    /* assert
     * (imgPixelData != NULL && imgAlphaData != NULL)
     */
    status = imgdcd_decode_png((srcBuffer + offset), length,
                        midpImageData->width, 
                        midpImageData->width,
                        (imgdcd_pixel_type *)imgPixelData,
                        (imgdcd_alpha_type *)imgAlphaData,
                        &creationError);

    if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
}

/**
 * Decodes the given byte array into the <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     loadJPG(Ljavax/microedition/lcdui/ImageData;[BII)V
 * </pre>
 *
 * @param imageData the ImageData to load to
 * @param imageBytes A byte array containing the encoded JPEG image data
 * @param imageOffset The start of the image data within the byte array
 * @param imageLength The length of the image data in the byte array
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadJPEG) {
    int            length = KNI_GetParameterAsInt(4);
    int            offset = KNI_GetParameterAsInt(3);
    unsigned char* srcBuffer = NULL;
    int imgWidth, imgHeight;
    PIXEL *imgPixelData = NULL;
    ALPHA *imgAlphaData = NULL;

    java_imagedata * midpImageData = NULL;

    /* variable to hold error codes */
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    KNI_StartHandles(3);
    /* KNI_DeclareHandle(alphaData); */
    KNI_DeclareHandle(pixelData);
    KNI_DeclareHandle(jpegData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, jpegData);
    KNI_GetParameterAsObject(1, imageData);

    midpImageData = IMGAPI_GET_IMAGEDATA_PTR(imageData);

    /* assert
     * (KNI_IsNullHandle(jpegData))
     */

    srcBuffer = (unsigned char *)JavaByteArray(jpegData);
    /*
     * JAVA_TRACE("loadJPEG jpegData length=%d  %x\n",
     *            JavaByteArray(jpegData)->length, srcBuffer);
     */

    imgWidth  = midpImageData->width;
    imgHeight = midpImageData->height;

    unhand(jbyte_array, pixelData) = midpImageData->pixelData;
    if (!KNI_IsNullHandle(pixelData)) {
        imgPixelData = (PIXEL *)JavaByteArray(pixelData);
        /*
         * JAVA_TRACE("loadJPEG pixelData length=%d\n",
         *            JavaByteArray(pixelData)->length);
         */
    }

    /* assert
     * (imgPixelData != NULL)
     */
    imgdcd_decode_jpeg((srcBuffer + offset), length,
                       imgWidth, imgHeight, 
                       (imgdcd_pixel_type *)imgPixelData,
                       (imgdcd_alpha_type *)imgAlphaData, 
                       &creationError);

    if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Initializes an <tt>ImageData</tt> from a romized pixel data
 * <p>
 * Java declaration:
 * <pre>
 *     loadRomizedImage(Ljavax/microedition/lcdui/ImageData;I)V
 * </pre>
 *
 * @param imageData The ImageData to load to
 * @param imageDataPtr native pointer to image data as Java int
 * @param imageDataLength length of image data array
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRomizedImage) {
    int imageDataPtr = KNI_GetParameterAsInt(2);
    int imageDataLength = KNI_GetParameterAsInt(3);

    int alphaSize;
    int pixelSize;
    int imageSize;
    int expectedLength;

    imgdcd_image_buffer_raw *rawBuffer;
    java_imagedata *midpImageData;

    jboolean status = KNI_FALSE;


    KNI_StartHandles(1);
    KNI_DeclareHandle(imageData);
    KNI_GetParameterAsObject(1, imageData);

    rawBuffer = (imgdcd_image_buffer_raw*)imageDataPtr;

    do {
        if (rawBuffer == NULL) {
            REPORT_ERROR(LC_LOWUI, "Romized image data is null");

            status = KNI_FALSE;
            break;
        }

        /** Check header */
        if (memcmp(rawBuffer->header, imgdcd_raw_header, 4) != 0) {
            REPORT_ERROR(LC_LOWUI, "Unexpected romized image type");

            status = KNI_FALSE;
            break;
        }

        imageSize = rawBuffer->width * rawBuffer->height;
        pixelSize = sizeof(PIXEL) * imageSize;
        alphaSize = 0;
        if (rawBuffer->hasAlpha) {
            alphaSize = sizeof(ALPHA) * imageSize;
        }

        /** Check data array length */
        expectedLength = offsetof(imgdcd_image_buffer_raw, data) +
            pixelSize + alphaSize;
        if (imageDataLength != expectedLength) {
            REPORT_ERROR(LC_LOWUI,
                    "Unexpected romized image data array length");

            status = KNI_FALSE;
            break;
        }

        midpImageData = IMGAPI_GET_IMAGEDATA_PTR(imageData);

        midpImageData->width = (jint)rawBuffer->width;
        midpImageData->height = (jint)rawBuffer->height;

        midpImageData->nativePixelData = (jint)rawBuffer->data;

        if (rawBuffer->hasAlpha) {
            midpImageData->nativeAlphaData =
                (jint)(rawBuffer->data + pixelSize);
        }

        status = KNI_TRUE;

    } while (0);

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
}

/**
 * Loads the <tt>ImageData</tt> with the given ARGB integer
 * array. The array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     loadRGB(Ljavax/microedition/lcdui/ImageData;[I)V
 * </pre>
 *
 * @param rgbData The array of argb image data
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRGB) {
    /* jboolean processAlpha = KNI_GetParameterAsBoolean(2); */
    int height;
    int width;
    PIXEL *pixelData;
    ALPHA *alphaData;

    int *rgbBuffer;
   
    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, rgbData);
    KNI_GetParameterAsObject(1, imageData);


    rgbBuffer = JavaIntArray(rgbData);
    if (getImageData(imageData, &width, &height, &pixelData, &alphaData) 
        == KNI_TRUE) {
        int i;
        int len = KNI_GetArrayLength(rgbData);
        int data_length = width * height;

        if (len > data_length) {
            len = data_length;
        }

        /* if (len != width*height) {
         *    JAVA_TRACE("len mismatch  %d !=  %d\n", len, width*height);
         * }
         */

        if (alphaData != NULL) {
            for (i = 0; i < len; i++) {
                pixelData[i] = RGB24TORGB16(rgbBuffer[i]);
                alphaData[i] = (rgbBuffer[i] >> 24) & 0x00ff;
            }
        } else {
            for (i = 0; i < len; i++) {
                pixelData[i] = RGB24TORGB16(rgbBuffer[i]);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

static void pixelCopy(PIXEL *src, const int srcLineW, const int srcXInc,
                      const int srcYInc, const int srcXStart,
                      PIXEL *dst, const int w, const int h) {
    int x, srcX;
    PIXEL *dstPtrEnd = dst + (h * w );

    (void)srcLineW; /* Surpress unused warning */

    for (; dst < dstPtrEnd; dst += w, src += srcYInc) {
        for (x = 0, srcX = srcXStart; x < w; srcX += srcXInc) {
            // printf("%d = %d\n", ((dst+x)-dstData), (src+srcX)-srcData);
            dst[x++] = src[srcX];
        }
    }
}

static void pixelAndAlphaCopy(PIXEL *src, 
                              const int srcLineW, const int srcXInc,
                              const int srcYInc, const int srcXStart, 
                              PIXEL *dst,
                              const int w, const int h,
                              const ALPHA *srcAlpha, ALPHA *dstAlpha) {
    int x, srcX;
    PIXEL *dstPtrEnd = dst + (h * w );

    (void)srcLineW; /* Surpress unused warning */

    for (; dst < dstPtrEnd; dst += w, src += srcYInc,
        dstAlpha += w, srcAlpha += srcYInc) {
        for (x = 0, srcX = srcXStart; x < w; srcX += srcXInc) {
            // printf("%d = %d\n", ((dst+x)-dstData), (src+srcX)-srcData);
            dstAlpha[x] = srcAlpha[srcX];
            dst[x++] = src[srcX];
        }
    }
}

static void blit(int srcWidth, int srcHeight,
                 PIXEL *srcPixelData, ALPHA *srcAlphaData,
                 int xSrc, int ySrc, int width, int height,
                 PIXEL *dstPixelData, ALPHA *dstAlphaData,
                 int transform) {
    PIXEL *srcPtr = NULL;
    int srcXInc=0, srcYInc=0, srcXStart=0;
    (void)srcHeight;

    switch (transform) {
    case TRANS_NONE:
        srcPtr = srcPixelData + (ySrc * srcWidth + xSrc);
        srcYInc = srcWidth;
        srcXStart = 0;
        srcXInc = 1;
        break;
    case TRANS_MIRROR_ROT180:
        srcPtr = srcPixelData + ((ySrc + height - 1) * srcWidth + xSrc);
        srcYInc = -srcWidth;
        srcXStart = 0;
        srcXInc = 1;
        break;
    case TRANS_MIRROR:
        srcPtr = srcPixelData + (ySrc * srcWidth + xSrc);
        srcYInc = srcWidth;
        srcXStart = width - 1;
        srcXInc = -1;
        break;
    case TRANS_ROT180:
        srcPtr = srcPixelData + ((ySrc + height - 1) * srcWidth + xSrc);
        srcYInc = -srcWidth;
        srcXStart = width - 1;
        srcXInc = -1;
        break;
    case TRANS_MIRROR_ROT270:
        srcPtr = srcPixelData + (ySrc * srcWidth + xSrc);
        srcYInc = 1;
        srcXStart = 0;
        srcXInc = srcWidth;
        break;
    case TRANS_ROT90:
        srcPtr = (srcPixelData) + ((ySrc + height - 1) * srcWidth + xSrc);
        srcYInc = 1;
        srcXStart = 0;
        srcXInc = -(srcWidth);
        break;
    case TRANS_ROT270:
        srcPtr = (srcPixelData) + (ySrc * srcWidth + xSrc + width - 1);
        srcYInc = -1;
        srcXStart = 0;
        srcXInc = srcWidth;
        break;
    case TRANS_MIRROR_ROT90:
        srcPtr = (srcPixelData) + ((ySrc + height - 1) * srcWidth + xSrc);
        srcYInc = -1;
        srcXStart = width - 1;
        srcXInc = -(srcWidth);
        break;
    }

    if (transform & TRANSFORM_INVERTED_AXES) {
        if (srcAlphaData == NULL) {
            pixelCopy(srcPtr, srcWidth, srcXInc, srcYInc, srcXStart,
                      dstPixelData, height, width);
        } else {
            ALPHA *srcAlpha = srcAlphaData + (srcPtr - srcPixelData);
            pixelAndAlphaCopy(srcPtr, srcWidth, srcXInc, srcYInc,
                              srcXStart,
                              dstPixelData, height, width, srcAlpha,
                              dstAlphaData);
        }
    } else {
        if (srcAlphaData == NULL) {
            pixelCopy(srcPtr, srcWidth, srcXInc, srcYInc, srcXStart,
                      dstPixelData, width, height);
        } else {
            ALPHA *srcAlpha = srcAlphaData + (srcPtr - srcPixelData);
            pixelAndAlphaCopy(srcPtr, srcWidth, srcXInc, srcYInc, srcXStart,
                              dstPixelData, width, height,
                              srcAlpha, dstAlphaData);
        }
    }
}

/**
 * Copies the region of the specified <tt>ImageData</tt> to
 * the specified <tt>ImageData</tt> object.
 * <p>
 * Java declaration:
 * <pre>
 *     loadRegion(Ljavax/microedition/lcdui/ImageData;
 *                Ljavax/microedition/lcdui/ImageData;IIIII)V
 * </pre>
 *
 * @param dest the ImageData to copy to
 * @param source the source image to be copied from
 * @param x The x coordinate of the upper left corner of the
 *          region to copy
 * @param y The y coordinate of the upper left corner of the
 *          region to copy
 * @param width The width of the region to copy
 * @param height The height of the region to copy
 * @param transform The transform to apply to the selected region.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRegion) {
    int      transform = KNI_GetParameterAsInt(7);
    int         height = KNI_GetParameterAsInt(6);
    int          width = KNI_GetParameterAsInt(5);
    int              y = KNI_GetParameterAsInt(4);
    int              x = KNI_GetParameterAsInt(3);

    int srcWidth, srcHeight;
    PIXEL *srcPixelData;
    ALPHA *srcAlphaData;

    int dstWidth, dstHeight;
    PIXEL *dstPixelData;
    ALPHA *dstAlphaData;

    KNI_StartHandles(2);
    KNI_DeclareHandle(srcImg);
    KNI_DeclareHandle(destImg);

    KNI_GetParameterAsObject(2, srcImg);
    KNI_GetParameterAsObject(1, destImg);

    if ((getImageData(destImg, &dstWidth, &dstHeight, 
                     &dstPixelData, &dstAlphaData) == KNI_TRUE) &&
        (getImageData(srcImg, &srcWidth, &srcHeight,
                     &srcPixelData, &srcAlphaData) == KNI_TRUE)) {

      blit(srcWidth, srcHeight, srcPixelData, srcAlphaData, 
           x, y, width, height, 
           dstPixelData, dstAlphaData, transform);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
