/*
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


#ifndef __JPEGDECODER_H__
#define __JPEGDECODER_H__

void * JPEG_To_RGB_init();

/**
 * Decodes a jpeg header, 
 * fills all interna fields related to input image. 
 * a useful return is decoded image width & height
 *
 * @param info handle returned from JPEG_To_RGB_init
 * @param inData JPEG data
 * @param inDataLen length of inData
 * @param width pointer where to store decoded image width
 * @param height pointer where to store decoded image height
 *
 * @return non-zero on success, zeot on failure

 */
int JPEG_To_RGB_decodeHeader(void *info, char *inData, int inDataLen, 
    int* width, int* height);

/**
 * Decodes a jpeg data to the provided buffer.
 * Assumes that JPEG_To_RGB_decodeHeader() has been called before,
 * and outData contains buffer of a valid size.
 *
 * @param info handle returned from JPEG_To_RGB_init
 * @param outData long 24 bit RGB image
 *
 * @return size of filled outData bytes, 0 when failed
 */
int JPEG_To_RGB_decodeData(void *info, char *outData);

/**
 * Decodes a jpeg data to the provided buffer.
 * Assumes that JPEG_To_RGB_decodeHeader() has been called before,
 * and outData contains buffer of a valid size.
 *
 * @param info handle returned from JPEG_To_RGB_init
 * @param outData short 16 (5,6,5) or long 32 bit RGB image
 * @param outPixelSize the desired pixel size in bytes, 2 or 4
 * @param left -
 * @param top -
 * @param right -
 * @param bottom - rectange in the decoded image that will be copied to outData
 *
 * @return size of filled outData bytes, 0 when failed
 */
int JPEG_To_RGB_decodeData2(void *info, char *outData, int outPixelSize,
    int left, int top, int right, int bottom);


/**
 * Decodes a jpeg into the provided buffer.
 * Call JPEG_ToRGB_decodedSize to get the correct size for the image.
 *
 * @param info handle returned from JPEG_To_RGB_init
 * @param inData JPEG data
 * @param inDataLen length of inData
 * @param width pointer where to store decoded image width
 * @param height pointer where to store decoded image height
 *
 * @return allocated long 32 bit RGB image buffer 
           when successful, NULL when failed
 */
char * JPEG_To_RGB_decode(void *info, char *inData, int inDataLen,
                          int *width, int *height);

/**
 * Decodes a jpeg into the provided buffer.
 * Call JPEG_ToRGB_decodedSize to get the correct size for the image.
 *
 * @param info handle returned from JPEG_To_RGB_init
 * @param inData JPEG data
 * @param inDataLen length of inData
 * @param outPixelSize the desired pixel size in bytes, 2 or 4
 * @param left -
 * @param top -
 * @param right -
 * @param bottom - rectange in the decoded image that will be copied to outData
 * @param width pointer where to store decoded image width
 * @param height pointer where to store decoded image height
 *
 * @return allocated short 16 (5,6,5) or long 32 bit RGB image buffer 
           when successful, NULL when failed
 */
char* JPEG_To_RGB_decode2(void *info, 
    char *inData, int inDataLen, int outPixelSize,
    int left, int top, int right, int bottom, 
    int *width, int *height);

void JPEG_To_RGB_free(void *cinfo);

#endif /* __JPEGDECODER_H__ */
