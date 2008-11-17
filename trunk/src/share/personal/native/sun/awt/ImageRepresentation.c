/*
 * @(#)ImageRepresentation.c	1.37 06/10/10
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

#include "sun_awt_image_ImageRepresentation.h"
#include "Xheaders.h"

#include "common.h"

#include "img_util.h"
#include "img_util_md.h"

/* #define ZALLOC(typ) (typ *) sysCalloc(1, sizeof(typ)) */

#define CVFUNCTRACKING
#ifdef CVFUNCTRACKING
#include <dlfcn.h>
static int            prevflags = 0;
static ImgConvertFcn *prevfunc = NULL;
#endif


#undef MITSHM

/************************************************************/
/* In large part, this is a JNI'd version of code from      */
/* the JDK 1.1.x version of solaris/sun/awt/image/image.c   */
/* (Hence all of the commented out lines like // AWT_LOCK() */
/************************************************************/

/* mask_gc is a gc that is compatible with 1-bit deep Pixmaps;  */
/* it is used for creating or modifying image masks on the fly. */
static GC mask_gc;

static unsigned long white_pixel;

#define IMAGE_SIZEINFO (IRCachedIDs.WIDTHFID | IRCachedIDs.HEIGHTFID)

#define IMAGE_DRAWINFO (IRCachedIDs.WIDTHFID | IRCachedIDs.SOMEBITSFID | IRCachedIDs.HEIGHTFID)

#define IMAGE_FULLDRAWINFO (IRCachedIDs.FRAMEBITSFID | IRCachedIDs.ALLBITSFID)

#define IMAGE_OFFSCRNINFO (IRCachedIDs.WIDTHFID | IRCachedIDs.HEIGHTFID | IRCachedIDs.SOMEBITSFID | IRCachedIDs.ALLBITSFID)

#define HINTS_DITHERFLAGS (IRCachedIDs.TOPDOWNLEFTRIGHTFID)
#define HINTS_SCANLINEFLAGS (IRCachedIDs.COMPLETESCANLINESFID)

#define HINTS_OFFSCREENSEND (IRCachedIDs.TOPDOWNLEFTRIGHTFID | IRCachedIDs.COMPLETESCANLINESFID | IRCachedIDs.SINGLEPASSFID)


/*
JNIACCESS(sun_awt_image_ImageRepresentation);
JNIACCESS(sun_awt_image_OffscreenImageSource);
JNIACCESS(java_awt_image_DirectColorModel);
JNIACCESS(java_awt_image_ImageObserver);
JNIACCESS(java_awt_image_ImageConsumer);
JNIACCESS(java_awt_image_IndexColorModel);
JNIACCESS(java_awt_image_ImageObserver);
*/


struct IRCachedIDs IRCachedIDs;


void initIRClass(JNIEnv *env, jobject obj)
{

/*

    FINDCLASS(sun_awt_image_ImageRepresentation,
	      "sun/awt/image/ImageRepresentation");
    DEFINEMETHOD(sun_awt_image_ImageRepresentation, reconstruct, "(I)V");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  pData, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  srcW, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  srcH, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  width, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  height, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  hints, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  availinfo, "I");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  offscreen, "Z");
    DEFINEFIELD(sun_awt_image_ImageRepresentation,  newbits,
		"Ljava/awt/Rectangle;");

    FINDCLASS(java_awt_image_ImageObserver,
	      "java/awt/image/ImageObserver");
    DEFINECONSTANT(java_awt_image_ImageObserver, WIDTH,     "I", Int);
    DEFINECONSTANT(java_awt_image_ImageObserver, HEIGHT,    "I", Int);
    DEFINECONSTANT(java_awt_image_ImageObserver, SOMEBITS,  "I", Int);
    DEFINECONSTANT(java_awt_image_ImageObserver, FRAMEBITS, "I", Int);
    DEFINECONSTANT(java_awt_image_ImageObserver, ALLBITS,   "I", Int);

    FINDCLASS(java_awt_image_ImageConsumer, "java/awt/image/ImageConsumer");
    DEFINEMETHOD(java_awt_image_ImageConsumer, setColorModel,
		 "(Ljava/awt/image/ColorModel;)V");
    DEFINEMETHOD(java_awt_image_ImageConsumer, setHints, "(I)V");
    DEFINEMETHODALT(java_awt_image_ImageConsumer, setIntPixels, "setPixels",
		 "(IIIILjava/awt/image/ColorModel;[III)V");
    DEFINEMETHODALT(java_awt_image_ImageConsumer, setBytePixels, "setPixels",
		 "(IIIILjava/awt/image/ColorModel;[BII)V");

    DEFINECONSTANT(java_awt_image_ImageConsumer, TOPDOWNLEFTRIGHT,  "I", Int);
    DEFINECONSTANT(java_awt_image_ImageConsumer, COMPLETESCANLINES, "I", Int);
    DEFINECONSTANT(java_awt_image_ImageConsumer, SINGLEPASS,        "I", Int);

*/





    /* other static initializations */
    NATIVE_LOCK(env);

    white_pixel = WhitePixel(display, DefaultScreen(display));
    {
	Pixmap one_bit;
	XGCValues xgcv;
	xgcv.background = 0;
	xgcv.foreground = 1;
	one_bit = XCreatePixmap(display, root_drawable, 1, 1, 1);
	mask_gc = XCreateGC(display, one_bit, GCForeground | GCBackground, &xgcv);
	XFreePixmap(display, one_bit);
    }

    NATIVE_UNLOCK(env);
}

extern void initDirectColorModel(JNIEnv *env);
extern void initIndexColorModel(JNIEnv *env);

jobject getColorModel(JNIEnv *env) {
    Visual    *v;
    int       depth;
    jobject   colorModel;

    v     = root_visual;
    depth = screen_depth;

    if (v->class == TrueColor) {
        /* make a DirectColorModel */
/*
	if (MUSTLOAD(java_awt_image_DirectColorModel)) {
	    initDirectColorModel(env);
	}
*/

/*
	colorModel = 
	    (*env)->NewObject(env, 
			      JNIjava_awt_image_DirectColorModel.class,
			      JNIjava_awt_image_DirectColorModel.newIIIII,
			      depth, v->red_mask, v->green_mask, v->blue_mask, 
			      0);
*/

        colorModel = 
            (*env)->NewObject(env, IRCachedIDs.DirectColorModel,
                                   IRCachedIDs.DirectColorModel_constructor,
                                   depth, v->red_mask, v->green_mask, v->blue_mask, 0 );



    } else {
        /* Make an IndexColorModel */
        jbyteArray red = (*env)->NewByteArray(env, numColors);
	jbyteArray grn = (*env)->NewByteArray(env, numColors);
	jbyteArray blu = (*env)->NewByteArray(env, numColors);
	jbyte *rBase, *gBase, *bBase;
	unsigned int i;

/*
	if (MUSTLOAD(java_awt_image_IndexColorModel)) {
	    initIndexColorModel(env);
	}
*/

	rBase = (*env)->GetByteArrayElements(env, red, 0);
	gBase = (*env)->GetByteArrayElements(env, grn, 0);
	bBase = (*env)->GetByteArrayElements(env, blu, 0);

	for (i = 0; i < numColors; ++i) {
	    int r, g, b;
	    pixel2RGB(i, &r, &g, &b);
	    rBase[i] = (jbyte)r;
	    gBase[i] = (jbyte)g;
	    bBase[i] = (jbyte)b;
	}

	(*env)->ReleaseByteArrayElements(env, red, rBase, JNI_COMMIT);
	(*env)->ReleaseByteArrayElements(env, grn, gBase, JNI_COMMIT);
	(*env)->ReleaseByteArrayElements(env, blu, bBase, JNI_COMMIT);

/*
	colorModel= 
	    (*env)->NewObject(env, 
			      JNIjava_awt_image_IndexColorModel.class,
			      JNIjava_awt_image_IndexColorModel.newIIaBaBaB,
			      depth, numColors, red, grn, blu);
*/

        colorModel = 
            (*env)->NewObject(env, IRCachedIDs.IndexColorModel,
                                   IRCachedIDs.IndexColorModel_constructor,
                                   depth, numColors, red, grn, blu );

    }

    return colorModel;
}

static GC
getImageGC(Pixmap pixmap) {
    static GC _imagegc;

    assert(NATIVE_LOCK_HELD());
    if (!_imagegc) {
	_imagegc = XCreateGC(display, pixmap, 0, NULL);
	XSetForeground(display, _imagegc, white_pixel);
    }
    return _imagegc;
}

