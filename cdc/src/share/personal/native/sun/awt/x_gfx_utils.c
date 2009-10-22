/*
 * @(#)x_gfx_utils.c	1.33 06/10/10
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
#include "Xheaders.h"
#include "common.h"
#include "img_util.h"
#include "img_util_md.h"

/* If defined, check the validity of the java region code by     */
/* shadowing all operations with their native X equivalents.     */
/* Requires using a special version of RegionImpl which supplies */
/* pdata and native methods.                                     */
#define CHECK_REGIONS

extern sgn_ordered_dither_array img_oda_red;
extern sgn_ordered_dither_array img_oda_green;
extern sgn_ordered_dither_array img_oda_blue;

#define ABS(n) ((n < 0) ? -n : n)

JNIEXPORT void JNICALL 
JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg)
{
    jclass cls = (*env)->FindClass(env, name);

    if (cls != 0) /* Otherwise an exception has already been thrown */
        (*env)->ThrowNew(env, cls, msg);
}


JNIACCESS(regionImpl);
JNIACCESS(sun_porting_utils_RegionImpl);
JNIACCESS(sun_porting_utils_YXBand);
JNIACCESS(sun_porting_utils_XSpan);
JNIACCESS(java_awt_Color);

void initRegionClass(JNIEnv *env) {
    FINDCLASS(sun_porting_utils_RegionImpl, "sun/porting/utils/RegionImpl");
    DEFINEFIELD(sun_porting_utils_RegionImpl, bandList,
		"Lsun/porting/utils/YXBand;");
    DEFINEFIELD(sun_porting_utils_RegionImpl, boundingBox,
		"Lsun/porting/utils/Rectangle;");

#ifdef CHECK_REGIONS
    FINDCLASS(regionImpl, "sun/awt/gfX/RegionImpl");
    DEFINEFIELD(regionImpl, pData, "I");
#endif
}

static void initYXBandClass(JNIEnv *env, jobject bands) {
    /* JNI does not allow you to find classes that are not public. */
    /* FINDCLASS(sun_porting_utils_YXBand, "sun/porting/utils/YXBand"); */
    DEFINECLASS(sun_porting_utils_YXBand, (*env)->GetObjectClass(env, bands));
    DEFINEFIELD(sun_porting_utils_YXBand, y, "I");
    DEFINEFIELD(sun_porting_utils_YXBand, h, "I");
    DEFINEFIELD(sun_porting_utils_YXBand, next, "Lsun/porting/utils/YXBand;");
    DEFINEFIELD(sun_porting_utils_YXBand, prev, "Lsun/porting/utils/YXBand;");
    DEFINEFIELD(sun_porting_utils_YXBand, children, 
		"Lsun/porting/utils/XSpan;");
#define NEXTBAND(this) GETFIELD(Object, this, sun_porting_utils_YXBand, next)
 
}

static void initXSpanClass(JNIEnv *env, jobject span) {
    /* JNI does not allow you to find classes that are not public. */
    /* FINDCLASS(sun_porting_utils_XSpan, "sun/porting/utils/XSpan"); */
    DEFINECLASS(sun_porting_utils_XSpan, (*env)->GetObjectClass(env, span));
    DEFINEFIELD(sun_porting_utils_XSpan, x, "I");
    DEFINEFIELD(sun_porting_utils_XSpan, w, "I");
    DEFINEFIELD(sun_porting_utils_XSpan, next, "Lsun/porting/utils/XSpan;");
    DEFINEFIELD(sun_porting_utils_XSpan, prev, "Lsun/porting/utils/XSpan;");
#define NEXTSPAN(this) GETFIELD(Object, this, sun_porting_utils_XSpan, next)
}

static void initColorClass(JNIEnv *env)
{
    FINDCLASS(java_awt_Color, "java/awt/Color");
    DEFINEMETHOD(java_awt_Color, getRGB, "()I");
#define GETRGB(color) CALLMETHOD(Int, color, java_awt_Color, getRGB)
}

#define CopyClip(a, b) CopyClip1(a, b, 0)

#ifdef CHECK_REGIONS
static Region CopyClip2(JNIEnv *env, jobject objClip, int debug) {
    Region clip = XCreateRegion();
    Region reg = (Region) PDATA(objClip, regionImpl);

    XUnionRegion(reg, clip, clip);

    return clip;
}
#endif

