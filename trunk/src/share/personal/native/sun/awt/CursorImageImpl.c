/*
 * @(#)CursorImageImpl.c	1.20 06/10/10
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

#include "sun_awt_gfX_CursorImageImpl.h"
#include "Xheaders.h"
#include "common.h"
#include "img_util.h"
#include "img_util_md.h"
#include "X11/cursorfont.h"

/* Taken from src/share/java/awt/Cursor.java */
#define	DEFAULT_CURSOR 0
#define	CROSSHAIR_CURSOR 1
#define	TEXT_CURSOR 2
#define	WAIT_CURSOR 3
#define	SW_RESIZE_CURSOR 4
#define	SE_RESIZE_CURSOR 5
#define	NW_RESIZE_CURSOR 6
#define	NE_RESIZE_CURSOR 7
#define	N_RESIZE_CURSOR 8
#define	S_RESIZE_CURSOR 9
#define	W_RESIZE_CURSOR 10
#define	E_RESIZE_CURSOR 11
#define	HAND_CURSOR 12
#define	MOVE_CURSOR 13

JNIACCESS(cursorImage);
JNIACCESS(sun_awt_image_Image);

JNIEXPORT void JNICALL 
Java_sun_awt_gfX_CursorImageImpl_initJNI(JNIEnv *env, jobject cls)
{
    DEFINECLASS(cursorImage, cls);
    DEFINEFIELD(cursorImage, hotX,  "I");
    DEFINEFIELD(cursorImage, hotY,  "I");
    DEFINEFIELD(cursorImage, pData, "I");
    DEFINEFIELD(cursorImage, image, "Ljava/awt/Image;");

    if (MUSTLOAD(sun_awt_image_Image)) {
	FINDCLASS(sun_awt_image_Image, "sun/awt/image/Image");
	DEFINEMETHOD(sun_awt_image_Image, getImageRep,
		     "()Lsun/awt/image/ImageRepresentation;");
    }
}

