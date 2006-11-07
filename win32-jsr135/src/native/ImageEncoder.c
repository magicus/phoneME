/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * @file ImageEncoder.c
 *
 * This file implements native functionality required by 
 * ImageEncoder.java.
 * It performs image/audio/video format conversion.
 */

#include "mni.h"

/* #define WRITE_SNAPSHOT_TO_FILE */

#ifdef WRITE_SNAPSHOT_TO_FILE
#include <stdio.h>
#endif

#include "jpeg_encoder.h"
#include "png_encoder.h"

#define SIZEOFPIXEL 4

#define CONVERT_RGB_TO_JPEG 1
#define CONVERT_RGB_TO_PNG  2

#ifdef WRITE_SNAPSHOT_TO_FILE
void dumpToFile(char* dataArray, int dataSize, int encType) {
    char* fname = (encType == CONVERT_RGB_TO_JPEG) 
            ? "d:\\temp\\tmp.jpg"
            : (encType == CONVERT_RGB_TO_PNG) 
                ? "d:\\temp\\tmp.png"
                : "d:\\temp\\tmp.raw";
    FILE *fp = fopen(fname, "wb");
    fwrite(dataArray, 1, dataSize, fp);
    fclose(fp);
    printf("ImageEncoder wrote image to local file %s !\n", fname);
}
#endif


/**
 * Compresses an RGB image into JPEG or PNG format.
 * This function makes use of the IJG JPEG compression library to
 * compress the given RGB image (in the form of a byte array) into
 * a JPEG or PNG image.
 *
 * @param inArray The RGB pixels in 32 bit XRGB format (4 bytes per pixel).
 * @param jwidth  The width of the image.
 * @param jheight  The height of the image.
 * @param jquality The required quality of the compression. This is a value
 *                 between 1 and 100 (for JPEG only).
 * @param outArray The array to which the compressed data is written. Needs to
 *                 be as big as the input RGB frame (width x height).
 * @return Returns the size of the compressed data in bytes. If there was
 * a failure, returns 0.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_ImageEncoder_RGBByteCompress
(MNI_FUNCTION_PARAMS_6(jbyteArray inArray,
		       jint jwidth,
		       jint jheight,
		       jint jquality,
		       jbyteArray outArray,
		       jint jencType))
{
    int MNI_GET_INT_PARAM(width, jwidth, 2);
    int MNI_GET_INT_PARAM(height, jheight, 3);
    int MNI_GET_INT_PARAM(quality, jquality, 4);
    int MNI_GET_INT_PARAM(encType, jencType, 6);
    char *inData = NULL;
    char *outData = NULL;
    int result = 1;


    /* Don't know how to avoid this since it's not
       possible to nest MNI_GET_***_ARRAY_PARAM. */
#ifdef CLDC
    int array_len_in;
    int array_len_out;
    KNI_StartHandles(2);
    KNI_DeclareHandle(arrHandleIn);
    KNI_DeclareHandle(arrHandleOut);
    KNI_GetParameterAsObject(1, arrHandleIn);
    KNI_GetParameterAsObject(5, arrHandleOut);
    array_len_in = KNI_GetArrayLength(arrHandleIn);
    array_len_out = KNI_GetArrayLength(arrHandleOut);
    result = (int) (inData = (jbyte *) MNI_MALLOC(array_len_in));
    if (result)
	result = (int) (outData = (jbyte *) MNI_MALLOC(array_len_out));
    if (result)
	KNI_GetRawArrayRegion(arrHandleIn, 0, array_len_in, (jbyte *)inData);
#endif

    /* Do the actual decoding */
    if (result) {
	if (encType == CONVERT_RGB_TO_JPEG)
	    result = RGBToJPEG(
                inData, 
                width, height, 
                quality, 
                outData,
		JPEG_ENCODER_COLOR_XRGB);
	else
	if (encType == CONVERT_RGB_TO_PNG)
	    result = RGBToPNG(
                inData, 
                outData, 
                width, height,
                PNG_ENCODER_COLOR_XRGB);
        else {
        }
    }

#ifdef WRITE_SNAPSHOT_TO_FILE
    if (result)
        dumpToFile(outData, result, encType);
#endif
   