static Region CopyClip1(JNIEnv *env, jobject objClip, int debug)
{
    /*
     * NOTE: X has its own implementation of region structures.  We
     * could use them, but they don't support listing the rectangles
     * in the region, so we would have to write a rather gross native
     * method to handle this.  (See XQueryRectangles below.)  And the
     * implementation of our window system running inside another
     * window system was not designed primarily for efficiency,
     * anyway.  So the choice was made to construct a new X region
     * each time we draw a primitive; this has the advantages both
     * that it is relatively simple, and that it demonstrates using
     * the sample Region implementation.
     *
     * Actually, we're currently using both the Java regions and the 
     * native X region implementation in parallel, in order to shake 
     * out any bugs from the Java regions.  (Only one has been found 
     * so far--an anomaly that would occur if you intersect a region
     * with a rectangle that had negative width.)  
     */
    XRectangle rect;
    Region clip;

    jobject bands;
    clip = XCreateRegion();

    /* initialize region; otherwise it is undefined */
    rect.x = rect.y = rect.width = rect.height = 0;
    XUnionRectWithRegion(&rect, clip, clip);
    
    if (MUSTLOAD(sun_porting_utils_RegionImpl)) {
	initRegionClass(env);
    }

    bands = GETFIELD(Object, objClip, sun_porting_utils_RegionImpl, bandList);
    if (bands == NULL) {
	jobject box;
 
	box = GETFIELD(Object, objClip, sun_porting_utils_RegionImpl,
		       boundingBox);

	rect.x      = (short)  GETFIELD(Int, box, java_awt_Rectangle, x);
	rect.y      = (short)  GETFIELD(Int, box, java_awt_Rectangle, y);
	rect.width  = (ushort) GETFIELD(Int, box, java_awt_Rectangle, width);
	rect.height = (ushort) GETFIELD(Int, box, java_awt_Rectangle, height);

	if (debug) {
	    fprintf(stderr, "Got bounds from rectangle...\n");
	}

	XUnionRectWithRegion(&rect, clip, clip);
    } else {
	/* loop through the rectangles */
	jobject yb, tmp;

	if (debug) {
	    fprintf(stderr, "looping through bands...\n");
	}

	if (MUSTLOAD(sun_porting_utils_YXBand)) {
	    initYXBandClass(env, bands);
	}

	yb = NEXTBAND(bands);
	while (! (*env)->IsSameObject(env, yb, bands)) {
	    jobject children, xs;
	    rect.y = (short) GETFIELD(Int, yb, sun_porting_utils_YXBand, y);
	    rect.height = (ushort) GETFIELD(Int, yb,
					    sun_porting_utils_YXBand, h);
	    
	    if (debug) {
		fprintf(stderr, "band from %d to %d\n", 
			rect.y, rect.y + rect.height);
	    }

	    children = GETFIELD(Object, yb, sun_porting_utils_YXBand, children);
	    if (MUSTLOAD(sun_porting_utils_XSpan)) {
		initXSpanClass(env, children);
	    }

	    xs = NEXTSPAN(children);
	    while (! (*env)->IsSameObject(env, xs, children)) {
		rect.x = (short) GETFIELD(Int, xs, sun_porting_utils_XSpan, x);
		rect.width = (ushort) GETFIELD(Int, xs,
					       sun_porting_utils_XSpan, w);

		if (debug) {
		    fprintf(stderr, "    span from %d to %d\n",
			    rect.x, rect.x + rect.width);
		}
		XUnionRectWithRegion(&rect, clip, clip);
		tmp = xs;
		xs = NEXTSPAN(xs);
		(*env)->DeleteLocalRef(env, tmp);
	    }

	    (*env)->DeleteLocalRef(env, children);
	    
	    tmp = yb;
	    yb = NEXTBAND(yb);
	    (*env)->DeleteLocalRef(env, tmp);
	}

	(*env)->DeleteLocalRef(env, bands);
    }

    return clip;
}

