/*
 * @(#)img_globals.c	1.14 06/10/04
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
 * This file implements some of the standard utility procedures used
 * by the image conversion package.
 */


#include "img_globals.h"
#include <jni.h>
#include <stdlib.h>
#include "java_awt_image_ColorModel.h"


extern struct ImageCachedIDs ImageCachedIDs;
extern void initColorModels(JNIEnv *env);

/* JNI calls are simpler in C++ because you can overload -> on JNIenv */

/*
 * This function constructs an 8x8 ordered dither array which can be
 * used to dither data into an output range with discreet values that
 * differ by the value specified as quantum.  A monochrome screen would
 * use a dither array constructed with the quantum 256.
 * The array values produced are unsigned and intended to be used with
 * a lookup table which returns the next color darker than the error
 * adjusted color used as the index.
 */
void
make_uns_ordered_dither_array(uns_ordered_dither_array oda, int quantum)
{
    int i, j, k;

#if ((DITHER_ARRAY_HEIGHT != 8) || (DITHER_ARRAY_WIDTH != 8))
#error "Code only handles 8x8 dither arrays"
#endif

    oda[0][0] = 0;
    for (k = 1; k < 8; k *= 2) {
	for (i = 0; i < k; i++) {
	    for (j = 0; j < k; j++) {
		oda[ i ][ j ] = oda[i][j] * 4;
		oda[i+k][j+k] = oda[i][j] + 1;
		oda[ i ][j+k] = oda[i][j] + 2;
		oda[i+k][ j ] = oda[i][j] + 3;
	    }
	}
    }
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    oda[i][j] = oda[i][j] * quantum / 64;
	}
    }
}

/*
 * This function constructs an 8x8 ordered dither array which can be
 * used to dither data into an output range with discreet values that
 * are distributed over the range from minerr to maxerr around a given
 * target color value.
 * The array values produced are signed and intended to be used with
 * a lookup table which returns the closest color to the error adjusted
 * color used as an index.
 */
void
make_sgn_ordered_dither_array(sgn_ordered_dither_array oda, int minerr, int maxerr)
{
    int i, j, k;

#if ((DITHER_ARRAY_HEIGHT != 8) || (DITHER_ARRAY_WIDTH != 8))
#error "Code only handles 8x8 dither arrays"
#endif

    oda[0][0] = 0;
    for (k = 1; k < 8; k *= 2) {
	for (i = 0; i < k; i++) {
	    for (j = 0; j < k; j++) {
		oda[ i ][ j ] = oda[i][j] * 4;
		oda[i+k][j+k] = oda[i][j] + 1;
		oda[ i ][j+k] = oda[i][j] + 2;
		oda[i+k][ j ] = oda[i][j] + 3;
	    }
	}
    }
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    oda[i][j] = oda[i][j] * (maxerr - minerr) / 64 + minerr;
	}
    }
}

/*
 * This function deletes the information which is attached to the
 * private data member of a ColorModel object.  It implements the
 * native method defined in the java.awt.image.ColorModel class
 * which is used for finalization.
 */
JNIEXPORT void JNICALL
Java_java_awt_image_ColorModel_deletepData(JNIEnv *env, jobject obj)
{
    ImgCMData *icmd;

    if (ImageCachedIDs.ColorModel == NULL) {
	initColorModels(env);
    }

    icmd = (ImgCMData *)(*env)->GetIntField(env, obj, ImageCachedIDs.ColorModel_pDataFID);

    if (icmd != 0) {
	free(icmd);

        (*env)->SetIntField(env, obj, ImageCachedIDs.ColorModel_pDataFID, (jint)0);
    }
}

/*
 * This function calculates the ColorModel characterization information
 * and attaches it to the private data member of a ColorModel object for
 * future reference.
 */
ImgCMData *
img_getCMData(JNIEnv *env, jobject obj) 
{
    ImgCMData *icmd;

    if (ImageCachedIDs.ColorModel == NULL) {
	initColorModels(env);
	if (ImageCachedIDs.ColorModel == NULL) {
	    return NULL;
	}
    }

    icmd = (ImgCMData *)(*env)->GetIntField(env, obj, ImageCachedIDs.ColorModel_pDataFID);

    if (icmd == 0) {
	int type;


	if((*env)->IsInstanceOf(env, obj, ImageCachedIDs.IndexColorModel)) {
	    jboolean opaque = (*env)->GetBooleanField(env, obj, ImageCachedIDs.IndexColorModel_opaqueFID);
	    type = IMGCV_ICM | (opaque ? IMGCV_OPAQUE : IMGCV_ALPHA);
	} else if ((*env)->IsInstanceOf(env, obj, ImageCachedIDs.DirectColorModel)) {
	    jint red_scale, green_scale, blue_scale, alpha_scale;
	    jint alpha_mask;

            red_scale = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_red_scaleFID);
            green_scale = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_green_scaleFID);
            blue_scale = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_blue_scaleFID);
            alpha_scale = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_alpha_scaleFID);
            alpha_mask = (*env)->GetIntField(env, obj, ImageCachedIDs.DirectColorModel_alpha_maskFID);

	    if (red_scale == 0 && green_scale == 0 &&
		blue_scale == 0 && (alpha_scale == 0 || alpha_mask == 0)) {
		type = IMGCV_DCM8;
	    } else {
		type = IMGCV_DCM;
	    }
	    type |= (alpha_mask ? IMGCV_ALPHA : IMGCV_OPAQUE);
	} else {
	    type = IMGCV_ANYCM | IMGCV_ALPHA;
	}
	icmd = (ImgCMData *)malloc(sizeof *icmd);
	if (icmd != 0) {
	    icmd->type   = type;
	    icmd->getRGB = ImageCachedIDs.ColorModel_getRGBMID;

	    (*env)->SetIntField(env, obj, ImageCachedIDs.ColorModel_pDataFID, (jint)icmd);
	}
    }

    return icmd;
}

#ifdef TESTING
#include <stdio.h>

/* Function to test the ordered dither error matrix initialization function. */
main(int argc, char **argv)
{
    int i, j;
    int quantum;
    int max, val;
    uns_ordered_dither_array oda;

    if (argc > 1) {
	quantum = atoi(argv[1]);
    } else {
	quantum = 64;
    }
    make_uns_ordered_dither_array(oda, quantum);
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    val = oda[i][j];
	    printf("%4d", val);
	    if (max < val) {
		max = val;
	    }
	}
	printf("\n");
    }
    printf("\nmax = %d\n", max);
}
#endif /* TESTING */





