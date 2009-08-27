/*
 * @(#)image.c	1.99 06/10/10
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

#define MITSHM

#include <assert.h>

#include "jni.h"
#include "image.h"
#include "color.h"
#include "img_globals.h"
#include "img_util.h"
#include "img_util_md.h"
#include "java_awt_image_ImageConsumer.h"
#include "java_awt_image_ImageObserver.h"
#include "java_awt_image_ColorModel.h"
#include "java_awt_image_IndexColorModel.h"
#include "java_awt_Color.h"
#include "java_awt_Graphics.h"
#include "java_awt_Rectangle.h"
#include "sun_awt_image_Image.h"
#include "sun_awt_image_OffScreenImageSource.h"

#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif				/* MITSHM */

#define ICMCLASS	"java/awt/image/IndexColorModel"

#define IMAGE_SIZEINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_DRAWINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_SOMEBITS |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_FULLDRAWINFO (java_awt_image_ImageObserver_FRAMEBITS |	\
			    java_awt_image_ImageObserver_ALLBITS)

#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)

#define HINTS_DITHERFLAGS (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT)
#define HINTS_SCANLINEFLAGS (java_awt_image_ImageConsumer_COMPLETESCANLINES)

#define HINTS_OFFSCREENSEND (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |  \
			     java_awt_image_ImageConsumer_COMPLETESCANLINES | \
			     java_awt_image_ImageConsumer_SINGLEPASS)

Pixel           awt_white;

JNIEXPORT void JNICALL
Java_sun_awt_image_ImageRepresentation_initIDs (JNIEnv *env, jclass cls)
{

}

static void
PutAndReformatImage(Display * dsp, Drawable win, GC gc, XImage * img,
		    int sx, int sy, int dx, int dy, int w, int h);


GC
awt_getImageGC(Pixmap pixmap)
{
	static GC       awt_imagegc;

	if (!awt_imagegc) {
		awt_white = AwtColorMatch(255, 255, 255);
		awt_imagegc = XCreateGC(awt_display, pixmap, 0, NULL);
		XSetForeground(awt_display, awt_imagegc, awt_white);
	}
	return awt_imagegc;
}

IRData *
image_getIRData(JNIEnv * env, jobject irh, jobject ch)
{
	IRData         *ird;
	GC              imagegc;
	jint            width, height;


	ird = (IRData *) (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);

	if (ird == NULL) {
		int             availinfo = (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

		if ((availinfo & IMAGE_SIZEINFO) != IMAGE_SIZEINFO) {
			return ird;
		}
		
		if((ird = (IRData *) XtMalloc(sizeof(IRData))) == NULL) {
			return ird;
		}
		memset(ird, '\0', sizeof(IRData));

		if((ird->hJavaObject = (*env)->NewGlobalRef(env, irh)) == NULL) {
		  XtFree((char *)ird);
		  return NULL;
		}

		width = (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
		height = (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_heightFID);


		XLockDisplay(awt_display);

		ird->pixmap = XCreatePixmap(awt_display,
					    awt_root,
					    width,
					    height,
					    awtImage->Depth);

		imagegc = awt_getImageGC(ird->pixmap);
		if (ch != 0) {
			XSetForeground(awt_display, imagegc, awt_getColor(env, ch));
		}
		XFillRectangle(awt_display, ird->pixmap, imagegc, 0, 0, width, height);
		if (ch != 0) {
			XSetForeground(awt_display, imagegc, awt_white);
		}

		XUnlockDisplay(awt_display);


		ird->depth = awtImage->clrdata.bitsperpixel;
		ird->dstW = width;
		ird->dstH = height;
		ird->hints = (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_hintsFID);

		(*env)->SetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_pDataFID, (jint) ird);
	} else if (ird->hints == 0) {
		ird->hints = (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_hintsFID);
	}
	return ird;
}

Drawable
image_getIRDrawable(JNIEnv * env, jobject ir)
{
	IRData         *ird;
	if (ir == NULL) {
		return 0;
	}
	ird = image_getIRData(env, ir, NULL);
	return (ird == NULL) ? 0 : ird->pixmap;
}

static void
image_FreeBufs(IRData * ird)
{
	if (ird->cvdata.outbuf) {
		XtFree(ird->cvdata.outbuf);
		ird->cvdata.outbuf = 0;
	}
	if (ird->xim) {
		XtFree((char *)(ird->xim));
		ird->xim = 0;
	}
	if (ird->cvdata.maskbuf) {
		XtFree((char *)(ird->cvdata.maskbuf));
		ird->cvdata.maskbuf = 0;
	}
	if (ird->maskim) {
		XtFree((char *)(ird->maskim));
		ird->maskim = 0;
	}
}

static void
image_FreeRenderData(IRData * ird)
{
	if (ird->cvdata.fserrors) {
		XtFree(ird->cvdata.fserrors);
		ird->cvdata.fserrors = 0;
	}
	if (ird->curpixels) {
		XDestroyRegion(ird->curpixels);
		ird->curpixels = 0;
	}
	if (ird->curlines.seen) {
		XtFree(ird->curlines.seen);
		ird->curlines.seen = 0;
	}
}

void *
image_InitMask(IRData * ird, int x1, int y1, int x2, int y2)
{
	void           *mask;
	int             scansize, bufsize;
	Region          pixrgn;

	scansize = paddedwidth(ird->dstW, 32) >> 3;
	bufsize = scansize * ird->dstH + 1;
	if ((bufsize < 0) || ((bufsize - 1) / scansize != ird->dstH)) {
		/* calculation must have overflown */
		ird->cvdata.maskbuf = 0;
		return (void *) NULL;
	}
	mask = (void *) XtMalloc(bufsize);
	ird->cvdata.maskbuf = mask;
	if (mask != 0) {

	  XLockDisplay(awt_display);

		ird->maskim = XCreateImage(awt_display, awt_visual, 1,
					   XYBitmap, 0, ird->cvdata.maskbuf,
					   ird->dstW, ird->dstH,
					   32, scansize);
		if (ird->maskim == 0) {
			XtFree(ird->cvdata.maskbuf);
			ird->cvdata.maskbuf = 0;
		} else {
			static int      native_order = (MSBFirst << 24) | LSBFirst;
			ird->maskim->byte_order = ((char *) &native_order)[0];
			ird->maskim->bitmap_bit_order = MSBFirst;
			ird->maskim->bitmap_unit = 32;
		}

		ird->mask = XCreatePixmap(awt_display, awt_root,
					  ird->dstW, ird->dstH, 1);
		XSetForeground(awt_display, awt_maskgc, 0);
		XFillRectangle(awt_display, ird->mask, awt_maskgc,
			       0, 0, ird->dstW, ird->dstH);
		XSetForeground(awt_display, awt_maskgc, 1);
		XFillRectangle(awt_display, ird->mask, awt_maskgc,
			       x1, y1, x2 - x1, y2 - y1);
		if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
			XFillRectangle(awt_display, ird->mask, awt_maskgc,
				       0, 0, ird->dstW, ird->curlines.num);
		} else {
			pixrgn = ird->curpixels;
			if (pixrgn != 0) {
				XSetRegion(awt_display, awt_maskgc, pixrgn);
				XFillRectangle(awt_display, ird->mask, awt_maskgc,
					       0, 0, ird->dstW, ird->dstH);
				XSetClipMask(awt_display, awt_maskgc, None);
				XDestroyRegion(pixrgn);
				ird->curpixels = 0;
			}
		}
		XGetSubImage(awt_display, ird->mask,
			     0, 0, ird->dstW, ird->dstH,
			     ~0, ZPixmap, ird->maskim, 0, 0);


		XUnlockDisplay(awt_display);
	}
	return mask;
}