unsigned long gfXGetPixel(JNIEnv *env, jobject fg, 
			  jobject xor, int gamma_correct) {
    jint RGB;
    unsigned long rval = 0;

    assert(NATIVE_LOCK_HELD());

    if (MUSTLOAD(java_awt_Color)) {
	initColorClass(env);
    }

    if (fg == NULL) {
	return rval;
    }

    RGB = GETRGB(fg);
    if (xor == NULL) {
	rval = jRGB2Pixel((unsigned)RGB, gamma_correct);
    } else {
	rval = jRGB2Pixel((unsigned)RGB, TRUE);

	RGB = GETRGB(xor);
	rval ^= jRGB2Pixel((unsigned)RGB, TRUE);
    }

    return rval;
}


static int pixel_near_match(int gray, int nc, int r, int g, int b,
			    int dR, int dG, int dB) {
    int er = (r > dR) ? r - dR : dR - r;

    if (gray) {
	return er < 3;
    } else {
	int eg = (g > dG) ? g - dG : dG - g;
	int eb = (b > dB) ? b - dB : dB - b;

	return ((er < 5) && (eg < 5) && (eb < 5));
    }
}

#define TILESIZE 2

#if   TILESIZE == 2
static unsigned char xpos[] = {0, 1, 1, 0};
static unsigned char ypos[] = {0, 1, 0, 1};
#elif TILESIZE == 4
static unsigned char xpos[] = {0, 2, 2, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 2, 2, 0};
static unsigned char ypos[] = {0, 2, 0, 2, 1, 3, 1, 3, 0, 2, 0, 2, 1, 3, 1, 3};
#endif

static void make_tile(XImage *img, int dr, int dg, int db)
{
    int i;
    int red = dr;
    int green = dg;
    int blue = db;
    unsigned pixel, tmp;
    for (i = 0; i < TILESIZE*TILESIZE; ++i) {
	int x = xpos[i];
	int y = ypos[i];
	pixel = jRGB2Pixel((red<<16) | (green<<8) | blue, True);

	XPutPixel(img, x, y, pixel);

	pixel2RGB(pixel, &red, &green, &blue);
	red   = 2*dr - red;    red   = ComponentBound(red);
	green = 2*dg - green;  green = ComponentBound(green);
	blue  = 2*db - blue;   blue  = ComponentBound(blue);
    }

#if TILESIZE == 2
    /* 
     * check a couple of special cases: if we're about to draw stripes,
     * turn them into checks instead.
     */
    tmp = XGetPixel(img, 0, 0);
    if (tmp == XGetPixel(img, 1, 0)) {
	/* horizontal stripe */
	XPutPixel(img, 0, 0, XGetPixel(img, 0, 1));
	XPutPixel(img, 0, 1, tmp);
    } else if (tmp == XGetPixel(img, 0, 1)) {
	/* vertical stripe */
	XPutPixel(img, 0, 0, XGetPixel(img, 1, 0));
	XPutPixel(img, 1, 0, tmp);
    } else if (pixel == XGetPixel(img, 1, 0)) {
	/* vertical stripe */
	XPutPixel(img, 1, 1, XGetPixel(img, 0, 1));
	XPutPixel(img, 0, 1, pixel);
    } else if (pixel == XGetPixel(img, 0, 1)) {
	/* vertical stripe */
	XPutPixel(img, 1, 1, XGetPixel(img, 1, 0));
	XPutPixel(img, 1, 0, pixel);
    }
#endif       
}

