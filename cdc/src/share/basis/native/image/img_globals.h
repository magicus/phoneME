/*
 * @(#)img_globals.h	1.24 06/10/04
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

/*
 * This file provides some global definitions needed by the image
 * conversion package.
 */

#ifndef _IMG_GLOBALS_H_
#define _IMG_GLOBALS_H_

#include "jni.h"


/* Image Conversion function return codes. */
#define SCALEFAILURE	-1
#define SCALENOOP	0
#define SCALESUCCESS	1

/*
 * The constants needed to choose from among the many variants of image
 * conversion functions that can be constructed with the standard header
 * files.  The types of input for the image conversion functions are
 * broken down into 5 different attributes each with 2 to 4 different
 * variants:
 *
 *	SCALING:	SCALED or UNSCALED
 *	INPUT SIZE:	BYTEIN (8-bit) or INTIN (32-bit)
 *	ALPHA:		OPAQUE or ALPHA
 *	ORDER:		TDLR or RANDOM
 *	COLORMODEL:	ICM, DCM, DCM8 (8-bits for each component) or ANY
 *
 * For each attribute, a mask is defined with the "BITS" suffix which
 * identifies which bits contain the variation information for that
 * particular attribute.  The input information should be analyzed and
 * characterized for each of the above categories and the appropriate
 * bit constants OR'd together to produce a unique constant that
 * identifies which conversion function is needed.  The reason that
 * attributes of the output space are not indicated in the masks is
 * that typically only a single output device type needs to be supported
 * at a time and so a vector of the functions specific to the necessary
 * output device can be constructed at AWT initialization time and then
 * indexed into with the constant identifier that characterizes the
 * input data, which is only known and constantly varies at run-time.
 */
#define IMGCV_UNSCALED		(0 << 0)
#define IMGCV_SCALED		(1 << 0)
#define IMGCV_SCALEBITS		(1 << 0)
#define IMGCV_BYTEIN		(0 << 1)
#define IMGCV_INTIN		(1 << 1)
#define IMGCV_INSIZEBITS	(1 << 1)
#define IMGCV_OPAQUE		(0 << 2)
#define IMGCV_ALPHA		(1 << 2)
#define IMGCV_ALPHABITS		(1 << 2)
#define IMGCV_TDLRORDER		(0 << 3)
#define IMGCV_RANDORDER		(1 << 3)
#define IMGCV_ORDERBITS		(1 << 3)
#define IMGCV_ICM		(0 << 4)
#define IMGCV_DCM		(1 << 4)
#define IMGCV_DCM8		(2 << 4)
#define IMGCV_ANYCM		(3 << 4)
#define IMGCV_CMBITS		(3 << 4)

#define NUM_IMGCV		(1 << 6)	/* total # of IMGCV variants */

/*
 * The structure which holds the image conversion data.
 */
typedef struct {
    void *outbuf;
    void *maskbuf;
    void *fserrors;
    void *outbuf_end;
    void *mask_end;
} ImgConvertData;

/*
 * The standard structure which holds information about the pixels
 * used in the output device.
 */
typedef struct {
    int grayscale;
    int bitsperpixel;
    int rOff;
    int gOff;
    int bOff;
    int rScale;
    int gScale;
    int bScale;
} ImgColorData;

/*
 * ImageCachedIDs structure containing IDs for methods, fields and classes necessary
 * to access the corresponding Java code
 *
 * These are coming off java.awt.image unless otherwise specified.

 */

struct ImageCachedIDs
{
    jclass      ColorModel;

    jfieldID    ColorModel_pDataFID;
    jfieldID    ColorModel_pixel_bitsFID;
    jmethodID   ColorModel_getRGBMID;

    jclass      IndexColorModel;

    jfieldID    IndexColorModel_rgbFID;
    jfieldID    IndexColorModel_map_sizeFID;
    jfieldID    IndexColorModel_opaqueFID;
    jfieldID    IndexColorModel_transparent_indexFID;
    jmethodID   IndexColorModel_constructorMID;

    jclass      DirectColorModel;

    jfieldID    DirectColorModel_red_maskFID;
    jfieldID    DirectColorModel_green_maskFID;
    jfieldID    DirectColorModel_blue_maskFID;
    jfieldID    DirectColorModel_alpha_maskFID;
    jfieldID    DirectColorModel_red_offsetFID;
    jfieldID    DirectColorModel_green_offsetFID;
    jfieldID    DirectColorModel_blue_offsetFID;
    jfieldID    DirectColorModel_alpha_offsetFID;
    jfieldID    DirectColorModel_red_scaleFID;
    jfieldID    DirectColorModel_green_scaleFID;
    jfieldID    DirectColorModel_blue_scaleFID;
    jfieldID    DirectColorModel_alpha_scaleFID;
    jmethodID   DirectColorModel_constructorMID;
};


/*
 * The private data member attached to a ColorModel which caches
 * the information needed to characterize and use a ColorModel
 * object on the fly.
 */
typedef struct {
    int type;
    jmethodID getRGB;
} ImgCMData;

/*
 * The standard signature of all of the image conversion functions
 * that can be produced with this package of include files.
 */
typedef int ImgConvertFcn(JNIEnv *env, jobject img, jobject colormodel,
			  int srcOX, int srcOY, int srcW, int srcH,
			  void *srcpix, int srcOff, int srcBPP, int srcScan,
			  int srcTotalWidth, int srcTotalHeight,
			  int dstTotalWidth, int dstTotalHeight,
			  ImgConvertData *cvdata, ImgColorData *clrdata);

/*
 * The type of the error matrix used in the ordered dithering code.
 * Beware: if you change the type of these constants, the code that
 * deals with dither arrays will also need to be changed.
 */
#define DITHER_ARRAY_WIDTH  8
#define DITHER_ARRAY_HEIGHT 8

typedef unsigned char uns_ordered_dither_array[DITHER_ARRAY_HEIGHT][DITHER_ARRAY_WIDTH];

typedef char sgn_ordered_dither_array[DITHER_ARRAY_HEIGHT][DITHER_ARRAY_WIDTH];

/*
 * The function provided for constructing the ordered dithering error
 * matrices based on a given quantum (i.e. the amplitude of the maximum
 * error values appearing in the matrix which should be the same as the
 * distance between adjacent allocated component values in the color cube).
 */
extern void make_uns_ordered_dither_array(uns_ordered_dither_array oda, int quantum);
extern void make_sgn_ordered_dither_array(sgn_ordered_dither_array oda, int errmin, int errmax);

/*
 * The function provided for calculating the contents of the ImgCMData
 * structure which can be attached to ColorModels to simplify the
 * work of characterizing their data.
 */
extern ImgCMData *img_getCMData(JNIEnv *env, jobject colormodel);

#endif /* _IMG_GLOBALS_H_ */