static int
image_BufAlloc(IRData * ird)
{
	int             w = ird->dstW;
	int             h = ird->dstH;
	int             bpp, slp, bpsl;
	int             bufsize;
	int             mask = ird->mask;

	if (w >= 0 && h >= 0) {
		image_FreeBufs(ird);
		bpp = awtImage->clrdata.bitsperpixel;
		slp = awtImage->wsImageFormat.scanline_pad;
		bpsl = paddedwidth(w * bpp, slp) >> 3;
		bufsize = bpsl * h;
		assert(bpp >= 8);
		if (((bpsl / (bpp >> 3)) < w) || (bufsize / bpsl < h)) {
			/* calculations must have overflown */
			return 0;
		}
		ird->cvdata.outbuf = (void *) XtMalloc(bufsize);
		if (ird->cvdata.outbuf != 0) {
			ird->xim = XCreateImage(awt_display,
						awt_visual,
						awtImage->Depth, ZPixmap, 0,
					        ird->cvdata.outbuf,
						w, h, slp, bpsl);
			if (ird->xim != 0 && mask) {
				image_InitMask(ird, 0, 0, 0, 0);
			}
		}
		if (ird->cvdata.outbuf == 0 || ird->xim == 0
		    || ((mask != 0) && ((ird->cvdata.maskbuf == 0)
					|| (ird->maskim == 0)))) {
			image_FreeBufs(ird);
			return 0;
		}
		ird->xim->bits_per_pixel = bpp;
	}
	return 1;
}

int
image_Done(IRData * ird, int x1, int y1, int x2, int y2)
{
	int             bpp;
	int             w = x2 - x1;
	int             h = y2 - y1;
	int             ytop = y1;
	GC              imagegc;

	JNIEnv         *env = NULL;
	jobject         newbits;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return 0;

	if (ird->pixmap == None || ird->xim == 0) {
		return 0;
	}
	bpp = awtImage->wsImageFormat.bits_per_pixel;
	imagegc = awt_getImageGC(ird->pixmap);


	XLockDisplay(awt_display);

	if (ird->xim->bits_per_pixel == bpp) {
		XPutImage(awt_display, ird->pixmap, imagegc,
			  ird->xim, x1, y1, x1, y1, w, h);
	} else {
		PutAndReformatImage(awt_display, ird->pixmap, imagegc,
				    ird->xim, x1, y1, x1, y1, w, h);
	}
	if (ird->mask) {
		XPutImage(awt_display, ird->mask, awt_maskgc,
			  ird->maskim, x1, y1, x1, y1, w, h);
		/* We just invalidated any background color preparation... */
		ird->bgcolor = 0;
	}
	if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
		char           *seen = ird->curlines.seen;
		int             l;
		if (seen == 0) {
			seen = XtMalloc(ird->dstH);
			memset(seen, 0, ird->dstH);
			ird->curlines.seen = seen;
		}
		for (l = y1 - 1; l >= 0; l--) {
			if (seen[l]) {
				break;
			}
			memcpy(ird->xim->data + y1 * ird->xim->bytes_per_line,
			       ird->xim->data + l * ird->xim->bytes_per_line,
			       ird->xim->bytes_per_line);
			XCopyArea(awt_display, ird->pixmap, ird->pixmap, imagegc,
				  x1, y1, w, 1, x1, l);
			if (ird->mask) {
				memcpy(ird->maskim->data + y1 * ird->maskim->bytes_per_line,
				       ird->maskim->data + l * ird->maskim->bytes_per_line,
				       ird->maskim->bytes_per_line);
				XCopyArea(awt_display, ird->mask, ird->mask, awt_maskgc,
					  x1, y1, w, 1, x1, l);
			}
			ytop = l;
		}
		for (l = y1; l < y2; l++) {
			seen[l] = 1;
		}
	} else if (ird->mask == 0) {
		XRectangle      rect;
		Region          curpixels = ird->curpixels;

		rect.x = x1;
		rect.y = y1;
		rect.width = w;
		rect.height = h;

		if (curpixels == 0) {
			curpixels = XCreateRegion();
			ird->curpixels = curpixels;
		}
		XUnionRectWithRegion(&rect, curpixels, curpixels);
	}


	XUnlockDisplay(awt_display);

	if (ird->curlines.num < y2) {
		ird->curlines.num = y2;
	}
	newbits = (*env)->GetObjectField(env, ird->hJavaObject, MCachedIDs.sun_awt_image_ImageRepresentation_newbitsFID);

	if (newbits != 0) {

		jobject         pNB = (*env)->GetObjectField(env, ird->hJavaObject, MCachedIDs.sun_awt_image_ImageRepresentation_newbitsFID);

		(*env)->SetIntField(env, pNB, MCachedIDs.java_awt_Rectangle_xFID, (jint) x1);
		(*env)->SetIntField(env, pNB, MCachedIDs.java_awt_Rectangle_yFID, (jint) ytop);
		(*env)->SetIntField(env, pNB, MCachedIDs.java_awt_Rectangle_widthFID, (jint) w);
		(*env)->SetIntField(env, pNB, MCachedIDs.java_awt_Rectangle_heightFID, (jint) (y2 - ytop));
	}
	return 1;
}