void SetupGC(JNIEnv *env, GC gc, jobject region, 
	     jobject fg, jobject xor,
	     Region *clipRegion) 
{
    Region clip;

    assert(NATIVE_LOCK_HELD());    

    if (MUSTLOAD(java_awt_Color)) {
	initColorClass(env);
    }

    if (region != NULL) {
	clip = CopyClip(env, region);
	assert(NATIVE_LOCK_HELD());
	XSetRegion(display, gc, clip);

	if (clipRegion == NULL) {
	    XDestroyRegion(clip);
	} else {
	    *clipRegion = clip;
	}
    } else if (clipRegion != NULL) {
	*clipRegion = NULL;
    }

    XSetArcMode(display, gc, ArcPieSlice);

    if (xor != NULL) {
	XSetFunction(display, gc, GXxor);
	XSetFillStyle(display, gc, FillSolid);
	XSetForeground(display, gc, gfXGetPixel(env, fg, xor, True));
    } else if (fg != NULL) {
	XImage *img;
	static Pixmap colorTile = 0;
	static GC tileGC;
	unsigned int pixel_data[TILESIZE][TILESIZE];
	unsigned long pix;
	unsigned long desired_color;
	int r, g, b, dr, dg, db;

	if (colorTile == 0) {
	    colorTile = XCreatePixmap(display, root_drawable, 
				      TILESIZE, TILESIZE, (unsigned)screen_depth);
	    tileGC = XCreateGC(display, colorTile, 0, 0);
	}

	XSetFunction(display, gc, GXcopy);
	pix = gfXGetPixel(env, fg, xor, False);
	desired_color = (fg == NULL) ? 0 : GETRGB(fg);

	/* split into elements */
	pixel2RGB(pix, &r, &g, &b);
	dr = (desired_color >> 16) & 0xff;
	dg = (desired_color >>  8) & 0xff;
	db = (desired_color >>  0) & 0xff;

	if (grayscale) {
	    extern unsigned short ntsc_gray_value[3][256];
	    int gray = ntsc_gray_value[0][dr] 
		     + ntsc_gray_value[1][dg]
		     + ntsc_gray_value[2][db];
	    dr = dg = db = (gray >> 8);
	}

	if (pixel_near_match(grayscale, numColors, r, g, b, dr, dg, db)) {
	    /* use the gamma-corrected version of the pixel */
	    XSetForeground(display, gc, gfXGetPixel(env, fg, xor, True));
	    XSetFillStyle(display, gc, FillSolid);
	} else {
	    img = XCreateImage(display, root_visual, (unsigned)screen_depth,
			       ZPixmap, 0, (char *)pixel_data, 
			       TILESIZE, TILESIZE, 
			       32, sizeof(unsigned int) * TILESIZE);

	    make_tile(img, dr, dg, db);

	    XPutImage(display, colorTile, tileGC, img, 0, 0, 
		      0, 0, TILESIZE, TILESIZE);
	    XSetTile(display, gc, colorTile);
	    XSetFillStyle(display, gc, FillTiled);

	    XFree(img);
	}
    }

    CHECK_EXCEPTION(SetupGC(finish));
}

void Translate(JNIEnv *env, jobject obj, jint *x, jint *y) 
{
    *x += GETFIELD(Int, obj, java_awt_Rectangle, x);
    *y += GETFIELD(Int, obj, java_awt_Rectangle, y);
}

XPoint *MakePolygonPoints(jint xOffset, jint yOffset, jint *nPoints,
			  jint *xPts, jint *yPts, int close) 
{
    XPoint *pts = sysCalloc((unsigned)(*nPoints) + 1, sizeof(XPoint));
    int i;

    if (pts == NULL) {
	/* OutOfMemoryException? */
	fprintf(stderr, "calloc failed at line %d file %s\n",
		__LINE__, __FILE__);
    }

    for (i = 0; i < *nPoints; ++i) {
	pts[i].x = (short) xPts[i] + xOffset;
	pts[i].y = (short) yPts[i] + yOffset;
    }

    if (close && memcmp(pts, pts + (*nPoints) - 1, sizeof(XPoint))) {
	pts[*nPoints].x = xPts[0] + xOffset;
	pts[*nPoints].y = yPts[0] + yOffset;
	++(*nPoints);
    }

    return pts;
}