#ifdef CLDC
    if (result)
	KNI_SetRawArrayRegion(arrHandleOut, 0, result, (jbyte*) outData);
    KNI_EndHandles();
    if (outData)
	MNI_FREE(outData);
    if (inData)
	MNI_FREE(inData);
#else
    (*env)->ReleaseIntArrayElements(env, inArray, inData, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env, outArray, (jbyte*) outData,
				     (result > 0) ? 0 : JNI_ABORT);
#endif

    MNI_RET_VALUE_INT(result);
}

/**
 * Compresses an RGB image into JPEG format.
 * This function makes use of the IJG JPEG compression library to
 * compress the given RGB image (in the form of an integer array) into
 * a JPEG image.
 *
 * @param inArray The RGB pixels in 32 bit XRGB format (one int per pixel).
 * @param jwidth  The width of the image.
 * @param jheight  The height of the image.
 * @param jquality The required quality of the compression. This is a value
 *                 between 1 and 100.
 * @param outArray The array to which the compressed data is written. Needs to
 *                 be as big as the input RGB frame (width x height x 4).
 * @return Returns the size of the compressed data in bytes. If there was
 * a failure, returns 0.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_ImageEncoder_RGBIntCompress
(MNI_FUNCTION_PARAMS_6(jintArray inArray,
		       jint jwidth,
		       jint jheight,
		       jint jquality,
		       jbyteArray outArray,
		       jint jencType))
{
    int MNI_GET_INT_PARAM(width, jwidth, 2);
    int MNI_GET_INT_PARAM(height, jheight, 3);
    int MNI_GET_INT_PARAM(quality, jquality, 4);
    int MNI_GET_INT_PARAM(encType, jencType, 6);
    int *inData = NULL;
    char *outData = NULL;
    int result = 1;


    /* Don't know how to avoid this since it's not
       possible to nest MNI_GET_***_ARRAY_PARAM. */
#ifdef CLDC
    int array_len_in;
    int array_len_out;
    KNI_StartHandles(2);
    KNI_DeclareHandle(arrHandleIn);
    KNI_DeclareHandle(arrHandleOut);
    KNI_GetParameterAsObject(1, arrHandleIn);
    KNI_GetParameterAsObject(5, arrHandleOut);
    array_len_in = KNI_GetArrayLength(arrHandleIn);
    array_len_out = KNI_GetArrayLength(arrHandleOut);
    result = (int) (inData = (int *) MNI_MALLOC(array_len_in * SIZEOFPIXEL));
    if (result)
	result = (int) (outData = (jbyte *) MNI_MALLOC(array_len_out));
    if (result)
	KNI_GetRawArrayRegion(arrHandleIn, 0, array_len_in * SIZEOFPIXEL, (jbyte *)inData);
#endif

    /* Do the actual decoding */
    if (result) {
	if (encType == CONVERT_RGB_TO_JPEG)
            /* 
             * WARNING: BGRX format is specific to int[]-based little-endian !
             * on others it will be XRGB !
             * Here we have (#if/then/else) directive to choose
             * XRGB or BGRX depending on platform endianess !
             */
#ifdef JM_LITTLE_ENDIAN
	    result = RGBToJPEG((char*)inData, width, height, quality, outData,
			       JPEG_ENCODER_COLOR_BGRX);
#else
	    result = RGBToJPEG((char*)inData, width, height, quality, outData,
			       JPEG_ENCODER_COLOR_XRGB);
#endif /*JM_LITTLE_ENDIAN*/
	else /* if (encType == CONVERT_RGB_TO_PNG) */
	    result = RGBToPNG(
                (char*)inData, 
                (char*)outData, 
                width, height,
                PNG_ENCODER_COLOR_BGRX);
    }

#ifdef WRITE_SNAPSHOT_TO_FILE
    if (result)
        dumpToFile(outData, result, encType);
#endif
    
#ifdef CLDC
    if (result)
	KNI_SetRawArrayRegion(arrHandleOut, 0, result, (jbyte*) outData);
    KNI_EndHandles();
    if (outData)
	MNI_FREE(outData);
    if (inData)
	MNI_FREE(inData);
#else
    (*env)->ReleaseIntArrayElements(env, inArray, inData, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env, outArray, (jbyte*) outData,
				     (result > 0) ? 0 : JNI_ABORT);
#endif

    MNI_RET_VALUE_INT(result);
}