IRData *
image_getIRData(JNIEnv *env, jobject obj, jobject color, int doReconstruct)
{
    GC imagegc;
    IRData *ird;

    if (obj == 0) {
	return NULL;
    }



#if 0

    if (JNIsun_awt_image_ImageRepresentation.class == 0) {
	initIRClass(env, obj);
    }


    if (! (*env)->IsInstanceOf(env, obj, 
			       JNIsun_awt_image_ImageRepresentation.class)) {
	jclass cls;
	jmethodID mid;
	jobject str;
	const char *ctmp;
	int i = 0;
	fprintf(stderr, "Requesting IRData from something that is not\n");
	fprintf(stderr, "an ImageRepresentation object!\n");
	cls = (*env)->GetObjectClass(env, obj);
	mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
	str = (*env)->CallObjectMethod(env, obj, mid);
	ctmp = (*env)->GetStringUTFChars(env, str, 0);
	fprintf(stderr, "Object is %s\n", ctmp);
	fflush(stderr);
	/* i = 100 / i; */
    }
#endif

    ird = (IRData *)(*env)->GetIntField(env, obj, IRCachedIDs.pDataFID);

    if (doReconstruct) {
        (*env)->CallVoidMethod(env, obj, IRCachedIDs.reconstructMID, IMAGE_SIZEINFO|IRCachedIDs.ALLBITSFID);
        ird = (IRData *)(*env)->GetIntField(env, obj, IRCachedIDs.pDataFID);
    }

    if (ird == 0) {
        int avail = (*env)->GetIntField(env, obj, IRCachedIDs.availinfoFID);

	if ((avail & IMAGE_SIZEINFO) != IMAGE_SIZEINFO) {
	    return NULL;
	}

        ird = calloc(1, sizeof(IRData));

	if (ird == NULL) {
	    return NULL;
	}

        ird->dstW = (*env)->GetIntField(env, obj, IRCachedIDs.widthFID);
        ird->dstH = (*env)->GetIntField(env, obj, IRCachedIDs.heightFID);

	NATIVE_LOCK(env);
	ird->pixmap = XCreatePixmap(display, root_drawable,
				    (unsigned)ird->dstW, 
				    (unsigned)ird->dstH, 
				    (unsigned)awtImage->Depth);
	imagegc = getImageGC(ird->pixmap);
	if (color != NULL) {
	    XSetForeground(display, imagegc, gfXGetPixel(env, color, NULL, TRUE));
	}
	XFillRectangle(display, ird->pixmap, imagegc, 0, 0, 
		       (unsigned)ird->dstW, (unsigned)ird->dstH);
	if (color != 0) {
	    XSetForeground(display, imagegc, white_pixel);
	}
	NATIVE_UNLOCK(env);

	ird->depth = awtImage->clrdata.bitsperpixel;
        ird->hints = (*env)->GetIntField(env, obj, IRCachedIDs.hintsFID);

        (*env)->SetIntField(env, obj, IRCachedIDs.pDataFID, (jint)ird);

    } else if (ird->hints == 0) {
        ird->hints = (*env)->GetIntField(env, obj, IRCachedIDs.hintsFID);
    }

    return ird;
}


void
image_FreeBufs(IRData *ird)
{
    if (ird->cvdata.outbuf) {
	free(ird->cvdata.outbuf);
	ird->cvdata.outbuf = 0;
    }
    if (ird->xim) {
	XFree(ird->xim);
	ird->xim = 0;
    }
    if (ird->cvdata.maskbuf) {
	free(ird->cvdata.maskbuf);
	ird->cvdata.maskbuf = 0;
    }
    if (ird->maskim) {
	XFree(ird->maskim);
	ird->maskim = 0;
    }
}

void
image_FreeRenderData(IRData *ird)
{
    if (ird->cvdata.fserrors) {
	free(ird->cvdata.fserrors);
	ird->cvdata.fserrors = 0;
    }
    if (ird->curpixels) {
	XDestroyRegion(ird->curpixels);
	ird->curpixels = 0;
    }
    if (ird->curlines.seen) {
	free(ird->curlines.seen);
	ird->curlines.seen = 0;
    }
}

void *
image_InitMask(JNIEnv *env, IRData *ird, int x1, int y1, int x2, int y2)
{
    void *mask;
    int scansize, bufsize;
    Region pixrgn;

    NATIVE_LOCK(env);

    scansize = paddedwidth(ird->dstW, 32) >> 3;
    bufsize = scansize * ird->dstH + 1;
    if ((bufsize < 0) || ((bufsize - 1) / scansize != ird->dstH)) {
	/* calculation must have overflowed */
	ird->cvdata.maskbuf = 0;
	NATIVE_UNLOCK(env);
	return (void *) NULL;
    }
    mask = (void *) calloc(1, bufsize);
    ird->cvdata.maskbuf = mask;
    ird->cvdata.mask_end = ((char*)ird->cvdata.maskbuf) + bufsize - 1;

    if (mask != 0) {
	Visual *v = root_visual;
	ird->maskim = XCreateImage(display, v, 1,
				   XYBitmap, 0, ird->cvdata.maskbuf,
				   (unsigned)ird->dstW, (unsigned)ird->dstH,
				   32, scansize);
	if (ird->maskim == 0) {
	    sysFree(ird->cvdata.maskbuf);
	    ird->cvdata.maskbuf = 0;
	} else {
	    static int native_order = (MSBFirst << 24) | LSBFirst;
	    ird->maskim->byte_order = ((char *)&native_order)[0];
	    ird->maskim->bitmap_bit_order = MSBFirst;
	    ird->maskim->bitmap_unit = 32;
	}

	ird->mask = XCreatePixmap(display, root_drawable,
				  (unsigned)ird->dstW, (unsigned)ird->dstH, 1);
	XSetForeground(display, mask_gc, 0);
	XFillRectangle(display, ird->mask, mask_gc,
		       0, 0, (unsigned)ird->dstW, (unsigned)ird->dstH);
	XSetForeground(display, mask_gc, 1);
	XFillRectangle(display, ird->mask, mask_gc,
		       x1, y1, (unsigned)(x2 - x1), (unsigned)(y2 - y1));
	if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
	    XFillRectangle(display, ird->mask, mask_gc,
			   0, 0, (unsigned)ird->dstW, 
			   (unsigned)ird->curlines.num);
	} else {
	    pixrgn = ird->curpixels;
	    if (pixrgn != 0) {
		XSetRegion(display, mask_gc, pixrgn);
		XFillRectangle(display, ird->mask, mask_gc,
			       0, 0, 
			       (unsigned)ird->dstW, (unsigned)ird->dstH);
		XSetClipMask(display, mask_gc, None);
		XDestroyRegion(pixrgn);
		ird->curpixels = 0;
	    }
	}
/* fprintf(stderr, "XGetSubImage %s %d\n", __FILE__, __LINE__); fflush(stderr); */
	XGetSubImage(display, ird->mask,
		     0, 0, (unsigned)ird->dstW, (unsigned)ird->dstH,
		     (unsigned)~0, ZPixmap, ird->maskim, 0, 0);
    }
    NATIVE_UNLOCK(env);
    return mask;
}

int
image_BufAlloc(JNIEnv *env, IRData *ird)
{
    int w = ird->dstW;
    int h = ird->dstH;
    int bpp, slp, bpsl;
    int bufsize;
    int mask = ird->mask;

    NATIVE_LOCK(env);
    if (w >= 0 && h >= 0) {
	image_FreeBufs(ird);
	bpp = awtImage->clrdata.bitsperpixel;
	slp = awtImage->wsImageFormat.scanline_pad;
	bpsl = paddedwidth(w * bpp, slp) >> 3;
	bufsize = bpsl * h + 1;
	assert(bpp >= 8);
	if (((bpsl / (bpp >> 3)) < w) || (bufsize / bpsl < h)) {
	    /* calculations must have overflown */
	    NATIVE_UNLOCK(env);
	    return 0;
	}
	ird->cvdata.outbuf = (void *) calloc(1, bufsize);
	ird->cvdata.outbuf_end = ((char *)ird->cvdata.outbuf) + bufsize - 1;

	if (ird->cvdata.outbuf != 0) {
	    Visual *v;
	    v = root_visual;
	    ird->xim = XCreateImage(display, v,
				    (unsigned)awtImage->Depth, ZPixmap, 0,
				    (char *) ird->cvdata.outbuf,
				    (unsigned)w, (unsigned)h, 
				    slp, bpsl);
	    if (ird->xim != 0 && mask) {
		image_InitMask(env, ird, 0, 0, 0, 0);
	    }
	}
	if (ird->cvdata.outbuf == 0 || ird->xim == 0
	    || ((mask != 0) && ((ird->cvdata.maskbuf == 0)
				|| (ird->maskim == 0))))
	{
	    image_FreeBufs(ird);
	    NATIVE_UNLOCK(env);
	    return 0;
	}
	ird->xim->bits_per_pixel = bpp;
    }

    NATIVE_UNLOCK(env);
    return 1;
}