void DrawRoundRect(Drawable drawable, GC gc, 
		   int x, int y, int w, int h, int arcWidth, int arcHeight) {

    assert(NATIVE_LOCK_HELD());

    if ((w < 4) && (h < 4)) {
	XDrawRectangle(display, drawable, gc, x, y, (unsigned)w, (unsigned)h);
    } else {
	int ty1 = y + (arcHeight / 2);
	int ty2 = y + h - (arcHeight / 2);
	int tx1 = x + (arcWidth / 2);
	int tx2 = x + w - (arcWidth / 2);
	int txw = x + w;
	int tyh = y + h;

	arcWidth = ABS(arcWidth);
	arcHeight = ABS(arcHeight);
	if (arcWidth > w) arcWidth = w;
	if (arcHeight > h) arcHeight = h;

	XDrawArc(display, drawable, gc,
		 x, y, (unsigned)arcWidth, (unsigned)arcHeight, 
		 90<<6, 90<<6);

	XDrawArc(display, drawable, gc,
		 x + w - arcWidth - 1, y, 
		 (unsigned)arcWidth, (unsigned)arcHeight, 
		 0, 90<<6);

	XDrawArc(display, drawable, gc,
		 x, y + h - arcHeight - 1,
		 (unsigned)arcWidth, (unsigned)arcHeight, 
		 180<<6, 90<<6);

	XDrawArc(display, drawable, gc,
		 x + w - arcWidth - 1, y + h - arcHeight - 1, 
		 (unsigned)arcWidth, (unsigned)arcHeight, 
		 270<<6, 90<<6);

	XDrawLine(display, drawable, gc,
		  tx1+1, y, tx2-1, y);

	XDrawLine(display, drawable, gc,
		  txw, ty1+1, txw, ty2-1);

	XDrawLine(display, drawable, gc,
		  tx1+1, tyh, tx2-1, tyh);

	XDrawLine(display, drawable, gc,
		  x, ty1+1, x, ty2-1);
    }
}