JNIEXPORT void  JNICALL
Java_sun_awt_image_ImageRepresentation_offscreenInit(JNIEnv * env, jobject this, jobject ch)
{
	IRData         *ird;

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	if ((*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_widthFID) <= 0 ||
	    (*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_heightFID) <= 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID, (jint) IMAGE_OFFSCRNINFO);

	ird = image_getIRData(env, this, ch);
	if (ird == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	AWT_UNLOCK();
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_image_ImageRepresentation_setBytePixels(JNIEnv * env, jobject this,
						     jint x, jint y,
						     jint w, jint h,
					       jobject cmh, jbyteArray arrh,
						     jint off, jint scansize)
{
	IRData         *ird;
	jbyte          *arrh_p;
	int             ret, flags;
	ImgCMData      *icmd;
	ImgConvertFcn  *cvfcn;

	if (this == 0 || cmh == 0 || arrh == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return -1;
	}

	if (x < 0 || y < 0 || w < 0 || h < 0
	    || scansize < 0 || off < 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_ArrayIndexOutOfBoundsExceptionClass, NULL);
		return -1;
	}
	if (w == 0 || h == 0) {
		return 0;
	}
	if (((*env)->GetArrayLength(env, arrh) < (unsigned long) (off + w)) ||
	    ((scansize != 0) && ((((*env)->GetArrayLength(env, arrh) - w - off) / scansize) < (h - 1)))) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_ArrayIndexOutOfBoundsExceptionClass, NULL);
		return -1;
	}
	AWT_LOCK();

	ird = image_getIRData(env, this, NULL);
	if (ird == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return -1;
	}
	if (ird->cvdata.outbuf == 0) {
		image_BufAlloc(ird);
		if (ird->cvdata.outbuf == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
			AWT_UNLOCK();
			return -1;
		}
	}
	icmd = img_getCMData(env, cmh);
	if (icmd == 0) {
		AWT_UNLOCK();
		return -1;
	}
	flags = (IMGCV_UNSCALED)
		| (IMGCV_BYTEIN)
		| (((ird->hints & HINTS_DITHERFLAGS) != 0)
		   ? IMGCV_TDLRORDER : IMGCV_RANDORDER)
		| icmd->type;
	if (ird->cvdata.maskbuf) {
		flags |= IMGCV_ALPHA;
	}
	cvfcn = awtImage->convert[flags];

	arrh_p = (jbyte *) (*env)->GetByteArrayElements(env, arrh, NULL);
	if (arrh_p == NULL) {
		AWT_UNLOCK();
		return -1;
	}
	ret = cvfcn(env, this, cmh, x, y, w, h,
		    arrh_p, off, 8, scansize,
		    ird->dstW, ird->dstH, ird->dstW, ird->dstH,
		    &ird->cvdata, &awtImage->clrdata);

	(*env)->ReleaseByteArrayElements(env, arrh, arrh_p, JNI_ABORT);

	AWT_UNLOCK();
	return (ret == SCALESUCCESS);
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_image_ImageRepresentation_setIntPixels(JNIEnv * env, jobject this,
					     jint x, jint y, jint w, jint h,
						jobject cmh, jintArray arrh,
						    jint off, jint scansize)
{
	IRData         *ird;
	ImgCMData      *icmd;
	ImgConvertFcn  *cvfcn;
	jint           *arrh_p;
	int             ret, flags;

	if (this == 0 || cmh == 0 || arrh == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return -1;
	}

	if (x < 0 || y < 0 || w < 0 || h < 0
	    || scansize < 0 || off < 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_ArrayIndexOutOfBoundsExceptionClass, NULL);
		return -1;
	}
	if (w == 0 || h == 0) {
		return 0;
	}
	if (((*env)->GetArrayLength(env, arrh) < (unsigned long) (off + w)) ||
	    ((scansize != 0) && ((((*env)->GetArrayLength(env, arrh) - w - off) / scansize) < (h - 1)))) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_ArrayIndexOutOfBoundsExceptionClass, NULL);
		return -1;
	}
	AWT_LOCK();
	ird = image_getIRData(env, this, NULL);
	if (ird == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return -1;
	}
	if (ird->cvdata.outbuf == 0) {
		image_BufAlloc(ird);
		if (ird->cvdata.outbuf == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
			AWT_UNLOCK();
			return -1;
		}
	}
	icmd = img_getCMData(env, cmh);
	if (icmd == 0) {
		AWT_UNLOCK();
		return -1;
	}
	flags = (IMGCV_UNSCALED)
		| (IMGCV_INTIN)
		| (((ird->hints & HINTS_DITHERFLAGS) != 0)
		   ? IMGCV_TDLRORDER : IMGCV_RANDORDER)
		| icmd->type;
	if (ird->cvdata.maskbuf) {
		flags |= IMGCV_ALPHA;
	}
	cvfcn = awtImage->convert[flags];

	arrh_p = (*env)->GetIntArrayElements(env, arrh, NULL);
	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return -1;
	}
	ret = cvfcn(env, this, cmh, x, y, w, h,
		    arrh_p, off, 32, scansize,
		    ird->dstW, ird->dstH, ird->dstW, ird->dstH,
		    &ird->cvdata, &awtImage->clrdata);

	(*env)->ReleaseIntArrayElements(env, arrh, arrh_p, JNI_ABORT);

	AWT_UNLOCK();
	return (ret == SCALESUCCESS);
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_image_ImageRepresentation_finish(JNIEnv * env, jobject this, jboolean force)
{
	IRData         *ird;
	int             ret = 0;

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return 0;
	}
	AWT_LOCK();
	ird = image_getIRData(env, this, NULL);
	if (ird != NULL) {
		ret = (!force
		       && ird->depth <= 8
		       && (ird->hints & HINTS_DITHERFLAGS) == 0);
		if (ird->mask == 0 &&
		    (((ird->hints & HINTS_SCANLINEFLAGS) != 0)
		     ? (ird->curlines.num < ird->dstH)
		     : (ird->curpixels != 0))) {
			/*
			 * Now that all of the pixels are claimed to have
			 * been delivered, if we have any remaining
			 * non-delivered pixels, they are now considered
			 * transparent.  Thus we construct a mask.
			 */
			image_InitMask(ird, 0, 0, 0, 0);
		}
		image_FreeRenderData(ird);
		ird->hints = 0;
		ird->curlines.num = ird->dstH;
	}
	AWT_UNLOCK();
	return ret;
}

#define INIT_GC(env, disp, gdata)\
if (gdata==0 || gdata->gc == 0 && !awt_init_gc(env, disp,gdata)) {\
    AWT_UNLOCK();\
    return;\
}

