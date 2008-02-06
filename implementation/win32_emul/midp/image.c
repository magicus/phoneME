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


 /**
  * Win32 implementation supports the following image formats: jpeg,png
 */

#include <stdio.h>
#include <windows.h>
#include "javacall_image.h"
#include <jpegdecoder.h>

    /**
     * PNG Header Data
     */
    unsigned char pngHeader[] ={
         (unsigned char)0x89, (unsigned char)0x50, (unsigned char)0x4e, (unsigned char)0x47,
         (unsigned char)0x0d, (unsigned char)0x0a, (unsigned char)0x1a, (unsigned char)0x0a,
         (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x0d,
         (unsigned char)0x49, (unsigned char)0x48, (unsigned char)0x44, (unsigned char)0x52
    };
    int pngHeaderSize = 16;
    /**
     * JPEG Header Data
     */
    unsigned char jpegHeader[] ={
         (unsigned char)0xff, (unsigned char)0xd8, (unsigned char)0xff, (unsigned char)0xe0
    };
    int jpegHeaderSize = 4;

    /**
     * RAW Header Data
     */
    unsigned char rawHeader[] = {
         (unsigned char)0x89, (unsigned char)0x53, (unsigned char)0x55, (unsigned char)0x4E
    };

    int rawHeaderSize = 4;

    typedef struct _imageData{
        int width;
        int height;
        char* pixelData;
        int pixelDataSize;
    }imageData;

/**
 * Function to compare byte data to the given header
 */
javacall_result headerMatch(unsigned char* header,int headerSize,
                                unsigned char* imageData,
                                int imageLength) {
    int i=0;
    if (imageLength < headerSize) {
        return JAVACALL_FAIL;
    }
    for (i = 0; i < headerSize; i++) {
        if (imageData[i] != header[i]) {
            return JAVACALL_FAIL;
        }
    }
    return JAVACALL_OK;
}
javacall_result initPNGImageInfo(unsigned char* source,long sourceSize,javacall_image_info* imageInfo){
    int width = ((source[16] & 0x0ff) << 24) +
                    ((source[17] & 0x0ff) << 16) +
                    ((source[18] & 0x0ff) <<  8) +
                    (source[19] & 0x0ff);

    int height = ((source[20] & 0x0ff) << 24) +
                     ((source[21] & 0x0ff) << 16) +
                     ((source[22] & 0x0ff) <<  8) +
                     (source[23] & 0x0ff);

        if (width <= 0 || height <= 0) {
            return JAVACALL_FAIL;
        }
        imageInfo->width = width;
        imageInfo->height = height;
        return JAVACALL_OK;

}


javacall_result initJPEGImageInfo(unsigned char* source,long sourceSize,javacall_image_info* imageInfo){
        int width = 0;
        int height = 0;
        int field_len = 0;
        int idx = 2;
        unsigned char code;

        while (idx + 8 <sourceSize) {
            if (source[idx] != (unsigned char)0xff) {
                break;
            }

            if ((unsigned char) (source[idx + 1] & 0xf0) == (unsigned char) 0xc0) {
                code = source[idx + 1];

                if (code != (unsigned char) 0xc4 || code != (unsigned char) 0xcc) {
                    /* Big endian */
                    height = ((source[idx + 5] & 0xff) << 8) +
                            (source[idx + 6] & 0xff);
                    width = ((source[idx + 7] & 0xff) << 8) +
                            (source[idx + 8] & 0xff);
                    break;
                }
            }

            /* Go to the next marker */
            field_len = ((source[idx + 2] & 0xff) << 8) +
                    (source[idx + 3] & 0xff);
            idx += field_len + 2;
        }

        if (width <= 0 || height <= 0) {
            return JAVACALL_FAIL;
        }
        imageInfo->width = width;
        imageInfo->height = height;
        return JAVACALL_OK;

}

/**
 * Get image information
 *
 * @param source        Pointer to image data source
 * @param sourceSize    Size of source in bytes
 * @param imageInfo     Pointer to javacall_image_info type buffer
 *
 * @retval JAVACALL_OK
 *          imageData is acceptable format and image information returned by imageInfo argument
 * @retval JAVACALL_FAIL
 *          imageData is not acceptable
 */
javacall_result javacall_image_get_info(const void* source,
                                        long sourceSize,
                                        /*OUT*/ javacall_image_info* imageInfo){
    javacall_result res= JAVACALL_FAIL;
    //check what is the image format
    if (headerMatch(pngHeader,pngHeaderSize,(unsigned char*)source,sourceSize)==JAVACALL_OK){
        res = initPNGImageInfo((unsigned char*)source,sourceSize,imageInfo);
        res = JAVACALL_FAIL;// currently supporting only jpeg via native image decoder

    }else if(headerMatch(jpegHeader,jpegHeaderSize,(unsigned char*)source,sourceSize)==JAVACALL_OK){//JPEG
        res = initJPEGImageInfo((unsigned char*)source,sourceSize,imageInfo);

    }

    return res;
}

/**
 * Start a decoding transaction of the source image to array of javacall_pixel type
 *
 * For Synchronous decoding:
 * 1.Copy the address of the source buffer to a static variable (also the handle can be used for that).
 * 2.Return JAVACALL_OK
 *
 * For Asynchronous decoding:
 * 1.Allocate a memory block of size sourceSize
 * 2.Copy the source buffer to the new allocated buffer.
 * 3.Send the buffer to the decoder which is in another task.
 * 4.Mark the decoding operation with a unique handle.
 * 5.Return JAVACALL_WOULD_BLOCK

 * @param source        Pointer to image data source
 * @param handle        Handle of this image decoding. This could be used from async decoding mode
 *                      It could return a NULL value, if this API return JAVACALL_OK
 *
 *
 * @retval JAVACALL_OK          Decoding OK! - Synchronous decoding
 * @retval JAVACALL_FAIL        Fail
 * @retval JAVACALL_WOULD_BLOCK Image decoding performed asynchronously
 */
javacall_result javacall_image_decode_start(const void* source,
                                            long sourceSize,
                                            long width,
                                            long height,
                                            /*OUT*/ javacall_handle* handle){
    imageData* image= malloc(sizeof(imageData));
    image->width = width;
    image->height = height;
    image->pixelDataSize = sourceSize;
    image->pixelData =(char*) malloc(sourceSize*sizeof(char));
    if (image->pixelData != NULL) {
        memcpy(image->pixelData,source,sourceSize*sizeof(char));
        *handle = (void*)image;
        return JAVACALL_OK;
    }
    return JAVACALL_FAIL;
}

/**
 * Finalize the image decoding transaction
 *
 * For Synchronous decoding:
 *   1.This function will be called right after javacall_image_decode_start() returns with JAVACALL_OK.
 *   2.Decode the source buffer that was stored previously into decodeBuf , alphaBuf.
 * For Asynchronous decoding:
 *   1.This function will be called after javanotify_on_image_decode_end is invoked.
 *   2.The platform should only copy the decoded image to decodeBuf , alphaBuf.
 *
 * @param handle        The handle get from javacall_image_decode_start
 * @param decodeBuf     Pointer to decoding target buffer
 * @param decodeBufSize Size of decodeBuf in bytes
 * @param alphaBuf      Pointer to alpha data buffer
 *                      It could be a NULL if there is no need to get alpha data
 * @param alphaBufSize  Size of alphaBuf in bytes
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_image_decode_finish(javacall_handle handle,
                                             /*OUT*/ javacall_pixel* decodeBuf,
                                             long decodeBufSize,
                                             /*OUT*/ char* alphaBuf,
                                             long alphaBufSize){

    javacall_result result = JAVACALL_FAIL;
    void *info ;
    imageData* image = (imageData*)handle;

    int i;
    for (i=0;i<alphaBufSize;i++) {//initialize alpha buffer
        alphaBuf[i]=(char)0xFF;
    }
    if (headerMatch(jpegHeader,jpegHeaderSize,image->pixelData,image->pixelDataSize)== JAVACALL_OK) {
        info = JPEG_To_RGB_init();
        if (info) {
            int width, height;
            if (JPEG_To_RGB_decodeHeader(info,image->pixelData, image->pixelDataSize,
                &width, &height) != 0) {
                if (JPEG_To_RGB_decodeData2(info,(char*)decodeBuf,
                    sizeof(unsigned short), 0, 0, image->width, image->height) != 0) {
                    result = JAVACALL_OK;
                }
            }
            JPEG_To_RGB_free(info);
        }
    }else if (headerMatch(pngHeader,pngHeaderSize,image->pixelData,image->pixelDataSize)==JAVACALL_OK) {
         result = JAVACALL_FAIL;
    }
    return result;
}
