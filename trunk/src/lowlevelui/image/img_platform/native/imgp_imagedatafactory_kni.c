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
 * Implementation of Java native methods for the <tt>ImageImpl</tt> class.
 */

#include <stdlib.h>

#include <midp_constants_data.h>
#include <commonKNIMacros.h>
#include <midp_logging.h>
#include <midpError.h>
#include <midpMalloc.h>
#include <midpEventUtil.h>
#include <midpResourceLimit.h>
#include <midpUtilKni.h>

#include <imgapi_image.h>
#include <img_imagedata_load.h>
#include <gxpport_immutableimage.h>

#include <jvm.h>
#include <sni.h>


/**
 * Retrieves a pointer to the raw image data.
 *
 * @param imgData a handle to the <tt>ImageData</tt> Java object that contains
 *            the raw image data
 *
 * @return A pointer to the raw data associated with the given
 *         <tt>ImageData</tt> object. Otherwise NULL.
 */
gxpport_image_native_handle gxp_get_imagedata(jobject imgData) {

    if (KNI_IsNullHandle(imgData)) {
        return NULL;
    } else {
	return (gxpport_image_native_handle)IMGAPI_GET_IMAGEDATA_PTR(imgData)->nativeImageData;
    }
}

/**
 * Decodes the given input data into a native platform representation that can
 * be saved.  The input data should be in a self-identifying format; that is,
 * the data must contain a description of the decoding process.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param ret_dataBuffer pointer to the platform representation data that *         be saved.
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
    img_native_error_codes creationError;

    /* This external API is implemented in each platform */
    gxpport_decodeimmutable_to_platformbuffer(srcBuffer, (long)length,
					      ret_dataBuffer,
					      (long *)ret_length,
					      &creationError);

    switch (creationError) {

    case IMG_NATIVE_IMAGE_NO_ERROR:
	return MIDP_ERROR_NONE;

    case IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR:
	return MIDP_ERROR_OUT_MEM;

    case IMG_NATIVE_IMAGE_DECODING_ERROR:
	return MIDP_ERROR_IMAGE_CORRUPTED;

    default:
	return MIDP_ERROR_UNSUPPORTED;
    }
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
    int imgWidth;
    int imgHeight;

    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    /* pointer to native image structure */
    gxpport_image_native_handle newImagePtr;

    /*
     * Do the decoding of the png in the buffer and initialize
     * class variables.
     */
    gxpport_loadimmutable_from_platformbuffer(buffer, length, KNI_FALSE,
						                      &imgWidth, &imgHeight,
						                      &newImagePtr,
						                      &creationError);

    if (IMG_NATIVE_IMAGE_NO_ERROR == creationError) {
        java_imagedata *dstImageDataPtr = IMGAPI_GET_IMAGEDATA_PTR(imageData);

        dstImageDataPtr->width   = (jint)imgWidth;
        dstImageDataPtr->height  = (jint)imgHeight;
        dstImageDataPtr->nativeImageData  = (jint)newImagePtr;
        status = KNI_TRUE;        
	} else if (IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR == creationError) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);        
    } else if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
        KNI_ThrowNew(midpOutOfMemoryError,
                    "Resource limit exceeded for immutable image");
    } else {
        REPORT_WARN(LC_LOWUI,"Warning: could not load cached image;\n");        
    }
    return status;
}