int
awt_imageDraw(JNIEnv * env, jobject this, jobject c, Drawable win, GC gc,
	      int xormode, unsigned long xorpixel, unsigned long fgpixel,
	      long x, long y,
	      long bx, long by, long bw, long bh,
	      XRectangle * clip)
{
	IRData         *ird;
	jint            availinfo;
	long            wx;
	long            wy;
	long            ix;
	long            iy;
	long            iw;
	long            ih;
	long            diff;
	Region          pixrgn = 0;

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return 0;
	}
	availinfo = (*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);
	if ((availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
		return 1;
	}
	ird = image_getIRData(env, this, NULL);
	if (ird == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return 0;
	} else {
		if (ird->pixmap == None) {
			return 1;
		}
		if (win == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "win");
			return 0;
		}
		if ((availinfo & IMAGE_FULLDRAWINFO) != 0) {
			ix = iy = 0;
			iw = (*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
			ih = (*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_heightFID);
		} else if ((ird->hints & HINTS_SCANLINEFLAGS) != 0 || ird->mask) {
			ix = iy = 0;
			iw = (*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
			ih = ird->curlines.num;
		} else {
			XRectangle      rect;
			pixrgn = ird->curpixels;
			if (!pixrgn) {
				return 1;
			}
			XClipBox(pixrgn, &rect);
			ix = rect.x;
			iy = rect.y;
			iw = rect.width;
			ih = rect.height;
		}

		if (bw >= 0 && bh >= 0) {
			diff = bx - ix;
			if (diff > 0) {
				iw -= diff;
				ix = bx;
			}
			diff = by - iy;
			if (diff > 0) {
				ih -= diff;
				iy = by;
			}
			diff = bx + bw - ix;
			if (iw > diff) {
				iw = diff;
			}
			diff = by + bh - iy;
			if (ih > diff) {
				ih = diff;
			}
		}
		wx = x + ix - bx;
		wy = y + iy - by;

		if (clip) {
			diff = (clip->x - wx);
			if (diff > 0) {
				iw -= diff;
				ix += diff;
				wx = clip->x;
			}
			diff = (clip->y - wy);
			if (diff > 0) {
				ih -= diff;
				iy += diff;
				wy = clip->y;
			}
			diff = clip->x + clip->width - wx;
			if (iw > diff) {
				iw = diff;
			}
			diff = clip->y + clip->height - wy;
			if (ih > diff) {
				ih = diff;
			}
		}
		if (iw <= 0 || ih <= 0) {
			return 1;
		}


		XLockDisplay(awt_display);


		if (ird->mask) {
			if (c == 0) {
				XSetClipMask(awt_display, gc, ird->mask);
			} else {
				Pixel           bgcolor;
				if (xormode) {
					bgcolor = xorpixel;
				} else {
					bgcolor = awt_getColor(env, c);
				}
				if (ird->bgcolor != bgcolor + 1) {
					/*
					 * Set the "transparent pixels" in
					 * the pixmap to the specified solid
					 * background color and just do an
					 * unmasked copy into place below.
					 */
					GC              imagegc = awt_getImageGC(ird->pixmap);
					XSetFunction(awt_display, awt_maskgc, GXxor);
					XFillRectangle(awt_display, ird->mask, awt_maskgc,
						0, 0, ird->dstW, ird->dstH);
					XSetClipMask(awt_display, imagegc, ird->mask);
					XSetForeground(awt_display, imagegc, bgcolor);
					XFillRectangle(awt_display, ird->pixmap, imagegc,
						0, 0, ird->dstW, ird->dstH);
					XSetForeground(awt_display, imagegc, awt_white);
					XSetClipMask(awt_display, imagegc, None);
					XFillRectangle(awt_display, ird->mask, awt_maskgc,
						0, 0, ird->dstW, ird->dstH);
					XSetFunction(awt_display, awt_maskgc, GXcopy);
					ird->bgcolor = bgcolor + 1;
				}
			}
		} else if (pixrgn) {
			XSetRegion(awt_display, gc, pixrgn);
		}
		if ((ird->mask && c == 0) || pixrgn) {
			XSetClipOrigin(awt_display, gc, x - bx, y - by);
		}
		if (xormode) {
			XSetForeground(awt_display, gc, xorpixel);
			XFillRectangle(awt_display, win, gc, wx, wy, iw, ih);
		}
		XCopyArea(awt_display,
			  ird->pixmap, win, gc,
			  ix, iy, iw, ih, wx, wy);
		if (xormode) {
			XSetForeground(awt_display, gc, fgpixel ^ xorpixel);
		}
		if ((ird->mask && c == 0) || pixrgn) {
			if (clip) {
				XSetClipRectangles(awt_display, gc, 0, 0, clip, 1, YXBanded);
			} else {
				XSetClipMask(awt_display, gc, None);
			}
		}


		XUnlockDisplay(awt_display);

	}
	return 1;
}

static XImage  *savedXImage;

#ifdef MITSHM
extern int      XShmQueryExtension();

static XShmSegmentInfo *
getSharedSegment(int image_size)
{
	XShmSegmentInfo *shminfo;

	if (!XShmQueryExtension(awt_display)) {
		return 0;
	}
	shminfo = (XShmSegmentInfo *)XtMalloc(sizeof(XShmSegmentInfo));
	if (shminfo == 0) {
		return 0;
	}
	shminfo->shmid = shmget(IPC_PRIVATE, image_size, IPC_CREAT | 0777);
	if (shminfo->shmid < 0) {
		XtFree((char *)shminfo);
		shminfo = NULL;
		return 0;
	}
	shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
	if (shminfo->shmaddr == ((char *) -1)) {
		XtFree((char *)shminfo);
		shminfo = NULL;
		return 0;
	}
	shminfo->readOnly = True;
	XShmAttach(awt_display, shminfo);
        XSync(awt_display, False);
	/*
	 * Once the XSync round trip has finished then we know that the
	 * server has attached to the shared memory segment and so now we can
	 * get rid of the id so that this segment does not stick around after
	 * we go away, holding system resources.
	 */
	shmctl(shminfo->shmid, IPC_RMID, 0);
	return shminfo;
}

#define DoPutImage(dsp, win, gc, img, sx, sy, dx, dy, w, h)		\
    do {	                                                        \
        XLockDisplay(dsp);                                      \
	if (img->obdata != 0) {						\
	    XShmPutImage(dsp, win, gc, img,				\
			 sx, sy, dx, dy, w, h, False);			\
	} else if (img->bits_per_pixel ==				\
		   awtImage->wsImageFormat.bits_per_pixel) {		\
	    XPutImage(dsp, win, gc, img, sx, sy, dx, dy, w, h);		\
	} else {							\
	    PutAndReformatImage(dsp, win, gc, img,			\
				sx, sy, dx, dy, w, h);			\
	}								\
	XUnlockDisplay(dsp);                                    \
    } while (0)

#else				/* MITSHM */

#define DoPutImage(dsp, win, gc, img, sx, sy, dx, dy, w, h)		\
    do {								\
        XLockDisplay(dsp);                                      \
	if (img->bits_per_pixel ==					\
	    awtImage->wsImageFormat.bits_per_pixel)			\
	{								\
	    XPutImage(dsp, win, gc, img, sx, sy, dx, dy, w, h);		\
	} else {							\
	    PutAndReformatImage(dsp, win, gc, img,			\
				sx, sy, dx, dy, w, h);			\
	}								\
	XUnlockDisplay(dsp);                                    \
    } while (0)

#endif				/* MITSHM */

static void
dropImageBuf(XImage * img)
{
	if (img != savedXImage) {
#ifdef MITSHM
		XShmSegmentInfo *shminfo = (XShmSegmentInfo *) img->obdata;
		if (shminfo != 0) {
			XShmDetach(awt_display, shminfo);
			shmdt(shminfo->shmaddr);
			shmctl(shminfo->shmid, IPC_RMID, 0);
			XtFree((char *)shminfo);
			shminfo = NULL;
			XtFree((char *)img);
			img = NULL;
			return;
		}
#endif				/* MITSHM */
		XDestroyImage(img);
	}
}

static XImage *
getImageBuf(int depth, int bpp, int width, int height)
{
	XImage         *img;
	int             slp, bpsl;
	int             image_size;
	int             samebpp = (bpp == awtImage->wsImageFormat.bits_per_pixel);

	if (savedXImage != 0 && samebpp) {
		if (width <= savedXImage->width) {
			width = savedXImage->width;
			if (height <= savedXImage->height) {
				return savedXImage;
			}
		} else if (height < savedXImage->height) {
			height = savedXImage->height;
		}
	}
	slp = awtImage->wsImageFormat.scanline_pad;
	bpsl = paddedwidth(width * bpp, slp) >> 3;
	if (((bpsl << 3) / bpp) < width) {
		return (XImage *) 0;
	}
	img = XCreateImage(awt_display, awt_visual,
			   depth, ZPixmap, 0,
			   0, width, height, 32, bpsl);
	if (img == 0) {
		return (XImage *) 0;
	}
	img->bits_per_pixel = bpp;
	image_size = height * img->bytes_per_line;
	if (image_size / height != img->bytes_per_line) {
		/* Overflow */
		XDestroyImage(img);
		img = NULL;
		return (XImage *) 0;
	}
#ifdef MITSHM
	if (samebpp) {
		img->obdata = (char *) getSharedSegment(image_size);
		if(img->obdata!=NULL)
		  img->data = ((XShmSegmentInfo *)(img->obdata))->shmaddr;
		else
		  img->data = NULL;
	} else {
      		img->obdata = (char *) 0;
		img->data = XtMalloc(image_size);
	}
#else				/* MITSHM */
      	img->obdata = (char *) 0;
	img->data = XtMalloc(image_size);
#endif				/* MITSHM */
	if (img->data == 0) {
		XDestroyImage(img);
		img = NULL;
		return (XImage *) 0;
	}
	if (image_size < (1 << 20) && samebpp) {
		XImage         *tmp = savedXImage;
		savedXImage = img;
		if (tmp != 0) {
			dropImageBuf(tmp);
		}
	}
	return img;
}

static void
PutAndReformatImage(Display * dsp, Drawable win, GC gc, XImage * img,
		    int sx, int sy, int dx, int dy, int w, int h)
{
	XImage         *xim2;
	int             i, j, off, adjust;

	xim2 = getImageBuf(awtImage->Depth,
			   awtImage->wsImageFormat.bits_per_pixel,
			   w, h);
	if (xim2 == 0) {
		return;
	}
	off = sy * img->bytes_per_line;
	if (img->bits_per_pixel == 8) {
		u_char         *p = (u_char *) img->data;
		p += off + sx;
		adjust = img->bytes_per_line - w;
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				XPutPixel(xim2, i, j, *p++);
			}
			p += adjust;
		}
	} else if (img->bits_per_pixel == 16) {
		u_short        *p = (u_short *) img->data;
		p += (off / 2) + sx;
		adjust = (img->bytes_per_line / 2) - w;
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				XPutPixel(xim2, i, j, *p++);
			}
			p += adjust;
		}
	} else {
		u_long         *p = (u_long *) img->data;
		p += (off / 4) + sx;
		adjust = (img->bytes_per_line / 4) - w;
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				XPutPixel(xim2, i, j, *p++);
			}
			p += adjust;
		}
	}
	DoPutImage(dsp, win, gc, xim2, 0, 0, dx, dy, w, h);
	dropImageBuf(xim2);
}

#define DECLARE_XSCALE_VARS				\
    long sx1;						\
    long sxinc;						\
    long sxinc1;					\
    long sxrem;						\
    long sx1increm;					\
    long sxincrem;

