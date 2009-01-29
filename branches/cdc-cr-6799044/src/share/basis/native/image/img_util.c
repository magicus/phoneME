/*
 * @(#)img_util.c	1.22 06/10/10
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
 *
 */
#include <math.h>

#include "Xheaders.h"
#include "common.h"
#include "img_util.h"
#include "img_util_md.h"

sgn_ordered_dither_array img_oda_red;
sgn_ordered_dither_array img_oda_green;
sgn_ordered_dither_array img_oda_blue;
uns_ordered_dither_array img_oda_alpha;


struct ImageCachedIDs ImageCachedIDs;

pixelToRGBConverter     pixel2RGB  = 0;
rgbToPixelConverter     RGB2Pixel;
javaRGBToPixelConverter jRGB2Pixel;

int                     numColors;
int                     grayscale;


int                     nlc = 0;
jobject                 native_lock = 0;
jobject                 lock_class;


jmethodID               lock_waitMID;
jmethodID               lock_notifyMID;
jmethodID               lock_notifyAllMID;

unsigned long           mapped_pixels[256];

extern   JavaVM   *JVM;

int check_locking_thread(jobject lock) {
    JNIEnv *env;
    int rval;

    (*JVM)->AttachCurrentThread(JVM, (void **)&env, NULL);
    (*env)->MonitorEnter(env, lock);
    rval = nlc > 0;
    (*env)->MonitorExit(env, lock);

    return rval;
}



#if 0
static void createNativeLock(JNIEnv *env, jobject lockObj) {
    jclass tmp;

    if (lockObj == 0) {
	fprintf(stderr, "WHOA! Lock object is null!?\n");
	fflush(stderr);
    }

    tmp = (*env)->GetObjectClass(env, lockObj);

    if (tmp == 0) {
	fprintf(stderr, "WHOA! No class for object?\n");
	fflush(stderr);
    }

    native_lock = (*env)->NewGlobalRef(env, lockObj);
    lock_class  = (*env)->NewGlobalRef(env, tmp);

    if (lock_class == 0) {
	fprintf(stderr, "Failed to create global ref for lock object class\n");
	fflush(stderr);
    }

    lock_waitMID = (*env)->GetMethodID(env, lock_class, "wait", "(J)V");
    lock_notifyMID = (*env)->GetMethodID(env, lock_class, "notify", "()V");
    lock_notifyAllMID = (*env)->GetMethodID(env, lock_class, "notifyAll", "()V");

}

#endif