/**
 * Creates a copy of the specified <tt>ImageData</tt> and stores the
 * copied image in this object.
 * <p>
 * Java declaration:
 * <pre>
 *     createImmutableImageDataCopy(Ljavax/microedition/lcdui/ImageData;
 *                                  Ljavax/microedition/lcdui/ImageData;)V
 * </pre>
 *
 * @param dest  The <tt>ImageData </tt>where to make a copy
 * @param source The mutable <tt>ImageData</tt> to copy
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageDataFactory_createImmutableImageDataCopy() {

    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    /* mutable image */
    gxpport_image_native_handle srcImage;

    /* pointer to native image structure */
    gxpport_image_native_handle newImage;

    KNI_StartHandles(2);
    KNI_DeclareHandle(dest);
    KNI_DeclareHandle(source);

    KNI_GetParameterAsObject(2, source);
    KNI_GetParameterAsObject(1, dest);

    srcImage = gxp_get_imagedata(source);

    gxpport_createimmutable_from_mutable(srcImage, &newImage, &creationError);

    if (IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR == creationError) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
	KNI_ThrowNew(midpOutOfMemoryError,
                     "Resource limit exceeded for immutable image");
    } else if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
	KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else {
	IMGAPI_GET_IMAGEDATA_PTR(dest)->nativeImageData = (jint)newImage;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Creates a copy of a region of the specified <tt>ImageData</tt> and
 * stores the copied image in passed in immutable <tt>ImageData</tt> object.
 * <p>
 * Java declaration:
 * <pre>
 *     createImmutableImageDataRegion(Ljavax/microedition/lcdui/ImageData;Ljavax/microedition/lcdui/ImageData;IIIII)V
 * </pre>
 *
 * @param dest The <tt>ImageData</tt> to copy to
 * @param source The <tt>ImageData</tt> to copy
 * @param x The x coordinate of the upper left corner of the
 *          region to copy
 * @param y The y coordinate of the upper left corner of the
 *          region to copy
 * @param width The width of the region to copy
 * @param height The height of the region to copy
 * @param transform The transform to apply to the selected region.
 * @param isMutable <tt>true</tt> if <tt>img</tt> is mutable, otherwise
 *                  <tt>false</tt>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageDataFactory_createImmutableImageDataRegion() {
    jboolean ismutable = KNI_GetParameterAsBoolean(8);
    int      transform = KNI_GetParameterAsInt(7);
    int         height = KNI_GetParameterAsInt(6);
    int          width = KNI_GetParameterAsInt(5);
    int              y = KNI_GetParameterAsInt(4);
    int              x = KNI_GetParameterAsInt(3);
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    /* pointer to native image structure */
    gxpport_image_native_handle srcImagePtr;
    gxpport_image_native_handle newImagePtr;

    KNI_StartHandles(2);
    KNI_DeclareHandle(srcImg);
    KNI_DeclareHandle(destImg);

    KNI_GetParameterAsObject(2, srcImg);
    KNI_GetParameterAsObject(1, destImg);

    /* get src image's midpNative data */
    srcImagePtr = gxp_get_imagedata(srcImg);

    if (KNI_FALSE == ismutable) {
	/* immutable image */
        gxpport_createimmutable_from_immutableregion(srcImagePtr,
						     x, y, width, height,
						     transform,
						     &newImagePtr,
						     &creationError);
    } else {
	/* mutable image */
	gxpport_createimmutable_from_mutableregion(srcImagePtr,
						   x, y, width, height,
						   transform,
						   &newImagePtr,
						   &creationError);
    }

    if (IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR == creationError) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
	KNI_ThrowNew(midpOutOfMemoryError,
		     "Resource limit exceeded for immutable image");
    } else if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
	KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else {
	IMGAPI_GET_IMAGEDATA_PTR(destImg)->nativeImageData = (jint)newImagePtr;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Decodes the given byte array and fill a passed in immutable <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     createImmutableImageDecodeImage([Ljavax/microedition/lcdui/ImageData;BII)V
 * </pre>
 *
 * @param imageData The <tt>ImageData</tt> to be filled
 * @param intputData A byte array containing the encoded image data
 * @param offset The start of the image data within the byte array
 * @param length The length of the image data in the byte array
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageDataFactory_createImmutableImageDecodeImage() {
    int            length = KNI_GetParameterAsInt(4);
    int            offset = KNI_GetParameterAsInt(3);
    unsigned char* srcBuffer = NULL;
    gxpport_image_native_handle newImagePtr;
    int            imgWidth;
    int            imgHeight;
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    KNI_StartHandles(2);
    KNI_DeclareHandle(pngData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, pngData);
    KNI_GetParameterAsObject(1, imageData);

    do {
        if ((offset < 0) ||
            (length < 0) ||
            (offset + length) > KNI_GetArrayLength(pngData)) {
            KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
            break;
        }

        srcBuffer = (unsigned char *)JavaByteArray(pngData);

	SNI_BEGIN_RAW_POINTERS;

	gxpport_decodeimmutable_from_selfidentifying(srcBuffer + offset,
						     length,
						     &imgWidth, &imgHeight,
						     &newImagePtr,
						     &creationError);

	SNI_END_RAW_POINTERS;

        if (IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR == creationError) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
            KNI_ThrowNew(midpOutOfMemoryError,
                         "Resource limit exceeded for immutable image");
            break;
        }

        if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
            break;
        }

	{
	  java_imagedata * dstImageDataPtr = IMGAPI_GET_IMAGEDATA_PTR(imageData);
	  dstImageDataPtr->width  = (jint)imgWidth;
	  dstImageDataPtr->height = (jint)imgHeight;
	  dstImageDataPtr->nativeImageData = (jint)newImagePtr;
	}
    } while (0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * boolean loadRomizedImage(IamgeData imageData, int imageDataArrayPtr,
 * int imageDataArrayLength);
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_lcdui_ImageDataFactory_loadRomizedImage() {
    int            status = KNI_FALSE;
    int            imageDataArrayPtr  = KNI_GetParameterAsInt(2);
    int            imageDataArrayLength  = KNI_GetParameterAsInt(3);
    int            imgWidth, imgHeight;
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    /* pointer to native image structure */
    gxpport_image_native_handle newImagePtr;

    KNI_StartHandles(1);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(1, imageData);

    do {
        unsigned char* buffer = (unsigned char*)imageDataArrayPtr;

        gxpport_loadimmutable_from_platformbuffer(
                          buffer, imageDataArrayLength,
                          KNI_TRUE,
						  &imgWidth, &imgHeight,
						  &newImagePtr,
						  &creationError);

        if (IMG_NATIVE_IMAGE_NO_ERROR == creationError) {
	    java_imagedata * dstImageDataPtr = IMGAPI_GET_IMAGEDATA_PTR(imageData);

            dstImageDataPtr->width   = (jint)imgWidth;
            dstImageDataPtr->height  = (jint)imgHeight;
            dstImageDataPtr->nativeImageData = (jint)newImagePtr;
            status = KNI_TRUE;
            break;

        } else if (IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR == creationError) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        } else if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
            KNI_ThrowNew(midpOutOfMemoryError,
                         "Resource limit exceeded for immutable image");
            break;
        } else {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
            break;
        }
    } while (0);

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
}

/**
 * Populates a passed in immutable <tt>ImageData</tt>
 * from the given ARGB integer
 * array. The array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     createImmutableImageDecodeRGBImage([Ljavax/microedition/lcdui/ImageData;III)V
 * </pre>
 *
 * @param imageData The <tt>ImadgeData</tt> to be populated
 * @param rgbData The array of argb image data
 * @param width The width of the new image
 * @param height The height of the new image
 * @param processAlpha If <tt>true</tt>, alpha channel bytes will
 *                     be used, otherwise, alpha channel bytes will
 *                     be ignored
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageDataFactory_createImmutableImageDecodeRGBImage() {
    jboolean processAlpha = KNI_GetParameterAsBoolean(5);
    int height = KNI_GetParameterAsInt(4);
    int width = KNI_GetParameterAsInt(3);
    jint *imageBuffer = NULL;
    int buflen;
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    /* pointer to native image structure */
    gxpport_image_native_handle newImagePtr;

    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, rgbData);
    KNI_GetParameterAsObject(1, imageData);

    do {
        if (KNI_IsNullHandle(rgbData)) {
            KNI_ThrowNew(midpNullPointerException, NULL);
            break;
        }

        if ((width <= 0) || (height <= 0)) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
            break;
        }

        buflen = KNI_GetArrayLength(rgbData);
        if ((width * height) > buflen) {
            KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
            break;
        }

        imageBuffer = JavaIntArray(rgbData);

	SNI_BEGIN_RAW_POINTERS;

        gxpport_decodeimmutable_from_argb(imageBuffer, width, height,
					  processAlpha,
					  &newImagePtr, &creationError);

	SNI_END_RAW_POINTERS;

        if (IMG_NATIVE_IMAGE_NO_ERROR == creationError) {
	    java_imagedata * dstImageDataPtr = IMGAPI_GET_IMAGEDATA_PTR(imageData);

            dstImageDataPtr->height = (jint)height;
            dstImageDataPtr->width = (jint)width;
            dstImageDataPtr->nativeImageData = (jint)newImagePtr;
            break;
        }

        if (IMG_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR == creationError) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
            KNI_ThrowNew(midpOutOfMemoryError,
                         "Resource limit exceeded for immutable image");
            break;
        }

        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } while (0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Creates the native data structures for a new <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     createMutableImage(Ljavax/microedition/lcdui/ImageData;II)V
 * </pre>
 *
 * @param imageData to be populated
 * @param width the width of the new <tt>MutableImage</tt>
 * @param height the height of the new <tt>MutableImage</tt>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageDataFactory_createMutableImageData() {

    int height = KNI_GetParameterAsInt(3);
    int  width = KNI_GetParameterAsInt(2);

    /* pointer to native image structure */
    gxpport_mutableimage_native_handle newImagePtr = NULL;

    /* variable to hold error codes */
    img_native_error_codes creationError = IMG_NATIVE_IMAGE_NO_ERROR;

    if ((width < 0) || (height < 0)) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else {
        /* initialize the internal members of the native image
           structure as required by the platform. */
        gxpport_create_mutable(&newImagePtr,
			       width, height, &creationError);

        if (IMG_NATIVE_IMAGE_RESOURCE_LIMIT == creationError) {
	    KNI_ThrowNew(midpOutOfMemoryError, "Resource limit exceeded for"
					       " Mutable image");
        } else if (IMG_NATIVE_IMAGE_NO_ERROR != creationError) {
	    KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
	    KNI_StartHandles(1);
	    KNI_DeclareHandle(imageData);

	    KNI_GetParameterAsObject(1, imageData);

	    IMGAPI_GET_IMAGEDATA_PTR(imageData)->nativeImageData =
                (jint) newImagePtr;
	    KNI_EndHandles();
        }
    }

    KNI_ReturnVoid();
}

/**
 * Garbage collect any zombie Images to free up their resources.
 * <p>
 * Java declaration:
 * <pre>
 *     garbageCollectImages(Z)V
 * </pre>
 *
 * @param doFullGC boolean indicating whether to do a full GC or not
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageDataFactory_garbageCollectImages() {
    jboolean doFullGC = KNI_GetParameterAsBoolean(1);

    if (doFullGC == KNI_TRUE) {
	JVM_GarbageCollect(0, 0);
    } else {
	JVM_GarbageCollect(JVM_COLLECT_YOUNG_SPACE_ONLY, 0);
    }

    KNI_ReturnVoid();
}