void FillRoundRect(Drawable drawable, GC gc, 
		   int x, int y, int w, int h, int arcWidth, int arcHeight)
{
    assert(NATIVE_LOCK_HELD());

    if ((w < 4) && (h < 4)) {
	XFillRectangle(display, drawable, gc, x, y, 
		       (unsigned)w, (unsigned)h);
    } else {
	int ty1 = y + (arcHeight / 2);
	int ty2 = y + h - (arcHeight / 2);
	int tx1 = x + (arcWidth / 2);
	int tx2 = x + w - (arcWidth / 2);
	int txw = x + w;
	int tyh = y + h;

	arcWidth = ABS(arcWidth);
	arcHeight = ABS(arcHeight);
	if (arcWidth > w) arcWidth = w;
	if (arcHeight > h) arcHeight = h;

	XFillArc(display, drawable, gc,
		 x, y, (unsigned)arcWidth, (unsigned)arcHeight,
		 90<<6, 90<<6);

	XFillArc(display, drawable, gc,
		 x + w - arcWidth - 1, y, 
		 (unsigned)arcWidth, (unsigned)arcHeight,
		 0, 90<<6);

	XFillArc(display, drawable, gc,
		 x, y + h - arcHeight - 1, 
		 (unsigned)arcWidth, (unsigned)arcHeight,
		 180<<6, 90<<6);

	XFillArc(display, drawable, gc,
		 x + w - arcWidth - 1, y + h - arcHeight - 1, 
		 (unsigned)arcWidth, (unsigned)arcHeight,
		 270<<6, 90<<6);

	XFillRectangle(display, drawable, gc,
		       tx1, y, 
		       (unsigned)(tx2-tx1), 
		       (unsigned)(tyh-y));

	XFillRectangle(display, drawable, gc,
		       x, ty1, 
		       (unsigned)(tx1-x), 
		       (unsigned)(ty2-ty1));

	XFillRectangle(display, drawable, gc,
		       tx2, ty1, 
		       (unsigned)(txw-tx2), 
		       (unsigned)(ty2-ty1));
		       
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_init(JNIEnv *env,
				jobject obj)
{
#ifdef CHECK_REGIONS
    Region  reg = XCreateRegion();
    jobject bbox;
    XRectangle rect;

    /* initialize region; otherwise it is undefined */
    rect.x = rect.y = rect.width = rect.height = 0;
    XUnionRectWithRegion(&rect, reg, reg);

    if (MUSTLOAD(sun_porting_utils_RegionImpl)) {
	initRegionClass(env);
    }

    bbox = GETFIELD(Object, obj,
		    sun_porting_utils_RegionImpl, boundingBox);

    if (bbox != NULL) {
	XRectangle r;
	r.x      = GETFIELD(Int, bbox, java_awt_Rectangle, x);
	r.y      = GETFIELD(Int, bbox, java_awt_Rectangle, y);
	r.width  = GETFIELD(Int, bbox, java_awt_Rectangle, width);
	r.height = GETFIELD(Int, bbox, java_awt_Rectangle, height);

	XUnionRectWithRegion(&r, reg, reg);
    }

    SETFIELD(Int, obj, regionImpl, pData, (jint)reg);
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_free(JNIEnv *env,
				jobject obj)
{
#ifdef CHECK_REGIONS
    Region reg = (Region) PDATA(obj, regionImpl);

    if (reg != NULL) {
	XDestroyRegion(reg);
    }

    SETFIELD(Int, obj, regionImpl, pData, 0);
#endif
}

#ifdef CHECK_REGIONS
static XRectangle *XRegionQuery(Region r, XRectangle *bounds);

static void dumpClipRegion(FILE *f, Region r) {
    XRectangle bounds;
    XRectangle *rects = XRegionQuery(r, &bounds);
    XRectangle *p;
    int curY = bounds.y - 1;
    int rowWidth = 0;

    if (rects == NULL) {
	fprintf(stderr, "Region is rectangle [%d, %d, %d, %d]\n",
		bounds.x, bounds.y, bounds.width, bounds.height);
	return;
    } else {
	fprintf(stderr, "Region has bounds [%d, %d, %d, %d]\n",
		bounds.x, bounds.y, bounds.width, bounds.height);
    }

    for (p = rects; ((p->x | p->y | p->width | p->height) != 0); ++p) {
	if (curY != p->y) {
	    if (rowWidth > 0) {
		fprintf(stderr, "\n");
	    }
	    rowWidth = 3;

	    fprintf(stderr, "Band from %d to %d: ",
		    p->y, p->y + p->height);
	    curY = p->y;
	}

	if (rowWidth > 5) {
	    fprintf(stderr, "\n");
	    rowWidth = 0;
	}
	fprintf(stderr, "  %d..%d", p->x, p->x + p->width);
	++rowWidth;
    }

    if (rowWidth > 0) {
	fprintf(stderr, "\n");
    }
}


static int regions_ok = 1;
#define CHECK_REGION_IMPL()                                         \
do {                                                                \
    Region clip = CopyClip(env, obj);                               \
    if (!XEqualRegion(clip, reg)) {                                 \
        regions_ok = 0;                                             \
        XDestroyRegion(clip);                                       \
	clip = CopyClip1(env, obj, 1);				    \
	fprintf(stderr, "Warning: clip regions differ\n");          \
	fprintf(stderr, "File %s, line %d\n", __FILE__, __LINE__);  \
	fprintf(stderr, "Java clip region: ");                      \
	dumpClipRegion(stderr, clip);                               \
	fprintf(stderr, "X clip region: ");                         \
	dumpClipRegion(stderr, reg) ;                               \
	fflush(stderr);                                             \
    }                                                               \
    XDestroyRegion(clip);                                           \
} while (0)
#else
#define CHECK_REGION_IMPL()
#endif

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_copy0(JNIEnv *env,
				  jobject obj,
				  jobject src)
{
#ifdef CHECK_REGIONS
    Region reg, org;

    if (src == NULL) {
	return;
    }

    org = (Region) PDATA(src, regionImpl);
    if (org == NULL) {
	return;
    }

    reg = (Region) PDATA(obj, regionImpl);
    if (reg != NULL) {
	XDestroyRegion(reg);
    }

    reg = XCreateRegion();
    if (reg == NULL) {
	return;
    }

    XUnionRegion(reg, org, reg);
    SETFIELD(Int, obj, regionImpl, pData, (jint)reg);

    CHECK_REGION_IMPL();
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_setEmpty0(JNIEnv *env,
				      jobject obj)
{
#ifdef CHECK_REGIONS
    XRectangle rect;
    Region reg = (Region) PDATA(obj, regionImpl);
    if (reg == NULL) {
	return;
    }

    XDestroyRegion(reg);
    reg = XCreateRegion();

    /* initialize region; otherwise it is undefined */
    rect.x = rect.y = rect.width = rect.height = 0;
    XUnionRectWithRegion(&rect, reg, reg);

    SETFIELD(Int, obj, regionImpl, pData, (jint)reg);
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_translate0(JNIEnv *env,
				       jobject obj,
				       jint dx, jint dy)
{
#ifdef CHECK_REGIONS
    Region reg = (Region) PDATA(obj, regionImpl);
    if (reg == NULL) {
	return;
    }

    XOffsetRegion(reg, dx, dy);
    CHECK_REGION_IMPL();
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_union0(JNIEnv *env,
				   jobject obj,
				   jobject src)
{
#ifdef CHECK_REGIONS
    Region reg, org;

    if (src == NULL) {
	return;
    }

    reg = (Region) PDATA(obj, regionImpl);
    org = (Region) PDATA(src, regionImpl);

    if ((reg == NULL) || (org == NULL)) {
	return;
    }

    XUnionRegion(reg, org, reg);
    CHECK_REGION_IMPL();
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_union1(JNIEnv *env,
				   jobject obj,
				   jint x, jint y, jint w, jint h)
{
#ifdef CHECK_REGIONS
    Region reg;
    XRectangle rect;

    reg = (Region) PDATA(obj, regionImpl);

    if (reg == NULL) {
	return;
    }

    rect.x = x; rect.y = y; rect.width = w; rect.height = h;

    XUnionRectWithRegion(&rect, reg, reg);
    CHECK_REGION_IMPL();
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_subtract0(JNIEnv *env,
				      jobject obj,
				      jobject src)
{
#ifdef CHECK_REGIONS
    Region reg, org;

    if (src == NULL) {
	return;
    }

    reg = (Region) PDATA(obj, regionImpl);
    org = (Region) PDATA(src, regionImpl);

    if ((reg == NULL) || (org == NULL)) {
	return;
    }

    XSubtractRegion(reg, org, reg);
    CHECK_REGION_IMPL();
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_subtract1(JNIEnv *env,
				      jobject obj,
				      jint x, jint y, jint w, jint h)
{
#ifdef CHECK_REGIONS
    XRectangle rect;
    Region reg, org;
    
    reg = (Region) PDATA(obj, regionImpl);
    if (reg == NULL) {
	return;
    }

    org = XCreateRegion();
    if (org == NULL) {
	return;
    }

    rect.x = x; rect.y = y; rect.width = w; rect.height = h;

    XUnionRectWithRegion(&rect, org, org);
    XSubtractRegion(reg, org, reg);
    XDestroyRegion(org);
    CHECK_REGION_IMPL();
#endif
}


JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_intersect0(JNIEnv *env,
				      jobject obj,
				      jobject src)
{
#ifdef CHECK_REGIONS
    Region reg, org;

    if (src == NULL) {
	return;
    }

    reg = (Region) PDATA(obj, regionImpl);
    org = (Region) PDATA(src, regionImpl);

    if ((reg == NULL) || (org == NULL)) {
	return;
    }

    XIntersectRegion(reg, org, reg);
    CHECK_REGION_IMPL();
#endif
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_RegionImpl_intersect1(JNIEnv *env,
				      jobject obj,
				      jint x, jint y, jint w, jint h)
{
#ifdef CHECK_REGIONS
    XRectangle rect;
    Region reg, org, keep;
    
    reg = (Region) PDATA(obj, regionImpl);

    if (reg == NULL) {
	return;
    }

    org = XCreateRegion();

    if (org == NULL) {
	return;
    }

    keep = XCreateRegion();
    if (keep != NULL) {
	XUnionRegion(keep, reg, keep);
    }

    rect.x = x; rect.y = y; rect.width = w; rect.height = h;

    XUnionRectWithRegion(&rect, org, org);

    XIntersectRegion(reg, org, reg);
    XDestroyRegion(org);
    CHECK_REGION_IMPL();

    if (!regions_ok) {
	fprintf(stderr, "Original region "); dumpClipRegion(stderr, keep);
	fprintf(stderr, "intersected with rectangle %d %d %d %d\n",
		x, y, w, h);
	fflush(stderr);
	regions_ok = 1;
    }
    if (keep) XDestroyRegion(keep);
#endif
}


#ifndef NDEBUG
/* returns a list of XRectangles, terminated with one that is 0,0,0,0 */
/* EXCEPT that if the region is simply a rectangle, it returns null.  */
static XRectangle *XRegionQuery(Region r, XRectangle *bounds)
{
    int bandStart;
    int curX, curY, curW, curH;

    int numRects;
    int regSize;
    XRectangle *rects;

#define REG_INCR 4
#define ADD_RECT(_x, _y, _w, _h)                                              \
do {                                                                          \
    if (++numRects >= regSize) {                                              \
	XRectangle *nr = sysMalloc(sizeof(XRectangle) * (regSize + REG_INCR));   \
        if (nr == NULL) { \
	    /* OutOfMemoryError */ \
	    fprintf(stderr, "malloc failed on line %d file %s\n", \
		    __LINE__, __FILE__); \
	    return NULL; \
        } \
	memcpy(nr, rects, (regSize * sizeof(XRectangle)));                    \
	memset(nr + regSize, 0, REG_INCR*sizeof(XRectangle));                 \
	regSize += REG_INCR;                                                  \
	free(rects);                                                          \
	rects = nr;                                                           \
    }                                                                         \
    rects[numRects-1].x      = _x;                                            \
    rects[numRects-1].y      = _y;                                            \
    rects[numRects-1].width  = _w;                                            \
    rects[numRects-1].height = _h;                                            \
} while (0)

    XClipBox(r, bounds);
    if ((bounds->width <= 0) || (bounds->height <= 0)) {
	return NULL;
    }

    if (   XRectInRegion(r, bounds->x, bounds->y, bounds->width, bounds->height)
	== RectangleIn) {
	return NULL;
    }

    numRects = 0;
    regSize = REG_INCR;
    rects = sysCalloc(sizeof(XRectangle), regSize);
    if (rects == NULL) {
	/* OutOfMemoryException? */
	fprintf(stderr, "calloc failed at line %d file %s\n",
		__LINE__, __FILE__);
	return NULL;
    }

    /* First do a height scan; we're looking for bands that scan the */
    /* complete width of the region.                                 */
    curY = bounds->y;
    while (curY < (bounds->y + bounds->height)) {
	curX = bounds->x;
	curW = bounds->width;
	curH = 1;
	while (curY < (bounds->y + bounds->height)) {
	    switch (XRectInRegion(r, curX, curY, curW - 1, curH)) {
	    case RectangleIn:
		++curH;
		break;

	    case RectangleOut:
		++curY;
		break;

	    case RectanglePart:
	    default:
		if (curH > 1) {
		    --curH;
		    ADD_RECT(curX, curY, curW, curH);
		    curY += curH;
		    curH = 1;
		} else {
		    /* time to exit the loop.  Because we're inside a switch */
		    /* statement, the easiest way is with a goto.            */
		    goto scanX;
		}
	    }
	}
scanX: /* EXIT FROM THE ABOVE LOOP */

	if (curY >= (bounds->y + bounds->height)) {
	    break;
	}

	/* Now do a width scan, looking for all bands at the current Y */
	bandStart = numRects;
	curW = 1;
	while (curX < (bounds->x + bounds->width)) {
	    switch (XRectInRegion(r, curX, curY, curW, curH)) {
	    case RectangleIn:
		++curW;
		break;

	    case RectangleOut:
		++curX;
		break;

	    case RectanglePart:
	    default:
		if (curW > 1) {
		    --curW;
		    ADD_RECT(curX, curY, curW, curH);
		}
		curX += curW;
		curW = 1;
	    }
	}

	/* Finally, increase height of all X spans equally until one    */
	/* of them doesn't fit.  This guarantees that we're retaining   */
	/* Y-X bandedness; in other words, we'll always find the widest */
	/* rectangles we can for any given Y, rather than the tallest.  */

	for (;;) {
	    int lastX = bounds->x;
	    int i = numRects - 1;

	    if (XRectInRegion(r, rects[i].x + rects[i].width + 1, 
			      rects[i].y,
			      bounds->width, 
			      rects[i].height + 1) != RectangleOut) {
		break;
	    }

	    for (i = bandStart; i < numRects; ++i) {
		++(rects[i].height);
		if (   (      XRectInRegion(r, rects[i].x, rects[i].y,
					   rects[i].width, rects[i].height)
			   != RectangleIn)
		    ||  (     XRectInRegion(r, lastX, rects[i].y, 
					    rects[i].x - 1 - lastX, 
					    rects[i].height) 
			   != RectangleOut)) {
		    while (i >= bandStart) {
			--(rects[i].height);
			--i;
		    }
		    break;
		}

		lastX = rects[i].x + rects[i].width + 1;
	    }

	    if (i < numRects) break;
	}

	curY += rects[bandStart].height;
    }

    return rects;
}
#endif