void initColorModels(JNIEnv *env)
{
    jclass     cls;

    if((cls = (*env)->FindClass(env, "java/awt/image/ColorModel"))==NULL) return;

    if((ImageCachedIDs.ColorModel=(*env)->NewGlobalRef(env, cls))==NULL) return;

    if((cls = (*env)->FindClass(env, "java/awt/image/IndexColorModel"))==NULL) return;

    if((ImageCachedIDs.IndexColorModel=(*env)->NewGlobalRef(env, cls))==NULL) return;

    if((cls = (*env)->FindClass(env, "java/awt/image/DirectColorModel"))==NULL) return;

    if((ImageCachedIDs.DirectColorModel=(*env)->NewGlobalRef(env, cls))==NULL) return;


    ImageCachedIDs.ColorModel_pDataFID = (*env)->GetFieldID(env, ImageCachedIDs.ColorModel, "pData", "I");
    if(ImageCachedIDs.ColorModel_pDataFID == NULL) return;

    ImageCachedIDs.ColorModel_pixel_bitsFID = (*env)->GetFieldID(env, ImageCachedIDs.ColorModel, "pixel_bits", "I");
    if(ImageCachedIDs.ColorModel_pixel_bitsFID == NULL) return;

    ImageCachedIDs.ColorModel_getRGBMID = (*env)->GetMethodID(env, ImageCachedIDs.ColorModel, "getRGB", "(I)I");
    if(ImageCachedIDs.ColorModel_getRGBMID == NULL) return;


    ImageCachedIDs.IndexColorModel_rgbFID = (*env)->GetFieldID(env, ImageCachedIDs.IndexColorModel, "rgb", "[I");
    if(ImageCachedIDs.IndexColorModel_rgbFID == NULL) return;

    ImageCachedIDs.IndexColorModel_map_sizeFID = (*env)->GetFieldID(env, ImageCachedIDs.IndexColorModel, "map_size", "I");
    if(ImageCachedIDs.IndexColorModel_map_sizeFID == NULL) return;

    ImageCachedIDs.IndexColorModel_opaqueFID = (*env)->GetFieldID(env, ImageCachedIDs.IndexColorModel, "opaque", "Z");
    if(ImageCachedIDs.IndexColorModel_opaqueFID == NULL) return;

    ImageCachedIDs.IndexColorModel_transparent_indexFID = (*env)->GetFieldID(env, ImageCachedIDs.IndexColorModel, "transparent_index", "I");
    if(ImageCachedIDs.IndexColorModel_transparent_indexFID == NULL) return;

    ImageCachedIDs.IndexColorModel_constructorMID = (*env)->GetMethodID(env, ImageCachedIDs.IndexColorModel, "<init>", "(II[B[B[B)V");
    if(ImageCachedIDs.IndexColorModel_constructorMID == NULL) return;



    ImageCachedIDs.DirectColorModel_red_maskFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "red_mask", "I");
    if(ImageCachedIDs.DirectColorModel_red_maskFID == NULL) return;

    ImageCachedIDs.DirectColorModel_green_maskFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "green_mask", "I");
    if(ImageCachedIDs.DirectColorModel_green_maskFID == NULL) return;

    ImageCachedIDs.DirectColorModel_blue_maskFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "blue_mask", "I");
    if(ImageCachedIDs.DirectColorModel_blue_maskFID == NULL) return;

    ImageCachedIDs.DirectColorModel_alpha_maskFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "alpha_mask", "I");
    if(ImageCachedIDs.DirectColorModel_alpha_maskFID == NULL) return;



    ImageCachedIDs.DirectColorModel_red_offsetFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "red_offset", "I");
    if(ImageCachedIDs.DirectColorModel_red_offsetFID == NULL) return;

    ImageCachedIDs.DirectColorModel_green_offsetFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "green_offset", "I");
    if(ImageCachedIDs.DirectColorModel_green_offsetFID == NULL) return;

    ImageCachedIDs.DirectColorModel_blue_offsetFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "blue_offset", "I");
    if(ImageCachedIDs.DirectColorModel_blue_offsetFID == NULL) return;

    ImageCachedIDs.DirectColorModel_alpha_offsetFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "alpha_offset", "I");
    if(ImageCachedIDs.DirectColorModel_alpha_offsetFID == NULL) return;



    ImageCachedIDs.DirectColorModel_red_scaleFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "red_scale", "I");
    if(ImageCachedIDs.DirectColorModel_red_scaleFID == NULL) return;

    ImageCachedIDs.DirectColorModel_green_scaleFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "green_scale", "I");
    if(ImageCachedIDs.DirectColorModel_green_scaleFID == NULL) return;

    ImageCachedIDs.DirectColorModel_blue_scaleFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "blue_scale", "I");
    if(ImageCachedIDs.DirectColorModel_blue_scaleFID == NULL) return;

    ImageCachedIDs.DirectColorModel_alpha_scaleFID = (*env)->GetFieldID(env, ImageCachedIDs.DirectColorModel, "alpha_scale", "I");
    if(ImageCachedIDs.DirectColorModel_alpha_scaleFID == NULL) return;


    ImageCachedIDs.DirectColorModel_constructorMID = (*env)->GetMethodID(env, ImageCachedIDs.DirectColorModel, "<init>", "(IIIII)V");
    if(ImageCachedIDs.DirectColorModel_constructorMID == NULL) return;

}


jboolean isIndexColorModel(JNIEnv *env, jobject obj) {
    return (*env)->IsInstanceOf(env, obj, ImageCachedIDs.IndexColorModel);

  /*    return INSTANCEOF(obj, java_awt_image_IndexColorModel); */
}

jboolean isDirectColorModel(JNIEnv *env, jobject obj) {
    return (*env)->IsInstanceOf(env, obj, ImageCachedIDs.DirectColorModel);

  /*    return INSTANCEOF(obj, java_awt_image_DirectColorModel); */
}

