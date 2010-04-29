/*
 * @(#)img_util_md.h	1.22 06/10/16
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

#ifndef _IMG_UTIL_MD_H_
#define _IMG_UTIL_MD_H_

#include <Xheaders.h>
#include <common.h>

typedef struct aID {
    int Depth;
    XPixmapFormatValues wsImageFormat;
    ImgColorData clrdata;
    ImgConvertFcn *convert[NUM_IMGCV];
} awtImageData;

extern awtImageData *awtImage;

typedef struct {
    ImgConvertData cvdata;	/* The data needed by ImgConvertFcn's */
    XID pixmap;			/* The X11 pixmap containing the image */
    XID mask;			/* The X11 pixmap with the transparency mask */
    int bgcolor;		/* The current bg color installed in pixmap */

    int depth;			/* The depth of the destination image */
    int dstW;			/* The width of the destination pixmap */
    int dstH;			/* The height of the destination pixmap */

    XImage *xim;		/* The Ximage structure for the temp buffer */
    XImage *maskim;		/* The Ximage structure for the mask */

    int hints;			/* The delivery hints from the producer */

    Region curpixels;		/* The region of randomly converted pixels */
    struct {
	int num;		/* The last fully delivered scanline */
	char *seen;		/* The lines which have been delivered */
    } curlines;			/* For hints=COMPLETESCANLINES */

  jobject hJavaObject;

} IRData;

extern int getRGBFromPixel(JNIEnv *env, jobject obj, unsigned int pixel);
extern jint *getICMParams(JNIEnv *env, jobject obj, int *length);
extern void freeICMParams(JNIEnv *env, jobject obj, jint *data);
extern void getDCMParams(JNIEnv *env, jobject obj,
			 int *red_mask, int *red_off, int *red_scale,
			 int *green_mask, int *green_off, int *green_scale,
			 int *blue_mask, int *blue_off, int *blue_scale,
			 int *alpha_mask, int *alpha_off, int *alpha_scale);
extern ImgCMData *img_getCMData(JNIEnv *env, jobject obj);

extern IRData *image_getIRData(JNIEnv *env, jobject irh, jobject ch);

typedef unsigned int MaskBits;

extern int image_Done(IRData *ird, int x1, int y1, int x2, int y2);

extern void *image_InitMask(IRData *ird, 
			    int x1, int y1, int x2, int y2);

#define BufComplete(cvdata, dstX1, dstY1, dstX2, dstY2)		\
    image_Done((IRData *) cvdata, dstX1, dstY1, dstX2, dstY2)

#define SendRow(ird, dstY, dstX1, dstX2)

#define ImgInitMask(cvdata, x1, y1, x2, y2)			  \
    image_InitMask((IRData *)cvdata, x1, y1, x2, y2)

#define ScanBytes(cvdata)	(((IRData *)cvdata)->xim->bytes_per_line)

#define MaskScan(cvdata)					\
	((((IRData *)cvdata)->maskim->bytes_per_line) >> 2)

#define MaskOffset(x)		((x) >> 5)

#define MaskInit(x)		(1U << (31 - ((x) & 31)))

#define SetOpaqueBit(mask, bit)		(mask |= bit)
#define SetTransparentBit(mask, bit)	(mask &= ~bit)

#define ColorCubeFSMap(r, g, b) \
    RGB2Pixel(r, g, b)

#define ColorCubeOrdMapSgn(r, g, b) \
    RGB2Pixel(r, g, b)

#define GetPixelRGB(pixel, red, green, blue)    \
    pixel2RGB(pixel, &red, &green, &blue)


#endif  /* _IMG_UTIL_MD_H_ */