#define INIT_XSCALE(dx1, sw, dw, sx)			\
    do {						\
	if (sw < 0) {					\
	    sw = -sw;					\
	    sxinc1 = -1;				\
	    sx1 = sx - SRC_XY(dx1, sw, dw) - 1;		\
	} else {					\
	    sx1 = SRC_XY(dx1, sw, dw) + sx;		\
	    sxinc1 = 1;					\
	}						\
	sxinc = sw / dw;				\
	if (sxinc1 < 0) {				\
	    sxinc = -sxinc;				\
	}						\
	sxrem = (2 * sw) % (2 * dw);			\
	sx1increm = sw % (2 * dw);			\
    } while (0)

#define INIT_SCOL(scol)					\
    do {						\
	scol = sx1;					\
	sxincrem = sx1increm;				\
    } while (0)

#define NEXT_SCOL(scol, dw)				\
    do {						\
	scol += sxinc;					\
	sxincrem += sxrem;				\
	if (sxincrem >= (2 * dw)) {			\
	    sxincrem -= (2 * dw);			\
	    scol += sxinc1;				\
	}						\
    } while (0)

#define CALC_SROW(srow, sy, y, sh, dh)			\
    do {						\
	if (sh < 0) {					\
	    srow = sy - SRC_XY(y, -sh, dh) - 1;		\
	} else {					\
	    srow = SRC_XY(y, sh, dh) + sy;		\
	}						\
    } while (0)

#define NCLIPRECTS 1000

/*
 * Malloc heap for rects instead of getting it from stack as local variable
 * to avoid huge stack usage.
 */
#define DECLARE_CLIPRECT_VARS				\
    int nrects = 0;					\
    XRectangle* rects;
/* XRectangle rects[NCLIPRECTS]; */

#define ALLOC_CLIPRECT_VARS()				\
    rects = (XRectangle*)XtMalloc(NCLIPRECTS * sizeof(XRectangle)); \
    if (!rects) return
#define FREE_CLIPRECT_VARS()				\
    do { XtFree((char *)rects); rects = NULL; } while (0)

#define SAVED_CLIPRECTS()	(nrects)
#define FREE_CLIPRECTS()	(NCLIPRECTS - nrects)

#define SET_CLIPRECTS(dsp, gc)				\
    do {						\
	XSetClipRectangles(dsp, gc, 0, 0,		\
			   rects, nrects, YXBanded);	\
	nrects = 0;					\
    } while (0)

#define ADD_CLIPRECT(dx, dy, w, h)			\
    do {						\
	rects[nrects].x = dx;				\
	rects[nrects].y = dy;				\
	rects[nrects].width = w;			\
	rects[nrects].height = h;			\
	nrects++;					\
    } while (0)

void
ScaleBytesOpaque(XImage * simg, XImage * dimg,
		 long sx, long sy, long sw, long sh, long dw, long dh,
		 long dx1, long dy1, long dx2, long dy2)
{
	long            x, y;
	long            srow, scol;
	long            prevrow = -1;
	unsigned char  *ps;
	unsigned char  *pd = (unsigned char *) dimg->data;
	long            bpsl = dimg->bytes_per_line;
	DECLARE_XSCALE_VARS

		INIT_XSCALE(dx1, sw, dw, sx);
	for (y = dy1; y < dy2; y++, pd += bpsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		if (srow == prevrow) {
			memcpy(pd, pd - bpsl, bpsl);
		} else {
			ps = (unsigned char *) simg->data;
			ps += srow * simg->bytes_per_line;
			INIT_SCOL(scol);
			for (x = dx1; x < dx2; x++) {
				*pd++ = ps[scol];
				NEXT_SCOL(scol, dw);
			}
			pd -= dx2 - dx1;
			prevrow = srow;
		}
	}
}

void
ScaleShortsOpaque(XImage * simg, XImage * dimg,
		  long sx, long sy, long sw, long sh, long dw, long dh,
		  long dx1, long dy1, long dx2, long dy2)
{
	long            x, y;
	long            srow, scol;
	long            prevrow = -1;
	unsigned short *ps;
	unsigned short *pd = (unsigned short *) dimg->data;
	long            spsl = (dimg->bytes_per_line >> 1);
	DECLARE_XSCALE_VARS

		INIT_XSCALE(dx1, sw, dw, sx);
	for (y = dy1; y < dy2; y++, pd += spsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		if (srow == prevrow) {
			memcpy(pd, pd - spsl, spsl << 1);
		} else {
			ps = (unsigned short *) simg->data;
			ps += (srow * simg->bytes_per_line) >> 1;
			INIT_SCOL(scol);
			for (x = dx1; x < dx2; x++) {
				*pd++ = ps[scol];
				NEXT_SCOL(scol, dw);
			}
			pd -= dx2 - dx1;
			prevrow = srow;
		}
	}
}

void
ScaleIntsOpaque(XImage * simg, XImage * dimg,
		long sx, long sy, long sw, long sh, long dw, long dh,
		long dx1, long dy1, long dx2, long dy2)
{
	long            x, y;
	long            srow, scol;
	long            prevrow = -1;
	unsigned int   *ps;
	unsigned int   *pd = (unsigned int *) dimg->data;
	long            ipsl = (dimg->bytes_per_line >> 2);
	DECLARE_XSCALE_VARS

		INIT_XSCALE(dx1, sw, dw, sx);
	for (y = dy1; y < dy2; y++, pd += ipsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		if (srow == prevrow) {
			memcpy(pd, pd - ipsl, ipsl << 2);
		} else {
			ps = (unsigned int *) simg->data;
			ps += (srow * simg->bytes_per_line) >> 2;
			INIT_SCOL(scol);
			for (x = dx1; x < dx2; x++) {
				*pd++ = ps[scol];
				NEXT_SCOL(scol, dw);
			}
			pd -= dx2 - dx1;
			prevrow = srow;
		}
	}
}

void
ScaleBytesMask(XImage * simg, XImage * dimg, XImage * smsk,
	       Drawable win, GC gc, XRectangle * clip,
	       long sx, long sy, long sw, long sh,
	       long dx, long dy, long dw, long dh,
	       long dx1, long dy1, long dx2, long dy2)
{
	long            x, y, x1;
	long            srow, scol;
	unsigned char  *ps;
	unsigned char  *pd = (unsigned char *) dimg->data;
	unsigned int   *pm;
	long            bpsl = dimg->bytes_per_line;
	DECLARE_CLIPRECT_VARS
		DECLARE_XSCALE_VARS

		ALLOC_CLIPRECT_VARS();

	INIT_XSCALE(dx1, sw, dw, sx);
	bpsl -= dx2 - dx1;
	for (y = dy1; y < dy2; y++, pd += bpsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		ps = (unsigned char *) simg->data;
		ps += srow * simg->bytes_per_line;
		pm = (unsigned int *) smsk->data;
		pm += (srow * smsk->bytes_per_line) >> 2;
		x1 = -1;
		INIT_SCOL(scol);
		for (x = dx1; x < dx2; x++) {
			if ((pm[scol >> 5] & (1 << (31 - (scol & 0x1f)))) != 0) {
				if (x1 < 0) {
					x1 = x;
				}
				*pd = ps[scol];
			} else {
				if (x1 >= 0) {
					if (FREE_CLIPRECTS() <= 1) {
						SET_CLIPRECTS(awt_display, gc);
						DoPutImage(awt_display, win, gc, dimg,
							   0, 0, dx, dy, dx2 - dx1, dy2 - dy1);
					}
					ADD_CLIPRECT(dx + x1, dy + y, x - x1, 1);
				}
				x1 = -1;
			}
			pd++;
			NEXT_SCOL(scol, dw);
		}
		if (x1 >= 0) {
			/* There is always room for one more... */
			ADD_CLIPRECT(dx + x1, dy + y, x - x1, 1);
		}
	}
	if (SAVED_CLIPRECTS() > 0) {
		SET_CLIPRECTS(awt_display, gc);
		DoPutImage(awt_display, win, gc, dimg,
			   0, 0, dx, dy, dx2 - dx1, dy2 - dy1);
	}
	if (clip) {
		XSetClipRectangles(awt_display, gc, 0, 0, clip, 1, YXBanded);
	} else {
		XSetClipMask(awt_display, gc, None);
	}

	FREE_CLIPRECT_VARS();
}