int getRGBFromPixel(JNIEnv *env, jobject obj, unsigned int pixel) {

    return (*env)->CallIntMethod(env, obj, ImageCachedIDs.ColorModel_getRGBMID);

    /*    return CALLMETHOD1(Int, obj, java_awt_image_ColorModel, getRGB, (jint)pixel); */
}

void getDCMParams(JNIEnv *env, jobject obj,
		  int *red_mask, int *red_off, int *red_scale,
		  int *green_mask, int *green_off, int *green_scale,
		  int *blue_mask, int *blue_off, int *blue_scale,
		  int *alpha_mask, int *alpha_off, int *alpha_scale) 
{
    *red_mask    = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_red_maskFID);
    *green_mask  = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_green_maskFID);
    *blue_mask   = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_blue_maskFID);
    *alpha_mask  = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_alpha_maskFID);

    *red_off     = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_red_offsetFID);
    *green_off   = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_green_offsetFID);
    *blue_off    = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_blue_offsetFID);
    *alpha_off   = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_alpha_offsetFID);

    *red_scale   = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_red_scaleFID);
    *green_scale = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_green_scaleFID);
    *blue_scale  = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_blue_scaleFID);
    *alpha_scale = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_alpha_scaleFID);
}

jint *getICMParams(JNIEnv *env, jobject obj, int *length) {
    jobject d;

    d = (*env)->GetObjectField(env, obj, ImageCachedIDs.IndexColorModel_rgbFID);
    *length   = (*env)->GetIntField(env, obj, ImageCachedIDs.IndexColorModel_map_sizeFID);

    return (*env)->GetIntArrayElements(env, d, 0);
}

void freeICMParams(JNIEnv *env, jobject obj, jint *data) {
    jobject d = (*env)->GetObjectField(env, obj, ImageCachedIDs.IndexColorModel_rgbFID);

    (*env)->ReleaseIntArrayElements(env, d, data, JNI_ABORT);
}


/*
typedef struct {
   int Depth;
   XPixmapFormatValues wsImageFormat;
   ImgColorData clrdata;
   ImgConvertFcn *convert[NUM_IMGCV];
} awtImageData;
*/
static awtImageData _img_ = {
    0, {0, }, {0, }, {NULL, }, 
};
static awtImageData *awtImage = &_img_;

static void
awt_fill_imgcv(ImgConvertFcn **array, int mask, int value, ImgConvertFcn fcn)
{
    int i;

    for (i = 0; i < NUM_IMGCV; i++) {
	if ((i & mask) == value) {
	    array[i] = fcn;
	}
    }
}

extern ImgConvertFcn DirectImageConvert;
extern ImgConvertFcn PseudoImageConvert;
extern ImgConvertFcn PseudoFSImageConvert;

extern sgn_ordered_dither_array img_oda_red;
extern sgn_ordered_dither_array img_oda_green;
extern sgn_ordered_dither_array img_oda_blue;
extern uns_ordered_dither_array img_oda_alpha;