/*
 * Class:     sun_awt_gfX_CursorImageImpl
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_gfX_CursorImageImpl_finalize(JNIEnv * env, jobject obj)
{
    Cursor pData;

    pData = (Cursor) GETFIELD(Int, obj, cursorImage, pData);
    if (pData != 0) {
	NATIVE_LOCK(env);
	XFreeCursor(display, pData);
	SETFIELD(Int, obj, cursorImage, pData, 0);
	NATIVE_UNLOCK(env);
    }
}

JNIEXPORT void JNICALL 
Java_sun_awt_gfX_CursorImageImpl_makeNativeCursor(JNIEnv *env, jobject obj, jint ctype)
{
    Cursor xcursor = None;
    int cursorType;

    switch (ctype) {
      case DEFAULT_CURSOR:
	cursorType = XC_arrow;
	break;

      case CROSSHAIR_CURSOR:
	cursorType = XC_crosshair;
	break;

      case TEXT_CURSOR:
	cursorType = XC_xterm;
	break;

      case WAIT_CURSOR:
	cursorType = XC_watch;
	break;

      case SW_RESIZE_CURSOR:
	cursorType = XC_bottom_left_corner;
	break;

      case NW_RESIZE_CURSOR:
	cursorType = XC_top_left_corner;
	break;

      case SE_RESIZE_CURSOR:
	cursorType = XC_bottom_right_corner;
	break;

      case NE_RESIZE_CURSOR:
	cursorType = XC_top_right_corner;
	break;

      case S_RESIZE_CURSOR:
	cursorType = XC_bottom_side;
	break;

      case N_RESIZE_CURSOR:
	cursorType = XC_top_side;
	break;

      case W_RESIZE_CURSOR:
	cursorType = XC_left_side;
	break;

      case E_RESIZE_CURSOR:
	cursorType = XC_right_side;
	break;

      case HAND_CURSOR:
	cursorType = XC_hand2;
	break;

      case MOVE_CURSOR:
	cursorType = XC_fleur;
	break;

      default:
	return;
    }
    
    NATIVE_LOCK(env);
    xcursor = XCreateFontCursor(display, cursorType);
    NATIVE_UNLOCK(env);

    SETFIELD(Int, obj, cursorImage, pData, (int)xcursor);
}


Cursor
cursorImage_getCursor(JNIEnv *env, jobject obj) {
    int pData;

    jobject img, imgRep;
    IRData *ird;
    int hot_x, hot_y;
    Pixmap dataBits;
    XColor fg, bg;
    Cursor c;

    if ((pData = GETFIELD(Int, obj, cursorImage, pData)) != 0) {
	return (Cursor) pData;
    }

    hot_x = GETFIELD(Int, obj, cursorImage, hotX);
    hot_y = GETFIELD(Int, obj, cursorImage, hotY);
    img   = GETFIELD(Object, obj, cursorImage, image);

    if (img == NULL) {
	return None;
    }

    imgRep = CALLMETHOD(Object, img, sun_awt_image_Image, getImageRep);
    ird = image_getIRData(env, imgRep, NULL, True);

    if (ird == 0) {
	OutOfMemoryError(env, "No memory for CursorImage");
	return None;
    }

    if (ird->pixmap == None) {
	return None;
    }

    /* 
     * create a bitmap from the given pixmap.  Pixmap is only allowed
     * two (non-transparent) colors.
     */
    NATIVE_LOCK(env);
    {
	int x, y;
	char fg_set = 0, bg_set = 0;
	XImage *maskImg, *srcImg;

#define TRANSPIXEL(x, y) ((maskImg == NULL) ? 0 : XGetPixel(maskImg, x, y))

/* fprintf(stderr, "XGetImage %s %d\n", __FILE__, __LINE__); fflush(stderr); */
	maskImg = (ird->mask == 0) ? NULL :
	    XGetImage(display, ird->mask, 0, 0, 
		      (unsigned)ird->dstW, (unsigned)ird->dstH, 
		      AllPlanes, XYPixmap);

/* fprintf(stderr, "XGetImage %s %d\n", __FILE__, __LINE__); fflush(stderr); */
	srcImg  = XGetImage(display, ird->pixmap, 0, 0, 
			    (unsigned)ird->dstW, (unsigned)ird->dstH, 
			    AllPlanes, XYPixmap);

	fg.pixel = 0xffffffff;
	bg.pixel = 0;

	if (srcImg->depth == 1) {
	    dataBits = ird->pixmap;
	} else {
	    int scansize;
	    XImage *newImg  = NULL;
	    unsigned char *data;

	    scansize = paddedwidth(ird->dstH, 32) >> 3;
	    data = sysCalloc(1, (unsigned)(ird->dstH * scansize + 1));
	    if (data == NULL) {
		fprintf(stderr, "calloc failed at line %d file %s\n",
			__LINE__, __FILE__);
	    }

	    newImg = XCreateImage(display, root_visual, 1, XYBitmap,
				  0, (char *)data, 
				  (unsigned)ird->dstW, (unsigned)ird->dstH, 
				  32, scansize);
	    if (newImg == NULL) {
		fprintf(stderr, "XCreateImage failed at line %d file %s\n",
			__LINE__, __FILE__);
	    }

	    for (y = 0; y < ird->dstH; ++y) {
		for (x = 0; x < ird->dstW; ++x) {
		    int pix;
		    if (TRANSPIXEL(x, y)) {
			continue;
		    }
		    pix = XGetPixel(srcImg, x, y);
		    if (!bg_set) {
			bg_set = True;
			bg.pixel = pix;
		    } else if (pix != bg.pixel) {
			XPutPixel(srcImg, x, y, 1);
			if (!fg_set) {
			    fg_set = True;
			    fg.pixel = pix;
			}
			if (pix != fg.pixel) {
			    /* NOTE: error case! */
			}
		    }

		}
	    }

	    dataBits = XCreatePixmapFromBitmapData(display, root_drawable,
						   (char *)data, 
						   (unsigned)ird->dstW, 
						   (unsigned)ird->dstH, 
						   1, 0, 1);
	    XDestroyImage(newImg);
	}

	if (maskImg != NULL) {
	    XDestroyImage(maskImg);
	}
	XDestroyImage(srcImg);
    }
 
    /* finally, create the cursor */
    c = XCreatePixmapCursor(display, dataBits, ird->mask, &fg, &bg,
			    (unsigned)hot_x, (unsigned)hot_y);

    if (dataBits != ird->pixmap) {
	XFreePixmap(display, dataBits);
    }

    NATIVE_UNLOCK(env);

    SETFIELD(Int, obj, cursorImage, pData, (int)c);

    return c;
}