void
ScaleShortsMask(XImage * simg, XImage * dimg, XImage * smsk,
		Drawable win, GC gc, XRectangle * clip,
		long sx, long sy, long sw, long sh,
		long dx, long dy, long dw, long dh,
		long dx1, long dy1, long dx2, long dy2)
{
	long            x, y, x1;
	long            srow, scol;
	unsigned short *ps;
	unsigned short *pd = (unsigned short *) dimg->data;
	unsigned int   *pm;
	long            spsl = (dimg->bytes_per_line >> 1);
	DECLARE_CLIPRECT_VARS
		DECLARE_XSCALE_VARS

		ALLOC_CLIPRECT_VARS();

	INIT_XSCALE(dx1, sw, dw, sx);
	spsl -= dx2 - dx1;
	for (y = dy1; y < dy2; y++, pd += spsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		ps = (unsigned short *) simg->data;
		ps += (srow * simg->bytes_per_line) >> 1;
		pm = (unsigned int *) smsk->data;
		pm += (srow * smsk->bytes_per_line) >> 2;
		x1 = -1;
		INIT_SCOL(scol);
		for (x = dx1; x < dx2; x++) {
			if ((pm[scol >> 5] & (1 << (31 - (scol & 0x1f)))) != 0) {
				if (x1 < 0) {
					x1 = x;
				}
				*pd = ps[scol];
			} else {
				if (x1 >= 0) {
					if (FREE_CLIPRECTS() <= 1) {
						SET_CLIPRECTS(awt_display, gc);
						DoPutImage(awt_display, win, gc, dimg,
							   0, 0, dx, dy, dx2 - dx1, dy2 - dy1);
					}
					ADD_CLIPRECT(dx + x1, dy + y, x - x1, 1);
				}
				x1 = -1;
			}
			pd++;
			NEXT_SCOL(scol, dw);
		}
		if (x1 >= 0) {
			/* There is always room for one more... */
			ADD_CLIPRECT(dx + x1, dy + y, x - x1, 1);
		}
	}
	if (SAVED_CLIPRECTS() > 0) {
		SET_CLIPRECTS(awt_display, gc);
		DoPutImage(awt_display, win, gc, dimg,
			   0, 0, dx, dy, dx2 - dx1, dy2 - dy1);
	}
	if (clip) {
		XSetClipRectangles(awt_display, gc, 0, 0, clip, 1, YXBanded);
	} else {
		XSetClipMask(awt_display, gc, None);
	}
	FREE_CLIPRECT_VARS();
}

void
ScaleIntsMask(XImage * simg, XImage * dimg, XImage * smsk,
	      Drawable win, GC gc, XRectangle * clip,
	      long sx, long sy, long sw, long sh,
	      long dx, long dy, long dw, long dh,
	      long dx1, long dy1, long dx2, long dy2)
{
	long            x, y, x1;
	long            srow, scol;
	unsigned int   *ps;
	unsigned int   *pd = (unsigned int *) dimg->data;
	unsigned int   *pm;
	long            ipsl = (dimg->bytes_per_line >> 2);
	DECLARE_CLIPRECT_VARS
		DECLARE_XSCALE_VARS

		ALLOC_CLIPRECT_VARS();

	INIT_XSCALE(dx1, sw, dw, sx);
	ipsl -= dx2 - dx1;
	for (y = dy1; y < dy2; y++, pd += ipsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		ps = (unsigned int *) simg->data;
		ps += (srow * simg->bytes_per_line) >> 2;
		pm = (unsigned int *) smsk->data;
		pm += (srow * smsk->bytes_per_line) >> 2;
		x1 = -1;
		INIT_SCOL(scol);
		for (x = dx1; x < dx2; x++) {
			if ((pm[scol >> 5] & (1 << (31 - (scol & 0x1f)))) != 0) {
				if (x1 < 0) {
					x1 = x;
				}
				*pd = ps[scol];
			} else {
				if (x1 >= 0) {
					if (FREE_CLIPRECTS() <= 1) {
						SET_CLIPRECTS(awt_display, gc);
						DoPutImage(awt_display, win, gc, dimg,
							   0, 0, dx, dy, dx2 - dx1, dy2 - dy1);
					}
					ADD_CLIPRECT(dx + x1, dy + y, x - x1, 1);
				}
				x1 = -1;
			}
			pd++;
			NEXT_SCOL(scol, dw);
		}
		if (x1 >= 0) {
			/* There is always room for one more... */
			ADD_CLIPRECT(dx + x1, dy + y, x - x1, 1);
		}
	}
	if (SAVED_CLIPRECTS() > 0) {
		SET_CLIPRECTS(awt_display, gc);
		DoPutImage(awt_display, win, gc, dimg,
			   0, 0, dx, dy, dx2 - dx1, dy2 - dy1);
	}
	if (clip) {
		XSetClipRectangles(awt_display, gc, 0, 0, clip, 1, YXBanded);
	} else {
		XSetClipMask(awt_display, gc, None);
	}
	FREE_CLIPRECT_VARS();
}

void
ScaleBytesMaskBG(XImage * simg, XImage * dimg, XImage * smsk,
		 long sx, long sy, long sw, long sh, long dw, long dh,
		 long dx1, long dy1, long dx2, long dy2,
		 int bgcolor)
{
	unsigned int    pixel;
	long            x, y;
	long            srow, scol;
	long            prevrow = -1;
	unsigned char  *ps;
	unsigned char  *pd = (unsigned char *) dimg->data;
	unsigned int   *pm;
	long            bpsl = dimg->bytes_per_line;
	DECLARE_XSCALE_VARS

		INIT_XSCALE(dx1, sw, dw, sx);
	for (y = dy1; y < dy2; y++, pd += bpsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		if (srow == prevrow) {
			memcpy(pd, pd - bpsl, bpsl);
		} else {
			ps = (unsigned char *) simg->data;
			ps += srow * simg->bytes_per_line;
			pm = (unsigned int *) smsk->data;
			pm += (srow * smsk->bytes_per_line) >> 2;
			INIT_SCOL(scol);
			for (x = dx1; x < dx2; x++) {
				if ((pm[scol >> 5] & (1 << (31 - (scol & 0x1f)))) != 0) {
					pixel = ps[scol];
				} else {
					pixel = bgcolor;
				}
				*pd++ = pixel;
				NEXT_SCOL(scol, dw);
			}
			pd -= dx2 - dx1;
			prevrow = srow;
		}
	}
}

void
ScaleShortsMaskBG(XImage * simg, XImage * dimg, XImage * smsk,
		  long sx, long sy, long sw, long sh, long dw, long dh,
		  long dx1, long dy1, long dx2, long dy2,
		  int bgcolor)
{
	unsigned int    pixel;
	long            x, y;
	long            srow, scol;
	long            prevrow = -1;
	unsigned short *ps;
	unsigned short *pd = (unsigned short *) dimg->data;
	unsigned int   *pm;
	long            spsl = (dimg->bytes_per_line >> 1);
	DECLARE_XSCALE_VARS

		INIT_XSCALE(dx1, sw, dw, sx);
	for (y = dy1; y < dy2; y++, pd += spsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		if (srow == prevrow) {
			memcpy(pd, pd - spsl, spsl << 1);
		} else {
			ps = (unsigned short *) simg->data;
			ps += (srow * simg->bytes_per_line) >> 1;
			pm = (unsigned int *) smsk->data;
			pm += (srow * smsk->bytes_per_line) >> 2;
			INIT_SCOL(scol);
			for (x = dx1; x < dx2; x++) {
				if ((pm[scol >> 5] & (1 << (31 - (scol & 0x1f)))) != 0) {
					pixel = ps[scol];
				} else {
					pixel = bgcolor;
				}
				*pd++ = pixel;
				NEXT_SCOL(scol, dw);
			}
			pd -= dx2 - dx1;
			prevrow = srow;
		}
	}
}

