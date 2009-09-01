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
 * Implementation of Java native methods for the <tt>ImageData</tt> class.
 */


#include <stdlib.h>
#include <sni.h>
#include <midpError.h>
#include <midpUtilKni.h>
#include <Midp_logging.h>
#include <commonKNIMacros.h>

#include <imgapi_image.h>
#include <imgdcd_image_util.h>
#include <img_errorcodes.h>
#include "imgj_rgb.h"

#include "javacall_memory.h"

#define PIXEL imgdcd_pixel_type
#define ALPHA imgdcd_alpha_type

/* Macro to retrieve C structure representation of an Object */
typedef struct Java_javax_microedition_lcdui_ImageData _ImageData;
#define getMidpImageDataPtr(handle) (unhand(_ImageData,(handle)))

/**
 * Gets an ARGB integer array from this <tt>ImmutableImage</tt>. The
 * array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     getRGB([IIIIIII)V
 * </pre>
 *
 * @param rgbData The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlen Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_getRGB) {
    int height = KNI_GetParameterAsInt(7);
    int width = KNI_GetParameterAsInt(6);
    int y = KNI_GetParameterAsInt(5);
    int x = KNI_GetParameterAsInt(4);
    int scanlength = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int *rgbBuffer;
    img_native_error_codes error;


    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetThisPointer(thisObject);


    error = IMG_NATIVE_IMAGE_NO_ERROR;

    SNI_BEGIN_RAW_POINTERS;
    
    rgbBuffer = JavaIntArray(rgbData);
    imgj_get_argb(IMGAPI_GET_IMAGEDATA_PTR(thisObject), rgbBuffer,
		  offset, scanlength,
		  x, y, width, height, &error);

    SNI_END_RAW_POINTERS;
    
    if (error != IMG_NATIVE_IMAGE_NO_ERROR) {
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Get image native data buffer
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_ImageData_nAllocateDataBuffer) {
	jint w, h;
	PIXEL *pixelData = NULL;
	ALPHA *alphaData = NULL;
	jboolean allocateAlpha;
	jboolean result = KNI_FALSE;

	KNI_StartHandles(1);
	KNI_DeclareHandle(imageData);

	allocateAlpha = KNI_GetParameterAsBoolean(1);
	KNI_GetThisPointer(imageData);

    	w = (jint)getMidpImageDataPtr(imageData)->width;
    	h = (jint)getMidpImageDataPtr(imageData)->height;

REPORT_CRIT2(LC_CORE, "nAllocateDataBuffer >> w = %d , h = %d ", w, h);
	do{
    	 	pixelData = (PIXEL *)javacall_malloc(w * h * sizeof(PIXEL));
    	 	if(pixelData == NULL){
    	 		REPORT_INFO(LC_LOWUI, "pdLCDUIimage nAllocateDataBuffer pixel data failed !\n");
    	 		break;
    	 	}

    	 	if(allocateAlpha){
    	 		alphaData = (ALPHA *)javacall_malloc(w * h);
    	 		if(alphaData == NULL){
    	 			javacall_free(pixelData);
    	 			pixelData = NULL;
    	 			REPORT_INFO(LC_LOWUI, "pdLCDUIimage nAllocateDataBuffer alpha data failed !\n");
    	 			break;
    	 		}    	 		
    	 	}
    	 	
    	 	result = KNI_TRUE;
	}while(0);

 	getMidpImageDataPtr(imageData)->nativePixelData = (jint)pixelData;
 	getMidpImageDataPtr(imageData)->nativeAlphaData = (jint)alphaData;	
 	
	KNI_EndHandles();
	KNI_ReturnBoolean(result);
}

 /**
  * Make a copy of native pxiel data.
  */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_nClonePixelData) {
	jint w, h;
	PIXEL *pixelDataSrc = NULL;
	PIXEL *pixelData = NULL;

	KNI_StartHandles(2);
	KNI_DeclareHandle(imageData);
	KNI_DeclareHandle(imageDataSrc);

	KNI_GetThisPointer(imageData);

	KNI_GetParameterAsObject(1, imageDataSrc);
    	w = (jint)getMidpImageDataPtr(imageDataSrc)->width;
    	h = (jint)getMidpImageDataPtr(imageDataSrc)->height;	
	pixelDataSrc = (PIXEL *)getMidpImageDataPtr(imageDataSrc)->nativePixelData;

	if(pixelDataSrc != NULL){
		pixelData = (PIXEL *)javacall_malloc(w * h * sizeof(PIXEL));
		if(pixelData != NULL){
			memcpy(pixelData, pixelDataSrc, w * h * sizeof(PIXEL));
		}else{
			KNI_ThrowNew(midpOutOfMemoryError, NULL);
		}
		getMidpImageDataPtr(imageData)->nativePixelData = (jint)pixelData;
	}

	KNI_EndHandles();
	KNI_ReturnVoid();	
 }
 
 /**
  * Set each pixel data to 0xff.
  */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_nClearPixelData) {
	jint w, h;
	PIXEL *pixelData = NULL;

	KNI_StartHandles(1);
	KNI_DeclareHandle(imageData);

	KNI_GetThisPointer(imageData);

    	w = (jint)getMidpImageDataPtr(imageData)->width;
    	h = (jint)getMidpImageDataPtr(imageData)->height;	
	pixelData = (PIXEL *)getMidpImageDataPtr(imageData)->nativePixelData;

	if(pixelData != NULL){
		memset((char*)pixelData, 0xFF, w * h * sizeof(PIXEL));
	}

	KNI_EndHandles();
	KNI_ReturnVoid();	
 }

 /**
  * Removes native alpha data information
  */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_nRemoveAlpha) {
	ALPHA *alphaData = NULL;

	KNI_StartHandles(1);
	KNI_DeclareHandle(imageData);

	KNI_GetThisPointer(imageData);

	alphaData = (PIXEL *)getMidpImageDataPtr(imageData)->nativeAlphaData;

	if(alphaData != NULL){
		javacall_free(alphaData);
	}
	getMidpImageDataPtr(imageData)->nativeAlphaData = NULL;

	KNI_EndHandles();
	KNI_ReturnVoid();	
 }
 
 /**
  * Cleanup after garbage collected instance.
  */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageData_finalize) {
	PIXEL *pixelData = NULL;
	ALPHA *alphaData = NULL;
	
	KNI_StartHandles(1);
	KNI_DeclareHandle(imageData);

	KNI_GetThisPointer(imageData);

	pixelData = (PIXEL *)((const java_imagedata*)imageData)->nativePixelData;
	if(pixelData != NULL){
		javacall_free(pixelData);
	}

	alphaData = (ALPHA *)((const java_imagedata*)imageData)->nativeAlphaData;
	if(alphaData != NULL){
		javacall_free(alphaData);
	}

	KNI_EndHandles();
	KNI_ReturnVoid();
}