/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    offscreenInit
 * Signature: (Ljava/awt/Color;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_image_ImageRepresentation_offscreenInit(JNIEnv *env,
						     jobject obj,
						     jobject backgroundColor)
{
    IRData *ird;
    int w, h;

    if (obj == 0) {
	NullPointerException(env, "No memory for image");
	return;
    }

    /*  AWT_LOCK(); */
    w = (*env)->GetIntField(env, obj, IRCachedIDs.widthFID);
    h = (*env)->GetIntField(env, obj, IRCachedIDs.heightFID);

    if ((w <= 0) || (h <= 0)) {
	fprintf(stderr, "Illegal size for offscreen memory region\n");
	IllegalArgumentException(env, "Size cannot be negative");
	/*  AWT_UNLOCK(); */
	return;
    }
    (*env)->SetIntField(env, obj, IRCachedIDs.availinfoFID, IMAGE_OFFSCRNINFO);

    ird = image_getIRData(env, obj, backgroundColor, 0);
    if (ird == 0) {
	OutOfMemoryError(env, "Offscreen image private data");
        /*  AWT_UNLOCK(); */
	return;
    }
    /*  AWT_UNLOCK(); */
}

/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    setBytePixels
 * Signature: (IIIILjava/awt/image/ColorModel;[BII)Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_image_ImageRepresentation_setBytePixels(JNIEnv *env,
						     jobject obj,
						     jint x, jint y, 
						     jint w, jint h,
						     jobject colorModel, 
						     jbyteArray pixels, 
						     jint off, 
						     jint scansize)
{
    IRData        *ird;
    ImgCMData     *icmd;
    int           flags;
    int           ret;
    ImgConvertFcn *cvfcn;
    int           arrayLen;
    jbyte         *pixelData;
    int           srcW, srcH;

    if ((obj == NULL) || (colorModel == 0) || (pixels == 0)) {
	NullPointerException(env, "");
        return (jboolean)(~0);
    }

    srcW = (*env)->GetIntField(env, obj, IRCachedIDs.srcWFID);
    srcH = (*env)->GetIntField(env, obj, IRCachedIDs.srcHFID);

    if (x < 0 || y < 0 || w < 0 || h < 0
	|| scansize < 0 || off < 0
	|| x + w > srcW || y+h > srcH)
    {
	ArrayIndexOutOfBoundsException(env, "");
        /*  SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0); */
	fprintf(stderr, "Trying to write outside image?\n");
        return (jboolean)(~0);
    }
    if (w == 0 || h == 0) {
	return 0;
    }
    arrayLen = (*env)->GetArrayLength(env, pixels);
    if ((arrayLen < (unsigned long) (off + w))
	|| (   (scansize != 0) 
	    && (((arrayLen - w - off) / scansize) < (h - 1)))) {
	ArrayIndexOutOfBoundsException(env, "");
        return (jboolean)(~0);
    }
    /*  AWT_LOCK(); */
    NATIVE_LOCK(env);
    ird = image_getIRData(env, obj, NULL, 0);
    if (ird == 0) {
        OutOfMemoryError(env, "");
	/*  AWT_UNLOCK(); */
	NATIVE_UNLOCK(env);
        return (jboolean)(~0);
    }
    if (ird->cvdata.outbuf == 0) {
	image_BufAlloc(env, ird);
	if (ird->cvdata.outbuf == 0) {
	    OutOfMemoryError(env, "");
	    /*  AWT_UNLOCK(); */
	    NATIVE_UNLOCK(env);
	    return (jboolean)(~0);
	}
    }
    icmd = (ImgCMData*) img_getCMData(env, colorModel);
    if (icmd == 0) {
	/*  AWT_UNLOCK(); */
	NATIVE_UNLOCK(env);
        return (jboolean)(~0);
    }

    flags = (((srcW == ird->dstW) && (srcH == ird->dstH)) ? IMGCV_UNSCALED : IMGCV_SCALED)
	| (IMGCV_BYTEIN)
	| (((ird->hints & HINTS_DITHERFLAGS) != 0) ? IMGCV_TDLRORDER : IMGCV_RANDORDER)
	| icmd->type;

    if (ird->cvdata.maskbuf) {
	flags |= IMGCV_ALPHA;
    }
    cvfcn = awtImage->convert[flags];

    pixelData = (*env)->GetByteArrayElements(env, pixels, 0);
#ifdef CVFUNCTRACKING
    if (flags != prevflags && cvfcn != prevfunc && getenv("IMGCV_VERBOSE")) {
	Dl_info dlinfo;
	prevflags = flags;
	prevfunc = cvfcn;
	ret = dladdr((void *) cvfcn, &dlinfo);
	if (ret) {
	    fprintf(stderr, "now using %s for conversion\n", dlinfo.dli_sname);
	} else {
	    fprintf(stderr, "couldn't get cvfcn info\n");
	}

	fprintf(stderr, "cvfcn(env = 0x%08x, cm = 0x%08x,\n",
		(unsigned int)env, (unsigned int)colorModel);

	fprintf(stderr, "      x = %d, y = %d, w = %d, h = %d,\n",
		(int)x, (int)y, (int)w, (int)h);

	fprintf(stderr, "      pixelData = 0x%08x, off = %d, 8, scansize = %d,\n", 
		(unsigned int)pixelData, 
		(int)off, (int)scansize);

	fprintf(stderr, "      srcW = %d, srcH = %d, dstW = %d, dstH = %d,\n",
		srcW, srcH, ird->dstW, ird->dstH);

	fprintf(stderr, "      cvdata = 0x%08x, clrdata = 0x%08x)\n",
		(unsigned int)&ird->cvdata, (unsigned int) &awtImage->clrdata);
	fflush(stderr);
    }
#endif
    
    ret = cvfcn(env, obj, colorModel, x, y, w, h,
		pixelData, off, 8, scansize,
		srcW, srcH, ird->dstW, ird->dstH,
		&ird->cvdata, &awtImage->clrdata);
    (*env)->ReleaseByteArrayElements(env, pixels, pixelData, JNI_ABORT);
    NATIVE_UNLOCK(env);
    /*  AWT_UNLOCK(); */
    return (ret == SCALESUCCESS);
}

/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    setIntPixels
 * Signature: (IIIILjava/awt/image/ColorModel;[III)Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_image_ImageRepresentation_setIntPixels(JNIEnv *env,
						    jobject obj,
						    jint x, jint y, 
						    jint w, jint h, 
						    jobject colorModel, 
						    jintArray pixels, 
						    jint off, 
						    jint scansize)
{
    IRData        *ird;
    ImgCMData     *icmd;
    int           flags;
    int           ret;
    ImgConvertFcn *cvfcn;
    int           arrayLen;
    jint          *pixelData;
    jint          srcW, srcH;

    if ((obj == NULL) || (colorModel == 0) || (pixels == 0)) {
        NullPointerException(env, "");
        return (jboolean)(~0);
    }

    srcW = (*env)->GetIntField(env, obj, IRCachedIDs.srcWFID);
    srcH = (*env)->GetIntField(env, obj, IRCachedIDs.srcHFID);

    if (x < 0 || y < 0 || w < 0 || h < 0
	|| scansize < 0 || off < 0
	|| x + w > srcW || y+h > srcH)
    {
        ArrayIndexOutOfBoundsException(env, "");
	fprintf(stderr, "Trying to write outside image?\n");
        return (jboolean)(~0);
    }
    if (w == 0 || h == 0) {
	return 0;
    }
    arrayLen = (*env)->GetArrayLength(env, pixels);
    if (   (arrayLen < (unsigned long) (off + w))
	|| (   (scansize != 0) 
	    && (((arrayLen - w - off) / scansize) < (h - 1)))) {
	ArrayIndexOutOfBoundsException(env, "");
        return (jboolean)(~0);
    }
    /*  AWT_LOCK(); */
    NATIVE_LOCK(env);
    ird = image_getIRData(env, obj, NULL, 0);
    if (ird == 0) {
        OutOfMemoryError(env, "");
	/*  AWT_UNLOCK(); */
	NATIVE_UNLOCK(env);
        return (jboolean)(~0);
    }
    if (ird->cvdata.outbuf == 0) {
	image_BufAlloc(env, ird);
	if (ird->cvdata.outbuf == 0) {
	    OutOfMemoryError(env, "");
	    /*  AWT_UNLOCK(); */
	    NATIVE_UNLOCK(env);
	    return (jboolean)(~0);
	}
    }
    icmd = (ImgCMData*) img_getCMData(env, colorModel);
    if (icmd == 0) {
	/*  AWT_UNLOCK(); */
	NATIVE_UNLOCK(env);
        return (jboolean)(~0);
    }
    flags = (((srcW == ird->dstW) && (srcH == ird->dstH)) ? IMGCV_UNSCALED : IMGCV_SCALED)
	| (IMGCV_INTIN)
	| (((ird->hints & HINTS_DITHERFLAGS) != 0) ? IMGCV_TDLRORDER : IMGCV_RANDORDER)
	| icmd->type;
    if (ird->cvdata.maskbuf) {
	flags |= IMGCV_ALPHA;
    }
    cvfcn = awtImage->convert[flags];
#ifdef CVFUNCTRACKING
    if (flags != prevflags && cvfcn != prevfunc && getenv("IMGCV_VERBOSE")) {
	Dl_info dlinfo;
	prevflags = flags;
	prevfunc = cvfcn;
	ret = dladdr((void *) cvfcn, &dlinfo);
	if (ret) {
	    fprintf(stderr, "now using %s for conversion\n", dlinfo.dli_sname);
	}
    }
#endif
    pixelData = (*env)->GetIntArrayElements(env, pixels, 0);
    ret = cvfcn(env, obj, colorModel, x, y, w, h,
		pixelData, off, 32, scansize,
		srcW, srcH, ird->dstW, ird->dstH,
		&ird->cvdata, &awtImage->clrdata);
    (*env)->ReleaseIntArrayElements(env, pixels, pixelData, JNI_ABORT);
    NATIVE_UNLOCK(env);
    /*  AWT_UNLOCK(); */
    return (ret == SCALESUCCESS);
}

/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    finish
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_image_ImageRepresentation_finish(JNIEnv *env,
					      jobject obj,
					      jboolean force)
{
    IRData *ird;
    int ret = 0;

    if (obj == 0) {
	NullPointerException(env, "");
	return 0;
    }
    /*  AWT_LOCK(); */
    NATIVE_LOCK(env);
    ird = image_getIRData(env, obj, NULL, 0);
    if (ird != 0) {
	ret = (!force
	       && ird->depth <= 8
	       && (ird->hints & HINTS_DITHERFLAGS) == 0);
	if (ird->mask == 0 &&
	    (((ird->hints & HINTS_SCANLINEFLAGS) != 0)
	     ? (ird->curlines.num < ird->dstH)
	     : (ird->curpixels != 0)))
	{
	    /*
	     * Now that all of the pixels are claimed to have been
	     * delivered, if we have any remaining non-delivered
	     * pixels, they are now considered transparent.  Thus
	     * we construct a mask.
	     */
	    image_InitMask(env, ird, 0, 0, 0, 0);
	}
	image_FreeRenderData(ird);
	ird->hints = 0;
	ird->curlines.num = ird->dstH;
    }
    /*  AWT_UNLOCK(); */
    NATIVE_UNLOCK(env);
    return ret;
}

/*  ImageDraw and ImageStretch live in GraphicsImpl.c */

/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    disposeImage
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_image_ImageRepresentation_disposeImage(JNIEnv *env,
						    jobject obj)
{
    IRData *ird;

    if (obj == 0) {
	NullPointerException(env, "");
	return;
    }
    /*  AWT_LOCK(); */
    NATIVE_LOCK(env);

    ird = (IRData *)(*env)->GetIntField(env, obj, IRCachedIDs.pDataFID);

    if (ird != 0) {
	if (ird->pixmap != None) {
	    XFreePixmap(display, ird->pixmap);
	}
	if (ird->mask != None) {
	    XFreePixmap(display, ird->mask);
	}
	image_FreeRenderData(ird);
	image_FreeBufs(ird);

	free(ird);

        (*env)->SetIntField(env, obj, IRCachedIDs.pDataFID, (jint)0);
    }

    NATIVE_UNLOCK(env);
    /*  AWT_UNLOCK(); */
}

int
gfX_imageDraw(Drawable win, GC gc, JNIEnv *env, jobject obj, Region clip,
	      jobject xorColor, jobject fgColor,
	      long x, long y,
	      long bx, long by, long bw, long bh,
	      jobject bgColor)
{
    IRData		*ird;
    long		wx;
    long		wy;
    long		ix;
    long		iy;
    long		iw;
    long		ih;
    long		diff;
    Region		pixrgn = 0;
    Pixmap              cp = 0;
    jint                availinfo;

    if (obj == 0) {
	NullPointerException(env, "");
	return 0;
    }

    availinfo = (*env)->GetIntField(env, obj, IRCachedIDs.availinfoFID);

    if ((availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
	return 1;
    }

    ird = image_getIRData(env, obj, NULL, 0);
    if (ird == 0) {
	OutOfMemoryError(env, "");
	return 0;
    } else {
	if (ird->pixmap == None) {
	    return 1;
	}
	if (win == 0) {
	    NullPointerException(env, "win");
	    return 0;
	}

	/* could have been changed by image_getIRdata, I think... */
        availinfo = (*env)->GetIntField(env, obj, IRCachedIDs.availinfoFID);

	if ((availinfo & IMAGE_FULLDRAWINFO) != 0) {
	    ix = iy = 0;

            iw = (*env)->GetIntField(env, obj, IRCachedIDs.widthFID);
            ih = (*env)->GetIntField(env, obj, IRCachedIDs.heightFID);

	} else if ((ird->hints & HINTS_SCANLINEFLAGS) != 0 ||
		   ird->mask) {
	    ix = iy = 0;

            iw = (*env)->GetIntField(env, obj, IRCachedIDs.widthFID);

	    ih = ird->curlines.num;
	} else {
	    XRectangle rect;
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

	if (iw <= 0 || ih <= 0) {
	    return 1;
	}

	NATIVE_LOCK(env);
	if (ird->mask) {
	    if (bgColor == 0) {
		GC mask_gc;
		XGCValues xgcv;
		xgcv.background = 0;
		xgcv.foreground = 1;
		
	        /* we have to merge the clip region with the image's mask */
	        cp = XCreatePixmap(display, root_drawable, 
				   (unsigned)ird->dstW, 
				   (unsigned)ird->dstH, 1);
		mask_gc = XCreateGC(display, cp,
				    GCForeground | GCBackground, &xgcv);
		XSetFunction(display, mask_gc, GXcopy);
		XSetForeground(display, mask_gc, 0);
		XFillRectangle(display, cp, mask_gc,
			       0, 0, 
			       (unsigned)ird->dstW, (unsigned)ird->dstH);
		
		XOffsetRegion(clip, bx-x, by-y); /*  make up for XSetClipOrigin */
		XSetRegion(display, mask_gc, clip);
		XCopyArea(display, ird->mask, cp, mask_gc,
			  0, 0, 
			  (unsigned)ird->dstW, (unsigned)ird->dstH, 0, 0);
		XSetClipMask(display, gc, cp);
		XOffsetRegion(clip, x-bx, y-by); /*  undo the offset */

		XFreeGC(display, mask_gc);
	    } else {
		unsigned long bgcolor = gfXGetPixel(env, bgColor, xorColor, TRUE);

		if (ird->bgcolor != bgcolor + 1) {
		    /* Set the "transparent pixels" in the pixmap to
		     * the specified solid background color (harmless
		     * since they're never painted otherwise) and just
		     * do an unmasked copy into place below.
		     */
		    /* NOTE: I'm not convinced this is harmless.  The change */
		    /* will be noticeable by applications which grab pixels  */
		    /* from the image!  rib 3/9/98   Perhaps better to paint */
		    /* the background color into the destination rather than */
		    /* into the source? */

		    GC imagegc = getImageGC(ird->pixmap);

		    /* invert the sense of the mask by XOR with white */
		    XSetFunction(display, mask_gc, GXxor);
		    XFillRectangle(display, ird->mask, mask_gc,
				   0, 0, 
				   (unsigned)ird->dstW, (unsigned)ird->dstH);

		    /* now use the inverted mask as a clip */
		    XSetClipMask(display, imagegc, ird->mask);
		    XSetForeground(display, imagegc, bgcolor);
		    XFillRectangle(display, ird->pixmap, imagegc,
				   0, 0, 
				   (unsigned)ird->dstW, (unsigned)ird->dstH);
		    XSetForeground(display, imagegc, white_pixel);

		    /* finally, reinvert the mask */
		    XSetClipMask(display, imagegc, None);
		    XFillRectangle(display, ird->mask, mask_gc,
				   0, 0, 
				   (unsigned)ird->dstW, (unsigned)ird->dstH);
		    XSetFunction(display, mask_gc, GXcopy);

		    ird->bgcolor = bgcolor + 1;
		}
	    }
	} else if (pixrgn) {
	    Region tmp = XCreateRegion();
	    XOffsetRegion(clip, bx-x, by-y); /*  make up for XSetClipOrigin */
	    XIntersectRegion(clip, pixrgn, tmp);
	    XOffsetRegion(clip, x-bx, y-by); /*  undo the offset */
	    XSetRegion(display, gc, tmp);
	    XDestroyRegion(tmp);
	}

	if ((ird->mask && (bgColor == NULL)) || pixrgn) {
	    XSetClipOrigin(display, gc, x-bx, y-by);
	}
	if (xorColor != NULL) {
	    XSetForeground(display, gc, gfXGetPixel(env, xorColor, NULL, TRUE));
	    XFillRectangle(display, win, gc, wx, wy, 
			   (unsigned)iw, (unsigned)ih);
	}
	XCopyArea(display,
		  ird->pixmap, win, gc,
		  ix, iy, (unsigned)iw, (unsigned)ih, wx, wy);
	if (xorColor != NULL) {
	    XSetForeground(display, gc, gfXGetPixel(env, fgColor, xorColor, TRUE));
	}
	if ((ird->mask && (bgColor == NULL)) || pixrgn) {
	  XSetRegion(display, gc, clip);
	}
    }

    if (cp != 0) XFreePixmap(display, cp);
    NATIVE_UNLOCK(env);
    return 1;
}

static XImage *savedXImage;

#ifdef MITSHM
extern int XShmQueryExtension();

XShmSegmentInfo *
getSharedSegment(int image_size)
{
    XShmSegmentInfo *shminfo;

    assert(NATIVE_LOCK_HELD());
    if (!XShmQueryExtension(display)) {
	return 0;
    }
    shminfo = malloc(sizeof *shminfo);
    if (shminfo == 0) {
	return 0;
    }
    shminfo->shmid = shmget(IPC_PRIVATE, image_size, IPC_CREAT|0777);
    if (shminfo->shmid < 0) {
	free(shminfo);
	return 0;
    }
    shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
    if (shminfo->shmaddr == ((char *) -1)) {
	free(shminfo);
	return 0;
    }
    shminfo->readOnly = True;
    XShmAttach (display, shminfo);
    return shminfo;
}

#define DoPutImage(dsp, win, gc, img, sx, sy, dx, dy, w, h)		\
    do {								\
	if (img->obdata != 0) {						\
	    XShmPutImage(dsp, win, gc, img,				\
			 sx, sy, dx, dy, (unsigned)w, (unsigned)h, False); \
	} else if (img->bits_per_pixel ==				\
		   awtImage->wsImageFormat.bits_per_pixel) {		\
	    XPutImage(dsp, win, gc, img, sx, sy, dx, dy, (unsigned)w, (unsigned)h); \
	} else {							\
	    PutAndReformatImage(dsp, win, gc, img,			\
				sx, sy, dx, dy, w, h);			\
	}								\
    } while (0)

#else /* MITSHM */

#define DoPutImage(dsp, win, gc, img, sx, sy, dx, dy, w, h)		\
    do {								\
	if (img->bits_per_pixel ==					\
	    awtImage->wsImageFormat.bits_per_pixel)			\
	{								\
	    XPutImage(dsp, win, gc, img, sx, sy, dx, dy, (unsigned)w, (unsigned)h);		\
	} else {							\
	    PutAndReformatImage(dsp, win, gc, img,			\
				sx, sy, dx, dy, w, h);			\
	}								\
    } while (0)

#endif /* MITSHM */

void
dropImageBuf(XImage *img)
{
    if (img != savedXImage) {
#ifdef MITSHM
	XShmSegmentInfo *shminfo = (XShmSegmentInfo *) img->obdata;

	assert(NATIVE_LOCK_HELD());
	if (shminfo != 0) {
	    XShmDetach(display, shminfo);
	    shmdt(shminfo->shmaddr);
	    shmctl(shminfo->shmid, IPC_RMID, 0);
	    sysFree(shminfo);
	    XFree(img);
	    return;
	}
#endif /* MITSHM */
	XDestroyImage(img);
    }
}

XImage *
getImageBuf(int depth, int bpp, int width, int height)
{
    XImage *img;
    int slp, bpsl;
    int image_size;
    int samebpp = (bpp == awtImage->wsImageFormat.bits_per_pixel);
#ifdef MITSHM
    XShmSegmentInfo *shminfo;
#endif /* MITSHM */

    assert(NATIVE_LOCK_HELD());
    if (savedXImage != 0 && samebpp) {
	if (width <= savedXImage->width) {
	    width = savedXImage->width;
	    if (height <= savedXImage->height) {
#ifdef MITSHM
		/* Sync so that previous data gets flushed. */
		XSync(display, False);
#endif /* MITSHM */
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
    img = XCreateImage(display, root_visual,
		       (unsigned)depth, ZPixmap, 0,
		       0, (unsigned)width, (unsigned)height, 32, bpsl);
    if (img == 0) {
	return (XImage *) 0;
    }
    img->bits_per_pixel = bpp;
    image_size = height * img->bytes_per_line;
    if (image_size / height != img->bytes_per_line) {
	/* Overflow */
	XFree(img);
	return (XImage *) 0;
    }
#ifdef MITSHM
    if (samebpp) {
	shminfo = getSharedSegment(image_size);
    } else {
	shminfo = 0;
    }
    if (shminfo == 0) {
	img->obdata = (char *) 0;
	img->data = malloc(image_size);
    } else {
	img->obdata = (char *) shminfo;
	img->data = shminfo->shmaddr;
    }
#else /* MITSHM */
    img->data = malloc((unsigned)image_size);
#endif /* MITSHM */
    if (img->data == 0) {
	XFree(img);
	return (XImage *) 0;
    }
    if (image_size < (1 << 20) && samebpp) {
	XImage *tmp = savedXImage;
	savedXImage = img;
	if (tmp != 0) {
	    dropImageBuf(tmp);
	}
    }
    return img;
}

void
PutAndReformatImage(Display *dsp, Drawable win, GC gc, XImage *img,
		    int sx, int sy, int dx, int dy, int w, int h)
{
    XImage *xim2;
    int i, j, off, adjust;

    assert(NATIVE_LOCK_HELD());
    xim2 = getImageBuf(awtImage->Depth,
		       awtImage->wsImageFormat.bits_per_pixel,
		       w, h);
    if (xim2 == 0) {
	return;
    }

    off = sy * img->bytes_per_line;
    if (img->bits_per_pixel == 8) {
	u_char *p = (u_char *) img->data;
	p += off + sx;
	adjust = img->bytes_per_line - w;
	for (j = 0; j < h; j++) {
	    for (i = 0; i < w; i++) {
		XPutPixel(xim2, i, j, *p++);
	    }
	    p += adjust;
	}
    } else if (img->bits_per_pixel == 16) {
	u_short *p = (u_short *) img->data;
	p += (off / 2) + sx;
	adjust = (img->bytes_per_line / 2) - w;
	for (j = 0; j < h; j++) {
	    for (i = 0; i < w; i++) {
		XPutPixel(xim2, i, j, *p++);
	    }
	    p += adjust;
	}
    } else {
	u_long *p = (u_long *) img->data;
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

int
image_Done(JNIEnv *env, jobject hJavaObj, IRData *ird, int x1, int y1, int x2,
	   int y2)
{
    int bpp;
    int w = x2 - x1;
    int h = y2 - y1;
    int ytop = y1;
    GC imagegc;
    jobject nb;

    if (ird->pixmap == None || ird->xim == 0) {
	return 0;
    }
    bpp = awtImage->wsImageFormat.bits_per_pixel;

    NATIVE_LOCK(env);
    imagegc = getImageGC(ird->pixmap);
    if (ird->xim->bits_per_pixel == bpp) {
	XPutImage(display, ird->pixmap, imagegc,
		  ird->xim, x1, y1, x1, y1, (unsigned)w, (unsigned)h);
    } else {
	PutAndReformatImage(display, ird->pixmap, imagegc,
			    ird->xim, x1, y1, x1, y1, 
			    w, h);
    }
    if (ird->mask) {
	XPutImage(display, ird->mask, mask_gc,
		  ird->maskim, x1, y1, x1, y1, 
		  (unsigned)w, (unsigned)h);
	/* We just invalidated any background color preparation... */
	ird->bgcolor = 0;
    }
    if ((ird->hints & HINTS_SCANLINEFLAGS) != 0) {
	char *seen = ird->curlines.seen;
	int l;
	if (seen == 0) {
	    seen = calloc(1, (unsigned)ird->dstH);
	    if (seen == NULL) {
		/* OutOfMemoryError? */
		fprintf(stderr, "calloc failed at line %d file %s\n",
			__LINE__, __FILE__);
	    }

	    ird->curlines.seen = seen;
	}
	for (l = y1 - 1; l >= 0; l--) {
	    if (seen[l]) {
		break;
	    }
	    memcpy(ird->xim->data + y1 * ird->xim->bytes_per_line,
		   ird->xim->data + l * ird->xim->bytes_per_line,
		   (unsigned)ird->xim->bytes_per_line);
	    XCopyArea(display, ird->pixmap, ird->pixmap, imagegc,
		      x1, y1, (unsigned)w, 1, x1, l);
	    if (ird->mask) {
		memcpy(ird->maskim->data + y1 * ird->maskim->bytes_per_line,
		       ird->maskim->data + l * ird->maskim->bytes_per_line,
		       (unsigned)ird->maskim->bytes_per_line);
		XCopyArea(display, ird->mask, ird->mask, mask_gc,
			  x1, y1, (unsigned)w, 1, x1, l);
	    }
	    ytop = l;
	}
	for (l = y1; l < y2; l++) {
	    seen[l] = 1;
	}
    } else if (ird->mask == 0) {
	XRectangle rect;
	Region curpixels = ird->curpixels;

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
    NATIVE_UNLOCK(env);
    if (ird->curlines.num < y2) {
	ird->curlines.num = y2;
    }

    nb = (*env)->GetObjectField(env, hJavaObj, IRCachedIDs.newbitsFID);


    if (nb != 0) {
        (*env)->SetIntField(env, nb, IRCachedIDs.java_awt_Rectangle_xFID, (jint)x1);
        (*env)->SetIntField(env, nb, IRCachedIDs.java_awt_Rectangle_yFID, (jint)ytop);
        (*env)->SetIntField(env, nb, IRCachedIDs.java_awt_Rectangle_widthFID, (jint)w);
        (*env)->SetIntField(env, nb, IRCachedIDs.java_awt_Rectangle_heightFID, (jint)(y2-ytop));

    }
    return 1;
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


#define DECLARE_CLIPRECT_VARS    \
    Region clipRectList = NULL;

#define SAVED_CLIPRECTS() (clipRectList != NULL)

#define SET_CLIPRECTS(dsp, gc, clip)			    \
    do {						    \
	XIntersectRegion(clipRectList, clip, clipRectList); \
        XSetRegion(dsp, gc, clipRectList);		    \
	XDestroyRegion(clipRectList);			    \
    } while (0)

#define ADD_CLIPRECT(dx, dy, w, h)                                  \
    do {                                                            \
        XRectangle tmpRect;                                         \
        if (clipRectList == NULL) {                                 \
	    clipRectList = XCreateRegion();                         \
	}                                                           \
	tmpRect.x      = dx;                                        \
	tmpRect.y      = dy;                                        \
	tmpRect.width  = w;                                         \
	tmpRect.height = h;                                         \
	XUnionRectWithRegion(&tmpRect, clipRectList, clipRectList); \
    } while (0)

void
ScaleBytesOpaque(XImage *simg, XImage *dimg,
		 long sx, long sy, long sw, long sh, long dw, long dh,
		 long dx1, long dy1, long dx2, long dy2)
{
    long x, y;
    long srow, scol;
    long prevrow = -1;
    unsigned char *ps;
    unsigned char *pd = (unsigned char *) dimg->data;
    long bpsl = dimg->bytes_per_line;
    DECLARE_XSCALE_VARS

    INIT_XSCALE(dx1, sw, dw, sx);
    for (y = dy1; y < dy2; y++, pd += bpsl) {
	CALC_SROW(srow, sy, y, sh, dh);
	if (srow == prevrow) {
	    memcpy(pd, pd - bpsl, (unsigned)bpsl);
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
ScaleShortsOpaque(XImage *simg, XImage *dimg,
		  long sx, long sy, long sw, long sh, long dw, long dh,
		  long dx1, long dy1, long dx2, long dy2)
{
    long x, y;
    long srow, scol;
    long prevrow = -1;
    unsigned short *ps;
    unsigned short *pd = (unsigned short *) dimg->data;
    long spsl = (dimg->bytes_per_line >> 1);
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
    INIT_XSCALE(dx1, sw, dw, sx);
    for (y = dy1; y < dy2; y++, pd += spsl) {
	CALC_SROW(srow, sy, y, sh, dh);
	if (srow == prevrow) {
	    memcpy(pd, pd - spsl, (unsigned)spsl << 1);
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
ScaleIntsOpaque(XImage *simg, XImage *dimg,
		long sx, long sy, long sw, long sh, long dw, long dh,
		long dx1, long dy1, long dx2, long dy2)
{
    long x, y;
    long srow, scol;
    long prevrow = -1;
    unsigned int *ps;
    unsigned int *pd = (unsigned int *) dimg->data;
    long ipsl = (dimg->bytes_per_line >> 2);
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
    INIT_XSCALE(dx1, sw, dw, sx);
    for (y = dy1; y < dy2; y++, pd += ipsl) {
	CALC_SROW(srow, sy, y, sh, dh);
	if (srow == prevrow) {
	    memcpy(pd, pd - ipsl, (unsigned)ipsl << 2);
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
ScaleBytesMask(XImage *simg, XImage *dimg, XImage *smsk,
	       Drawable win, GC gc, Region clip,
	       long sx, long sy, long sw, long sh,
	       long dx, long dy, long dw, long dh,
	       long dx1, long dy1, long dx2, long dy2)
{
    long x, y, x1;
    long srow, scol;
    unsigned char *ps;
    unsigned char *pd = (unsigned char *) dimg->data;
    unsigned int *pm;
    long bpsl = dimg->bytes_per_line;
    DECLARE_CLIPRECT_VARS
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
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
		    ADD_CLIPRECT(dx+x1, dy+y, x-x1, 1);
		}
		x1 = -1;
	    }
	    pd++;
	    NEXT_SCOL(scol, dw);
	}
	if (x1 >= 0) {
	    /* There is always room for one more... */
	    ADD_CLIPRECT(dx+x1, dy+y, x-x1, 1);
	}
    }
    if (SAVED_CLIPRECTS() > 0) {
	SET_CLIPRECTS(display, gc, clip);
	DoPutImage(display, win, gc, dimg,
		   0, 0, dx, dy, 
		   (dx2 - dx1), 
		   (dy2 - dy1));
    }

    XSetClipMask(display, gc, None);
}

void
ScaleShortsMask(XImage *simg, XImage *dimg, XImage *smsk,
		Drawable win, GC gc, Region clip,
		long sx, long sy, long sw, long sh,
		long dx, long dy, long dw, long dh,
		long dx1, long dy1, long dx2, long dy2)
{
    long x, y, x1;
    long srow, scol;
    unsigned short *ps;
    unsigned short *pd = (unsigned short *) dimg->data;
    unsigned int *pm;
    long spsl = (dimg->bytes_per_line >> 1);
    DECLARE_CLIPRECT_VARS
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
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
		    ADD_CLIPRECT(dx+x1, dy+y, x-x1, 1);
		}
		x1 = -1;
	    }
	    pd++;
	    NEXT_SCOL(scol, dw);
	}
	if (x1 >= 0) {
	    /* There is always room for one more... */
	    ADD_CLIPRECT(dx+x1, dy+y, x-x1, 1);
	}
    }

    if (SAVED_CLIPRECTS() > 0) {
	SET_CLIPRECTS(display, gc, clip);
	DoPutImage(display, win, gc, dimg,
		   0, 0, dx, dy, 
		   (dx2 - dx1), 
		   (dy2 - dy1));
    }

    XSetClipMask(display, gc, None);
}

void
ScaleIntsMask(XImage *simg, XImage *dimg, XImage *smsk,
	      Drawable win, GC gc, Region clip,
	      long sx, long sy, long sw, long sh,
	      long dx, long dy, long dw, long dh,
	      long dx1, long dy1, long dx2, long dy2)
{
    long x, y, x1;
    long srow, scol;
    unsigned int *ps;
    unsigned int *pd = (unsigned int *) dimg->data;
    unsigned int *pm;
    long ipsl = (dimg->bytes_per_line >> 2);
    DECLARE_CLIPRECT_VARS
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
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
		    ADD_CLIPRECT(dx+x1, dy+y, x-x1, 1);
		}
		x1 = -1;
	    }
	    pd++;
	    NEXT_SCOL(scol, dw);
	}
	if (x1 >= 0) {
	    /* There is always room for one more... */
	    ADD_CLIPRECT(dx+x1, dy+y, x-x1, 1);
	}
    }

    if (SAVED_CLIPRECTS() > 0) {
	SET_CLIPRECTS(display, gc, clip);
	DoPutImage(display, win, gc, dimg,
		   0, 0, dx, dy, 
		   (dx2 - dx1), (dy2 - dy1));
    }

    XSetClipMask(display, gc, None);
}

void
ScaleBytesMaskBG(XImage *simg, XImage *dimg, XImage *smsk,
		 long sx, long sy, long sw, long sh, long dw, long dh,
		 long dx1, long dy1, long dx2, long dy2,
		 unsigned int bgcolor)
{
    unsigned int pixel;
    long x, y;
    long srow, scol;
    long prevrow = -1;
    unsigned char *ps;
    unsigned char *pd = (unsigned char *) dimg->data;
    unsigned int *pm;
    long bpsl = dimg->bytes_per_line;
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
    INIT_XSCALE(dx1, sw, dw, sx);
    for (y = dy1; y < dy2; y++, pd += bpsl) {
	CALC_SROW(srow, sy, y, sh, dh);
	if (srow == prevrow) {
	    memcpy(pd, pd - bpsl, (unsigned)bpsl);
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
ScaleShortsMaskBG(XImage *simg, XImage *dimg, XImage *smsk,
		  long sx, long sy, long sw, long sh, long dw, long dh,
		  long dx1, long dy1, long dx2, long dy2,
		  unsigned int bgcolor)
{
    unsigned int pixel;
    long x, y;
    long srow, scol;
    long prevrow = -1;
    unsigned short *ps;
    unsigned short *pd = (unsigned short *) dimg->data;
    unsigned int *pm;
    long spsl = (dimg->bytes_per_line >> 1);
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
    INIT_XSCALE(dx1, sw, dw, sx);
    for (y = dy1; y < dy2; y++, pd += spsl) {
	CALC_SROW(srow, sy, y, sh, dh);
	if (srow == prevrow) {
	    memcpy(pd, pd - spsl, (unsigned)spsl << 1);
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
ScaleIntsMaskBG(XImage *simg, XImage *dimg, XImage *smsk,
		long sx, long sy, long sw, long sh, long dw, long dh,
		long dx1, long dy1, long dx2, long dy2,
		unsigned int bgcolor)
{
    unsigned int pixel;
    long x, y;
    long srow, scol;
    long prevrow = -1;
    unsigned int *ps;
    unsigned int *pd = (unsigned int *) dimg->data;
    unsigned int *pm;
    long ipsl = (dimg->bytes_per_line >> 2);
    DECLARE_XSCALE_VARS

    assert(NATIVE_LOCK_HELD());
    INIT_XSCALE(dx1, sw, dw, sx);
    for (y = dy1; y < dy2; y++, pd += ipsl) {
	CALC_SROW(srow, sy, y, sh, dh);
	if (srow == prevrow) {
	    memcpy(pd, pd - ipsl, (unsigned)ipsl << 2);
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
	int t = c1;		\
	c1 = c2;		\
	c2 = t;			\
    } while (0)

int
gfX_imageStretch(Drawable win, GC gc, JNIEnv *env, jobject obj, Region clip,
		 jobject xorColor, jobject fgColor,
		 long dx1, long dy1, long dx2, long dy2,
		 long sx1, long sy1, long sx2, long sy2,
		 jobject bgColor)
{
    IRData		*ird;
    long		ix1;
    long		iy1;
    long		ix2;
    long		iy2;
    long		sw;
    long		sh;
    long		dw;
    long		dh;
    long		wx1;
    long		wy1;
    long		wx2;
    long		wy2;
    unsigned int	bgcolor;
    XImage		*dimg;
    XImage		*simg;
    XImage		*smsk;
    jint                availinfo;

    if (obj == 0) {
	NullPointerException(env, "");
	return 0;
    }

    availinfo = (*env)->GetIntField(env, obj, IRCachedIDs.availinfoFID);
    if ((availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
	return 1;
    }
    ird = image_getIRData(env, obj, NULL, 0);
    if (ird == 0) {
	OutOfMemoryError(env, "ImageRep private data");
	return 0;
    } else {
	if (ird->pixmap == None) {
	    return 1;
	}
	if (win == 0) {
	    NullPointerException(env, "win");
	    return 0;
	}

        availinfo = (*env)->GetIntField(env, obj, IRCachedIDs.availinfoFID);

	if ((availinfo & IMAGE_FULLDRAWINFO) != 0) {
	    ix1 = iy1 = 0;

            ix2 = ix1 + (*env)->GetIntField(env, obj, IRCachedIDs.widthFID);
            iy2 = iy1 + (*env)->GetIntField(env, obj, IRCachedIDs.heightFID);

	} else {
	    /* The region will be converted to a mask below... */
	    ix1 = iy1 = 0;

            ix2 = ix1 + (*env)->GetIntField(env, obj, IRCachedIDs.widthFID);
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
	    if (ix1 < sx2) {ix1 = sx2;}
	    if (ix2 > sx1) {ix2 = sx1;}
	    wx2 = dw - DEST_XY_RANGE_START(ix1 - sx2, -sw, dw);
	    wx1 = dw - DEST_XY_RANGE_START(ix2 - sx2, -sw, dw);
	} else {
	    if (ix1 < sx1) {ix1 = sx1;}
	    if (ix2 > sx2) {ix2 = sx2;}
	    wx1 = DEST_XY_RANGE_START(ix1 - sx1, sw, dw);
	    wx2 = DEST_XY_RANGE_START(ix2 - sx1, sw, dw);
	}
	if (sh < 0) {
	    if (iy1 < sy2) {iy1 = sy2;}
	    if (iy2 > sy1) {iy2 = sy1;}
	    wy2 = dh - DEST_XY_RANGE_START(iy1 - sy2, -sh, dh);
	    wy1 = dh - DEST_XY_RANGE_START(iy2 - sy2, -sh, dh);
	} else {
	    if (iy1 < sy1) {iy1 = sy1;}
	    if (iy2 > sy2) {iy2 = sy2;}
	    wy1 = DEST_XY_RANGE_START(iy1 - sy1, sh, dh);
	    wy2 = DEST_XY_RANGE_START(iy2 - sy1, sh, dh);
	}
#ifdef VERBOSE
	jio_fprintf(stderr,
		"blitting: (%4d, %4d) => (%4d, %4d) [%4d x %4d]\n",
		sx1, sy1, sx2, sy2, sw, sh);
	jio_fprintf(stderr,
		"    into: (%4d, %4d) => (%4d, %4d) [%4d x %4d]\n",
		dx1, dy1, dx2, dy2, dw, dh);
	jio_fprintf(stderr,
		"    have: (%4d, %4d) => (%4d, %4d)\n",
		ix1, iy1, ix2, iy2);
	jio_fprintf(stderr,
		"   doing: (%4d, %4d) => (%4d, %4d)\n",
		wx1, wy1, wx2, wy2);
#endif
	if (ix2 <= ix1 || iy2 <= iy1) {
	    return 1;
	}
	if (wx2 <= wx1 || wy2 <= wy1) {
	    return 1;
	}

	NATIVE_LOCK(env);
	dimg = getImageBuf(awtImage->Depth, awtImage->clrdata.bitsperpixel,
			   (wx2 - wx1), (wy2 - wy1));
	if (dimg == 0) {
	    OutOfMemoryError(env, "ImageStretch temporary image");
	    NATIVE_UNLOCK(env);
	    return 0;
	}

	if((*env)->GetBooleanField(env, obj, IRCachedIDs.offscreenFID)) {
	    if (ird->xim == 0) {
		/* how can this happen?? */
		ird->xim = XCreateImage(display, root_visual,
					(unsigned)dimg->depth, ZPixmap, 0,
					0, (unsigned)ird->dstW, 
					(unsigned)ird->dstH, 32, 0);
		if (ird->xim != 0) {
		    if (ird->xim->data != NULL) {
			free(ird->xim->data);
		    }
		    ird->xim->data = malloc((unsigned)(ird->dstH * ird->xim->bytes_per_line));
		    if (ird->xim->data == 0) {
			XFree(ird->xim);
			ird->xim = 0;
		    }
		}
		if (ird->xim == 0) {
		    OutOfMemoryError(env, "Offscreen temporary buffer");
		    dropImageBuf(dimg);
		    NATIVE_UNLOCK(env);
		    return 0;
		}
	    }
/* fprintf(stderr, "XGetSubImage %s %d\n", __FILE__, __LINE__); fflush(stderr); */
	    XGetSubImage(display, ird->pixmap,
			 0, 0, (unsigned)ird->dstW, (unsigned)ird->dstH,
			 (unsigned)~0, ZPixmap, ird->xim, 0, 0);
	}
	simg = ird->xim;
	smsk = ird->maskim;
	if (smsk == 0 && ird->curpixels != 0) {
	    /*
	     * There is no convenient Region readback in Xlib so
	     * we convert to an imagemask instead (this also saves
	     * duplication of effort as we've already created the
	     * mask versions of the functions).
	     */
	    image_InitMask(env, ird, 0, 0, 0, 0);
	    smsk = ird->maskim;
	}

	bgcolor = gfXGetPixel(env, bgColor, NULL, TRUE);
	if (ird->depth == 32) {
	    if (smsk == 0) {
		ScaleIntsOpaque(simg, dimg,
				sx1, sy1, sw, sh, dw, dh,
				wx1, wy1, wx2, wy2);
	    } else if (bgcolor == 0) {
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
	    } else if (bgcolor == 0) {
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
	    } else if (bgcolor == 0) {
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
	    InternalError2(env, "ImageStretch unsupported depth");
	    NATIVE_UNLOCK(env);
	    return 0;
	}
	if (bgColor != 0) {
	    if (wy1 > 0 || wx1 > 0 || wx2 < dw || wy2 < dh) {
	        unsigned long bgpixel = gfXGetPixel(env, bgColor, xorColor, TRUE);
		XSetForeground(display, gc, bgpixel);
	    }
	    if (wy1 > 0) {
		XFillRectangle(display, win, gc,
			       dx1, dy1, (unsigned)dw, (unsigned)wy1);
	    }
	    if (wx1 > 0) {
		XFillRectangle(display, win, gc,
			       dx1, dy1 + wy1, 
			       (unsigned)wx1, (unsigned)(wy2 - wy1));
	    }
	}
	if (smsk == 0 || bgColor != 0) {
	    DoPutImage(display, win, gc, dimg, 0, 0,
		       dx1 + wx1, dy1 + wy1, 
		       (wx2 - wx1), (wy2 - wy1));
	}
	if (bgColor != 0) {
	    if (wx2 < dw) {
		XFillRectangle(display, win, gc,
			       dx1 + wx2, dy1 + wy1, 
			       (unsigned)dw - wx2, (unsigned)wy2 - wy1);
	    }
	    if (wy2 < dh) {
		XFillRectangle(display, win, gc,
			       dx1, dy1 + wy2, 
			       (unsigned)dw, (unsigned)(dh - wy2));
	    }
	    if (wy1 > 0 || wx1 > 0 || wx2 < dw || wy2 < dh) {
	        unsigned long fgpixel = gfXGetPixel(env, fgColor, xorColor, TRUE);
		XSetForeground(display, gc, fgpixel);
	    }
	}
	dropImageBuf(dimg);
    }
    NATIVE_UNLOCK(env);
    return 1;
}


void initOsisClass(JNIEnv *env, jobject obj)
{
/*

    FINDCLASS(sun_awt_image_OffscreenImageSource, 
	      "sun/awt/image/OffScreenImageSource");
    DEFINEFIELD(sun_awt_image_OffscreenImageSource, baseIR,
		"Lsun/awt/image/ImageRepresentation;");
    DEFINEFIELD(sun_awt_image_OffscreenImageSource, theConsumer,
		"Ljava/awt/image/ImageConsumer;");

		*/

}



JNIEXPORT void JNICALL
Java_sun_awt_image_OffScreenImageSource_sendPixels(JNIEnv *env, jobject obj)
{
    IRData *ird;
    int i, w, h, d;
    XImage *xim;
    XID pixmap;

    jobject colorModel;
    jobject ir;
    jobject consumer;

    jbyteArray byteBuf;
    jintArray  intBuf;
    char *buf;
    jint avail;

    ir = (*env)->GetObjectField(env, obj, IRCachedIDs.baseIRFID);

    if (ir == 0) {
        NullPointerException(env, "");
	return;
    }
   
    avail = (*env)->GetIntField(env, ir, IRCachedIDs.availinfoFID);

    if ((avail & IMAGE_OFFSCRNINFO) != IMAGE_OFFSCRNINFO) {
        IllegalAccessError(env, "");
	return;
    }
    /*  AWT_LOCK(); */
    ird = image_getIRData(env, ir, NULL, 0);
    if (ird == 0) {
        OutOfMemoryError(env, "");
        /*  AWT_UNLOCK(); */
	return;
    }
    w = ird->dstW;
    h = ird->dstH;
    d = ird->depth;
    pixmap = ird->pixmap;
    /*  AWT_UNLOCK(); */
    /*  ee = EE(); */

    consumer = (*env)->GetObjectField(env, obj, IRCachedIDs.theConsumerFID);

    if (consumer == 0) {
	return;
    }

    colorModel = getColorModel(env);

    (*env)->CallVoidMethod(env, consumer, IRCachedIDs.setColorModelMID, colorModel);
    (*env)->CallVoidMethod(env, consumer, IRCachedIDs.setHintsMID, HINTS_OFFSCREENSEND);

    /*  AWT_LOCK(); */
    NATIVE_LOCK(env);
    if (d > 8) {
        intBuf = (*env)->NewIntArray(env, w);
	buf = (char *) (*env)->GetIntArrayElements(env, intBuf, 0);
	xim = XCreateImage(display, root_visual,
			   (unsigned)awtImage->Depth, ZPixmap, 0, buf,
			   (unsigned)w, 1, 32, 0);
	xim->bits_per_pixel = 32;
    } else {
	/* XGetSubImage takes care of expanding to 8 bit if necessary. */
	/* But, that is not a supported feature.  There are better ways */
	/* of handling this... */
        byteBuf = (*env)->NewByteArray(env, w);
	buf = (char *) (*env)->GetByteArrayElements(env, byteBuf, 0);
	xim = XCreateImage(display, root_visual,
			   (unsigned)awtImage->Depth, ZPixmap, 0, buf,
			   (unsigned)w, 1, 8, 0);
	xim->bits_per_pixel = 8;
    }
    /*  AWT_UNLOCK(); */
    for (i = 0; i < h; i++) {
        consumer = (*env)->GetObjectField(env, obj, IRCachedIDs.theConsumerFID);

	if (consumer == 0) {
	    break;
	}
	/*  AWT_LOCK(); */
/* fprintf(stderr, "XGetSubImage %s %d\n", __FILE__, __LINE__); fflush(stderr); */
	XGetSubImage(display, pixmap, 0, i, (unsigned)w, 1, 
		     (unsigned)~0, ZPixmap, xim, 0, 0);
	/*  AWT_UNLOCK(); */
	if (d > 8) {
            (*env)->CallVoidMethod(env, consumer, IRCachedIDs.setIntPixelsMID,
                        0, i, w, 1, colorModel, intBuf, 0, w);



	} else {

            (*env)->CallVoidMethod(env, consumer, IRCachedIDs.setBytePixelsMID,
                        0, i, w, 1, colorModel, byteBuf, 0, w);

	}
	if ((*env)->ExceptionOccurred(env)) {
	    break;
	}
    }
    NATIVE_UNLOCK(env);

    if (d > 8) {
      (*env)->ReleaseIntArrayElements(env, intBuf, (jint *)buf, JNI_ABORT);
    } else {
      (*env)->ReleaseByteArrayElements(env, byteBuf, (jbyte *)buf, JNI_ABORT);
    }

    /*  AWT_LOCK(); */
    XFree(xim);
    /*  AWT_UNLOCK(); */
}