void
ScaleIntsMaskBG(XImage * simg, XImage * dimg, XImage * smsk,
		long sx, long sy, long sw, long sh, long dw, long dh,
		long dx1, long dy1, long dx2, long dy2,
		int bgcolor)
{
	unsigned int    pixel;
	long            x, y;
	long            srow, scol;
	long            prevrow = -1;
	unsigned int   *ps;
	unsigned int   *pd = (unsigned int *) dimg->data;
	unsigned int   *pm;
	long            ipsl = (dimg->bytes_per_line >> 2);
	DECLARE_XSCALE_VARS

		INIT_XSCALE(dx1, sw, dw, sx);
	for (y = dy1; y < dy2; y++, pd += ipsl) {
		CALC_SROW(srow, sy, y, sh, dh);
		if (srow == prevrow) {
			memcpy(pd, pd - ipsl, ipsl << 2);
		} else {
			ps = (unsigned int *) simg->data;
			ps += (srow * simg->bytes_per_line) >> 2;
			pm = (unsigned int *) smsk->data;
			pm += (srow * smsk->bytes_per_line) >> 2;
			INIT_SCOL(scol);
			for (x = dx1; x < dx2; x++) {
				if ((pm[scol >> 5] & (1 << (31 - (scol & 0x1f)))) != 0) {
					pixel = ps[scol];
				} else {
					pixel = bgcolor;
				}
				*pd++ = pixel;
				NEXT_SCOL(scol, dw);
			}
			pd -= dx2 - dx1;
			prevrow = srow;
		}
	}
}

#define SWAP(c1, c2)		\
    do {			\
	c1 ^= c2;		\
	c2 ^= c1;		\
	c1 ^= c2;               \
    } while (0)

int
awt_imageStretch(JNIEnv * env, jobject irh, jobject c, Drawable win, GC gc,
		 int xormode, unsigned long xorpixel, unsigned long fgpixel,
		 long dx1, long dy1, long dx2, long dy2,
		 long sx1, long sy1, long sx2, long sy2,
		 XRectangle * clip)
{
	IRData         *ird;
	long            ix1;
	long            iy1;
	long            ix2;
	long            iy2;
	long            sw;
	long            sh;
	long            dw;
	long            dh;
	long            wx1;
	long            wy1;
	long            wx2;
	long            wy2;
	unsigned int    bgcolor;
	XImage         *dimg;
	XImage         *simg;
	XImage         *smsk;
	jint            availinfo;

	if (irh == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return 0;
	}
	availinfo = (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);
	if ((availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
		return 1;
	}
	ird = image_getIRData(env, irh, NULL);
	if (ird == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return 0;
	} else {
		if (ird->pixmap == None) {
			return 1;
		}
		if (win == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "win");
			return 0;
		}
		if ((availinfo & IMAGE_FULLDRAWINFO) != 0) {
			ix1 = iy1 = 0;
			ix2 = ix1 + (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
			iy2 = iy1 + (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_heightFID);
		} else {
			/* The region will be converted to a mask below... */
			ix1 = iy1 = 0;
			ix2 = ix1 + (*env)->GetIntField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
			iy2 = iy1 + ird->curlines.num;
		}

		sw = sx2 - sx1;
		dw = dx2 - dx1;
		if (dw < 0) {
			SWAP(dx1, dx2);
			SWAP(sx1, sx2);
			dw = -dw;
			sw = -sw;
		}
		sh = sy2 - sy1;
		dh = dy2 - dy1;
		if (dh < 0) {
			SWAP(dy1, dy2);
			SWAP(sy1, sy2);
			dh = -dh;
			sh = -sh;
		}
		if (sw == 0 || sh == 0) {
			/* Protect against arithmetic exceptions below... */
			return 1;
		}
		if (dw == 0 || dh == 0) {
			return 1;
		}
		if (sw < 0) {
			if (ix1 < sx2) {
				ix1 = sx2;
			}
			if (ix2 > sx1) {
				ix2 = sx1;
			}
			wx2 = dw - DEST_XY_RANGE_START(ix1 - sx2, -sw, dw);
			wx1 = dw - DEST_XY_RANGE_START(ix2 - sx2, -sw, dw);
		} else {
			if (ix1 < sx1) {
				ix1 = sx1;
			}
			if (ix2 > sx2) {
				ix2 = sx2;
			}
			wx1 = DEST_XY_RANGE_START(ix1 - sx1, sw, dw);
			wx2 = DEST_XY_RANGE_START(ix2 - sx1, sw, dw);
		}
		if (sh < 0) {
			if (iy1 < sy2) {
				iy1 = sy2;
			}
			if (iy2 > sy1) {
				iy2 = sy1;
			}
			wy2 = dh - DEST_XY_RANGE_START(iy1 - sy2, -sh, dh);
			wy1 = dh - DEST_XY_RANGE_START(iy2 - sy2, -sh, dh);
		} else {
			if (iy1 < sy1) {
				iy1 = sy1;
			}
			if (iy2 > sy2) {
				iy2 = sy2;
			}
			wy1 = DEST_XY_RANGE_START(iy1 - sy1, sh, dh);
			wy2 = DEST_XY_RANGE_START(iy2 - sy1, sh, dh);
		}
		if (ix2 <= ix1 || iy2 <= iy1) {
			return 1;
		}
		if (wx2 <= wx1 || wy2 <= wy1) {
			return 1;
		}
		dimg = getImageBuf(awtImage->Depth, awtImage->clrdata.bitsperpixel,
				   (wx2 - wx1), (wy2 - wy1));
		if (dimg == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, "ImageStretch temporary image");
			return 0;
		}
		if ((*env)->GetBooleanField(env, irh, MCachedIDs.sun_awt_image_ImageRepresentation_offscreenFID)) {

		  XLockDisplay(awt_display);

			if (ird->xim == 0) {
				ird->xim = XCreateImage(awt_display, awt_visual,
						    dimg->depth, ZPixmap, 0,
					    0, ird->dstW, ird->dstH, 32, 0);
				if (ird->xim != 0) {
					ird->xim->data = XtMalloc(ird->dstH * ird->xim->bytes_per_line);
					ird->cvdata.outbuf = ird->xim->data;
					if (ird->xim->data == 0) {
						XDestroyImage(ird->xim);
						ird->xim = 0;
					}
				}

				if (ird->xim == 0) {
					(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, "Offscreen temporary buffer");
					dropImageBuf(dimg);
					XUnlockDisplay(awt_display);
					return 0;
				}
			}
			XGetSubImage(awt_display, ird->pixmap,
				     0, 0, ird->dstW, ird->dstH,
				     ~0, ZPixmap, ird->xim, 0, 0);

			XUnlockDisplay(awt_display);
		}
		simg = ird->xim;
		smsk = ird->maskim;
		if (smsk == 0 && ird->curpixels != 0) {
			/*
			 * There is no convenient Region readback in Xlib so
			 * we convert to an imagemask instead (this also
			 * saves duplication of effort as we've already
			 * created the mask versions of the functions).
			 */
			image_InitMask(ird, 0, 0, 0, 0);
			smsk = ird->maskim;
		}
		bgcolor = (c == 0) ? 0 : awt_getColor(env, c);
		if (ird->depth == 32) {
			if (smsk == 0) {
				ScaleIntsOpaque(simg, dimg,
						sx1, sy1, sw, sh, dw, dh,
						wx1, wy1, wx2, wy2);
			} else if (c == 0) {
				ScaleIntsMask(simg, dimg, smsk, win, gc, clip,
					      sx1, sy1, sw, sh,
					      dx1, dy1, dw, dh,
					      wx1, wy1, wx2, wy2);
			} else {
				ScaleIntsMaskBG(simg, dimg, smsk,
						sx1, sy1, sw, sh, dw, dh,
						wx1, wy1, wx2, wy2, bgcolor);
			}
		} else if (ird->depth == 16) {
			if (smsk == 0) {
				ScaleShortsOpaque(simg, dimg,
						  sx1, sy1, sw, sh, dw, dh,
						  wx1, wy1, wx2, wy2);
			} else if (c == 0) {
				ScaleShortsMask(simg, dimg, smsk, win, gc, clip,
						sx1, sy1, sw, sh,
						dx1, dy1, dw, dh,
						wx1, wy1, wx2, wy2);
			} else {
				ScaleShortsMaskBG(simg, dimg, smsk,
						  sx1, sy1, sw, sh, dw, dh,
					       wx1, wy1, wx2, wy2, bgcolor);
			}
		} else if (ird->depth == 8) {
			if (smsk == 0) {
				ScaleBytesOpaque(simg, dimg,
						 sx1, sy1, sw, sh, dw, dh,
						 wx1, wy1, wx2, wy2);
			} else if (c == 0) {
				ScaleBytesMask(simg, dimg, smsk, win, gc, clip,
					       sx1, sy1, sw, sh,
					       dx1, dy1, dw, dh,
					       wx1, wy1, wx2, wy2);
			} else {
				ScaleBytesMaskBG(simg, dimg, smsk,
						 sx1, sy1, sw, sh, dw, dh,
					       wx1, wy1, wx2, wy2, bgcolor);
			}
		} else {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_InternalErrorClass, "ImageStretch unsupported depth");
			return 0;
		}
		if (c != 0) {

		  XLockDisplay(awt_display);

			if (wy1 > 0 || wx1 > 0 || wx2 < dw || wy2 < dh) {
				XSetForeground(awt_display, gc,
				  xormode ? (bgcolor ^ xorpixel) : bgcolor);
			}
			if (wy1 > 0) {
				XFillRectangle(awt_display, win, gc,
					       dx1, dy1, dw, wy1);
			}
			if (wx1 > 0) {
				XFillRectangle(awt_display, win, gc,
					    dx1, dy1 + wy1, wx1, wy2 - wy1);
			}

			XUnlockDisplay(awt_display);

		}
		if (smsk == 0 || c != 0) {
			DoPutImage(awt_display, win, gc, dimg, 0, 0,
				dx1 + wx1, dy1 + wy1, wx2 - wx1, wy2 - wy1);
		}
		if (c != 0) {
		  XLockDisplay(awt_display);

			if (wx2 < dw) {
				XFillRectangle(awt_display, win, gc,
				 dx1 + wx2, dy1 + wy1, dw - wx2, wy2 - wy1);
			}
			if (wy2 < dh) {
				XFillRectangle(awt_display, win, gc,
					       dx1, dy1 + wy2, dw, dh - wy2);
			}
			if (wy1 > 0 || wx1 > 0 || wx2 < dw || wy2 < dh) {
				XSetForeground(awt_display, gc,
				  xormode ? (fgpixel ^ xorpixel) : fgpixel);
			}
			XUnlockDisplay(awt_display);
		}
		dropImageBuf(dimg);
	}
	return 1;
}