int setupImageDecode()
{
    int i, j, k, depth, bpp;
    XPixmapFormatValues *pPFV;
    int numpfv;
    char *forcemono;
    char *forcegray;
    int awt_num_colors = 0;
    XVisualInfo *pVI;
    XVisualInfo template;

    make_uns_ordered_dither_array(img_oda_alpha, 256);

    forcemono = getenv("FORCEMONO");
    forcegray = getenv("FORCEGRAY");
    if (forcemono && !forcegray)
	forcegray = forcemono;

    /*
     * Get the colormap and make sure we have the right visual
     */
    /* assert(NATIVE_LOCK_HELD()); */
    depth  = awt_depth;
    template.visualid = XVisualIDFromVisual(awt_visual);
    pVI = XGetVisualInfo(awt_display, VisualIDMask, &template, &i);

    if ((numColors == 256) || (grayscale && (numColors == 16))) {
	make_sgn_ordered_dither_array(img_oda_red,   -32, 32);
	make_sgn_ordered_dither_array(img_oda_green, -32, 32);
	make_sgn_ordered_dither_array(img_oda_blue,  -32, 32);
    } else if (grayscale) {
	make_sgn_ordered_dither_array(img_oda_red,    -85, 85);
	make_sgn_ordered_dither_array(img_oda_green,  -85, 85);
	make_sgn_ordered_dither_array(img_oda_blue,   -85, 85);
    } else {
	make_sgn_ordered_dither_array(img_oda_red,    -128, 128);
	make_sgn_ordered_dither_array(img_oda_green,  -128, 128);
	make_sgn_ordered_dither_array(img_oda_blue,   -128, 128);
    }

    /*
     * Flip green horizontally and blue vertically so that
     * the errors don't line up in the 3 primary components.
     */
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 4; j++) {
	    k = img_oda_green[i][j];
	    img_oda_green[i][j] = img_oda_green[i][7 - j];
	    img_oda_green[i][7 - j] = k;
	    k = img_oda_blue[j][i];
	    img_oda_blue[j][i] = img_oda_blue[7 - j][i];
	    img_oda_blue[7 - j][i] = k;
	}
    }

    pPFV = XListPixmapFormats(awt_display, &numpfv);
    if (pPFV) {
	for (i = 0; i < numpfv; i++) {
	    if (pPFV[i].depth == depth) {
		awtImage->wsImageFormat = pPFV[i];
		break;
	    }
	}
	XFree(pPFV);
    }
    bpp = awtImage->wsImageFormat.bits_per_pixel;
    if (bpp == 24) {
	bpp = 32;
    }
    awtImage->clrdata.bitsperpixel = bpp;
    awtImage->Depth = depth;

    if ((bpp == 32 || bpp == 16) && pVI->class == TrueColor) {
	awtImage->clrdata.rOff = 0;
	for (i = pVI->red_mask; (i & 1) == 0; i >>= 1) {
	    awtImage->clrdata.rOff++;
	}
	awtImage->clrdata.rScale = 0;
	while (i < 0x80) {
	    awtImage->clrdata.rScale++;
	    i <<= 1;
	}
	awtImage->clrdata.gOff = 0;
	for (i = pVI->green_mask; (i & 1) == 0; i >>= 1) {
	    awtImage->clrdata.gOff++;
	}
	awtImage->clrdata.gScale = 0;
	while (i < 0x80) {
	    awtImage->clrdata.gScale++;
	    i <<= 1;
	}
	awtImage->clrdata.bOff = 0;
	for (i = pVI->blue_mask; (i & 1) == 0; i >>= 1) {
	    awtImage->clrdata.bOff++;
	}
	awtImage->clrdata.bScale = 0;
	while (i < 0x80) {
	    awtImage->clrdata.bScale++;
	    i <<= 1;
	}
	awt_fill_imgcv(awtImage->convert, 0, 0, DirectImageConvert);
    } else if (bpp <= 8 && (pVI->class == StaticGray
			    || pVI->class == GrayScale
			    || forcegray)) {
	awtImage->clrdata.grayscale = 1;
	awtImage->clrdata.bitsperpixel = 8;

	awt_fill_imgcv(awtImage->convert, 0, 0, PseudoImageConvert);
	if (getenv("NOFSDITHER") == 0) {
	    awt_fill_imgcv(awtImage->convert,
			   IMGCV_ORDERBITS, IMGCV_TDLRORDER,
			   PseudoFSImageConvert);
	}
    } else if (bpp <= 8 && (pVI->class == PseudoColor
			    || pVI->class == TrueColor /* BugTraq ID 4061285 */
                            || pVI->class == StaticColor)) {
	if (pVI->class == TrueColor) {
	    awt_num_colors = (1 << pVI->depth);	/* BugTraq ID 4061285 */
	}
	awtImage->clrdata.bitsperpixel = 8;

	awt_fill_imgcv(awtImage->convert, 0, 0, PseudoImageConvert);
	if (getenv("NOFSDITHER") == 0) {

	    awt_fill_imgcv(awtImage->convert, IMGCV_ORDERBITS, IMGCV_TDLRORDER,
			   PseudoFSImageConvert);
	}
    } else {
	return 0;
    }

    if (depth > 8) {
	return 1;
    }

    if (awt_num_colors > 256) {
	return 0;
    }

    return 1;
}