JNIEXPORT void  JNICALL
Java_sun_awt_image_ImageRepresentation_disposeImage(JNIEnv * env, jobject this)
{
	IRData         *ird;

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	ird = (IRData *) (*env)->GetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);

	if (ird != NULL) {
		if (ird->pixmap != None) {
			XFreePixmap(awt_display, ird->pixmap);
			ird->pixmap = None;
		}
		if (ird->mask != None) {
			XFreePixmap(awt_display, ird->mask);
			ird->mask = None;
		}
		image_FreeRenderData(ird);
		image_FreeBufs(ird);
		XtFree((char *)ird);
		ird = NULL;
	}
	(*env)->SetIntField(env, this, MCachedIDs.sun_awt_image_ImageRepresentation_pDataFID, (jint) 0);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_image_OffScreenImageSource_sendPixels(JNIEnv * env, jobject osish)
{
	jobject         irh, theConsumer, pixh, cm;

	IRData         *ird;

	char           *buf;
	int             i, w, h, d;
	XImage         *xim;
	XID             pixmap;
	jint            availinfo;

	irh = (*env)->GetObjectField(env, osish, MCachedIDs.sun_awt_image_OffScreenImageSource_baseIRFID);

	if (irh == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	availinfo = (*env)->GetIntField(env, osish, MCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);
	if ((availinfo & IMAGE_OFFSCRNINFO) != IMAGE_OFFSCRNINFO) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		return;
	}
	AWT_LOCK();
	ird = image_getIRData(env, irh, NULL);
	if (ird == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	w = ird->dstW;
	h = ird->dstH;
	d = ird->depth;
	pixmap = ird->pixmap;
	AWT_UNLOCK();

	theConsumer = (*env)->GetObjectField(env, osish, MCachedIDs.sun_awt_image_OffScreenImageSource_theConsumerFID);
	if (theConsumer == NULL) {
		return;
	}
	cm = awt_getColorModel(env);

	(*env)->CallVoidMethod(env, osish, MCachedIDs.java_awt_image_ImageConsumer_setColorModelMID, cm);

	if ((*env)->ExceptionCheck(env)) {
		return;
	}
	(*env)->CallVoidMethod(env, osish, MCachedIDs.java_awt_image_ImageConsumer_setHintsMID, (jint) HINTS_OFFSCREENSEND);

	if ((*env)->ExceptionCheck(env)) {
		return;
	}
	AWT_LOCK();



	if (d > 8) {
		pixh = (*env)->NewIntArray(env, w);
		if ((*env)->ExceptionCheck(env)) {
			AWT_UNLOCK();
			return;
		}
		buf = (char *) (*env)->GetIntArrayElements(env, pixh, NULL);
		if ((*env)->ExceptionCheck(env)) {
			AWT_UNLOCK();
			return;
		}
		xim = XCreateImage(awt_display, awt_visual, awtImage->Depth, ZPixmap, 0, buf, w, 1, 32, 0);
		xim->bits_per_pixel = 32;

	} else {
		/*
		 * XGetSubImage takes care of expanding to 8 bit if
		 * necessary.
		 */
		/*
		 * But, that is not a supported feature.  There are better
		 * ways
		 */
		/* of handling this... */

		pixh = (*env)->NewIntArray(env, w);
		if ((*env)->ExceptionCheck(env)) {
			AWT_UNLOCK();
			return;
		}
		buf = (char *) (*env)->GetIntArrayElements(env, pixh, NULL);
		if ((*env)->ExceptionCheck(env)) {
			AWT_UNLOCK();
			return;
		}
		xim = XCreateImage(awt_display, awt_visual, awtImage->Depth, ZPixmap, 0, buf, w, 1, 8, 0);
		xim->bits_per_pixel = 8;
	}



	AWT_UNLOCK();

	XLockDisplay(awt_display);

	for (i = 0; i < h; i++) {
		if (theConsumer == NULL) {
			break;
		}
		AWT_LOCK();
		XGetSubImage(awt_display, pixmap, 0, i, w, 1, ~0, ZPixmap, xim, 0, 0);
		AWT_UNLOCK();

		if (d > 8) {
			(*env)->CallVoidMethod(env, theConsumer, MCachedIDs.java_awt_image_ImageConsumer_setPixels2MID,
					       0, i, w, 1,
					       cm, pixh, 0, w);
		} else {
			(*env)->CallVoidMethod(env, theConsumer, MCachedIDs.java_awt_image_ImageConsumer_setPixelsMID,
					       0, i, w, 1,
					       cm, pixh, 0, w);
		}

		if ((*env)->ExceptionCheck(env)) {
			break;
		}
	}


	XUnlockDisplay(awt_display);

	AWT_LOCK();
	XDestroyImage(xim);
	xim = NULL;
	AWT_UNLOCK();

}
