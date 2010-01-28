/*
 * @(#)GraphicsImpl.c	1.27 06/10/10
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

#include "sun_awt_gfX_GraphicsImpl.h"
#include "Xheaders.h"
#include "common.h"

extern void gfx_output_flush(JNIEnv *env);

typedef struct _gid_ {
    GC       gc;
    Drawable drawable;
} GraphicsImplData;

JNIACCESS(graphicsImpl);
JNIACCESS(sun_awt_DrawingSurfaceInfo);

JNIEXPORT void JNICALL 
Java_sun_awt_gfX_GraphicsImpl_initJNI(JNIEnv *env, jclass cl) {

    DEFINECLASS(graphicsImpl, cl);

    DEFINEFIELD(graphicsImpl, pData, "I");
    DEFINEFIELD(graphicsImpl, foreground, "Ljava/awt/Color;");
    DEFINEFIELD(graphicsImpl, xorColor,   "Ljava/awt/Color;");
    DEFINEFIELD(graphicsImpl, geometry, 
		"Lsun/porting/graphicssystem/GeometryProvider;");
    DEFINEFIELD(graphicsImpl, drawable, 
		"Lsun/porting/graphicssystem/Drawable;");
    DEFINEMETHOD(graphicsImpl, setupClip,
		 "(Ljava/awt/Rectangle;)Lsun/porting/graphicssystem/Region;");

    FINDCLASS(sun_awt_DrawingSurfaceInfo,
	      "sun/awt/DrawingSurfaceInfo");
    DEFINEMETHOD(sun_awt_DrawingSurfaceInfo, 
		 getClip, "()Ljava/awt/Shape;");
    DEFINEMETHOD(sun_awt_DrawingSurfaceInfo, 
		 getBounds, "()Ljava/awt/Rectangle;");
    DEFINEMETHOD(sun_awt_DrawingSurfaceInfo, lock, "()I");
    DEFINEMETHOD(sun_awt_DrawingSurfaceInfo, unlock, "()V");
}

extern Pixmap getDrawablePixmap(JNIEnv *env, jobject obj);

static Drawable getNativeDrawable(JNIEnv *env, jobject drawable)
{
    Drawable d;
    if (INSTANCEOF(drawable, screenImpl)) {
	d = root_drawable;
    } else if (INSTANCEOF(drawable, drawableImageImpl)) {
	d = getDrawablePixmap(env, drawable);
    } else {
	fprintf(stderr, "drawable argument is not really a drawable??\n");
	fflush(stderr);
	CastClassException(env, "Expecting a Drawable");
	return 0;
    }

    return d;
}

#define GRAPHICS_SETUP(_fg_, _data_, _xoff_, _yoff_)               \
    jobject region = NULL;                                         \
    GRAPHICS_SETUP_WITH_CLIP(_fg_, _data_, _xoff_, _yoff_, NULL)   \
    else {NATIVE_UNLOCK(env); return;}

#define GRAPHICS_CLEANUP(clip)               \
do {                                         \
    if (clip != NULL) XDestroyRegion(clip);  \
    gfx_output_flush(env);                   \
    assert(NATIVE_LOCK_HELD() == 1);         \
    NATIVE_UNLOCK(env);                      \
} while (0)

#define GRAPHICS_SETUP_WITH_CLIP(_fg_, _data_, _xoff_, _yoff_, _clip_)       \
    GraphicsImplData *_data_ = NULL;                                         \
    NATIVE_LOCK(env);                                                        \
    _data_ = (GraphicsImplData *) PDATA(obj, graphicsImpl);                  \
    if ((_data_ != NULL) && (_data_->gc != NULL)) {                          \
        assert(NATIVE_LOCK_HELD() == 1);                                     \
        Translate(env, bounds, &_xoff_, &_yoff_);                            \
        SetupGC(env, _data_->gc, region, _fg_, xorColor, _clip_);            \
    }


/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    CopyArea
 * Signature: (Lsun/porting/graphicssystem/Region;Ljava/awt/Point;Ljava/awt/Color;Ljava/awt/Color;IIIIII)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_CopyArea(JNIEnv *env,
				       jobject obj,
				       jobject bounds, 
				       jobject background, jobject xorColor, 
				       jint x, jint y, jint w, jint h, jint dx, jint dy)
{
    GRAPHICS_SETUP(background, data, x, y);
    XCopyArea(display, data->drawable, data->drawable, data->gc,
	      x, y, (unsigned)w, (unsigned)h, x + dx, y + dy);
    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    Line
 * Signature: (Lsun/porting/graphicssystem/Drawable;Lsun/porting/graphicssystem/Region;Ljava/awt/Point;Ljava/awt/Color;Ljava/awt/Color;IIII)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_Line(JNIEnv *env,
				   jobject obj,
				   jobject drawable, jobject bounds, 
				   jobject foreground, jobject xorColor, 
				   jint x1, jint y1, jint x2, jint y2)
{
    jint xoff = 0, yoff = 0;
    GRAPHICS_SETUP(foreground, data, xoff, yoff);
    XDrawLine(display, data->drawable, data->gc, 
	      x1 + xoff, y1 + yoff, x2 + xoff, y2 + yoff);
    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    Rectangle
 * Signature: (Lsun/porting/graphicssystem/Drawable;Lsun/porting/graphicssystem/Region;Ljava/awt/Point;Ljava/awt/Color;Ljava/awt/Color;ZIIII)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_Rectangle(JNIEnv *env,
					jobject obj,
					jobject drawable, jobject bounds, 
					jobject foreground, jobject xorColor, 
					jboolean filled, jint x, jint y, jint w, jint h)
{
    GRAPHICS_SETUP(foreground, data, x, y);
    if (filled) {
	XFillRectangle(display, data->drawable, data->gc, x, y, 
		       (unsigned)w, (unsigned)h);
    } else {
	XDrawRectangle(display, data->drawable, data->gc, x, y,
		       (unsigned)w, (unsigned)h);
    }
    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    RoundRect
 * Signature: (Lsun/porting/graphicssystem/Drawable;Lsun/porting/graphicssystem/Region;Ljava/awt/Point;Ljava/awt/Color;Ljava/awt/Color;ZIIIIII)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_RoundRect(JNIEnv *env,
					jobject obj,
					jobject drawable, jobject bounds, 
					jobject foreground, jobject xorColor, 
					jboolean filled, jint x, jint y, jint w, jint h, 
					jint arcWidth, jint arcHeight)
{
    GRAPHICS_SETUP(foreground, data, x, y);
    if (filled) {
	FillRoundRect(data->drawable, data->gc, x, y, w, h, arcWidth, arcHeight);
    } else {
	DrawRoundRect(data->drawable, data->gc, x, y, w, h, arcWidth, arcHeight);
    }
    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    Arc
 * Signature: (Lsun/porting/graphicssystem/Drawable;Lsun/porting/graphicssystem/Region;Ljava/awt/Point;Ljava/awt/Color;Ljava/awt/Color;ZIIIIII)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_Arc(JNIEnv *env,
				  jobject obj,
				  jobject drawable, jobject bounds, 
				  jobject foreground, jobject xorColor, 
				  jboolean filled, jint x, jint y, jint w, jint h, 
				  jint startAngle, jint arcAngle)
{
    GRAPHICS_SETUP(foreground, data, x, y);

    if (filled) {
	XFillArc(display, data->drawable, data->gc,
		 x, y, (unsigned)w, (unsigned)h, 
		 (startAngle <<6), (arcAngle << 6));
    } else {
	XDrawArc(display, data->drawable, data->gc,
		 x, y, (unsigned)w, (unsigned)h, 
		 (startAngle << 6), (arcAngle << 6));
    }
    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    Polygon
 * Signature: (Lsun/porting/graphicssystem/Drawable;Lsun/porting/graphicssystem/Region;Ljava/awt/Point;Ljava/awt/Color;Ljava/awt/Color;Z[I[IIZ)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_Polygon(JNIEnv *env,
				      jobject obj,
				      jobject drawable, jobject bounds, 
				      jobject foreground, jobject xorColor, 
				      jboolean filled, 
				      jintArray xPts, jintArray yPts, 
				      jint nPts, 
				      jboolean close)
{
    jint   *xp, *yp;
    XPoint *pts;
    jint xoff = 0, yoff = 0;

    GRAPHICS_SETUP(foreground, data, xoff, yoff);
    
    xp = (*env)->GetIntArrayElements(env, xPts, 0);
    yp = (*env)->GetIntArrayElements(env, yPts, 0);

    pts = MakePolygonPoints(xoff, yoff, &nPts, xp, yp, close ? 1 : 0);

    (*env)->ReleaseIntArrayElements(env, xPts, xp, JNI_ABORT);
    (*env)->ReleaseIntArrayElements(env, yPts, yp, JNI_ABORT);

    if (filled) {
	XFillPolygon(display, data->drawable, data->gc,
		     pts, nPts, Complex, CoordModeOrigin);
    } else {
	XDrawLines(display, data->drawable, data->gc, pts, nPts, 
		   CoordModeOrigin);
    }

    GRAPHICS_CLEANUP(NULL);

    sysFree(pts);
}

extern void 
DrawMFString(JNIEnv *env, jcharArray s, GC gc, Drawable drawable,
	     jobject font, int x, int y, int offset, int sLength);
/*
 * Class:     sun_awt_motif_X11Graphics
 * Method:    drawMFChars
 * Signature: ([CIIIILjava/awt/Font;)V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_gfX_GraphicsImpl_drawMFChars(JNIEnv *env,
					   jobject obj,
					   jobject drawable, 
					   jobject bounds, 
					   jobject foreground, 
					   jobject xorColor,
					   jcharArray text, 
					   jint offset,
					   jint length,
					   jint x, jint y,
					   jobject font) 
{
    GRAPHICS_SETUP(foreground, data, x, y);

    if (text == NULL || font == NULL) {
	NullPointerException(env, "NullPointerException");
        return;
    }

    DrawMFString(env, text, data->gc, data->drawable,
		 font, x, y, offset, length);

    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_motif_X11Graphics
 * Method:    drawSFChars
 * Signature: ([CIIII)V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_gfX_GraphicsImpl_drawSFChars(JNIEnv *env, 
					   jobject obj,
					   jobject drawable, 
					   jobject bounds, 
					   jobject foreground, 
					   jobject xorColor,
					   jcharArray text,
					   jint offset,
					   jint length,
					   jint x, jint y) 
{
    jchar *chardata;
    int arrayLength;

    GRAPHICS_SETUP(foreground, data, x, y);

    if (JNU_IsNull(env, text)) {
        NullPointerException(env, "NullPointerException");
        return;
    }
    arrayLength = (*env)->GetArrayLength(env, text);

    if (offset < 0 || length < 0 || offset + length > arrayLength) {
        ArrayIndexOutOfBoundsException(env, "ArrayIndexOutOfBoundsException");
        return;
    }

    if (length > 1024) {
        length = 1024;
    }

    chardata = (*env)->GetCharArrayElements(env, text, NULL);
    
    if (chardata == 0) {
        return;
    }
    
    XDrawString16(display, data->drawable, data->gc,
		  x, y, (XChar2b *) & (chardata[offset]), length);
    
    if (chardata) {
        (*env)->ReleaseCharArrayElements(env, text, chardata, JNI_ABORT);
    }

    GRAPHICS_CLEANUP(NULL);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_init(JNIEnv *env,
				   jobject obj,
				   jobject drawable)
{
    GraphicsImplData *data = sysMalloc(sizeof(GraphicsImplData));

    if (data == NULL) {
	OutOfMemoryError(env, "No memory for Graphics data");
	return;
    }

    data->drawable = getNativeDrawable(env, drawable);
    if (data->drawable == 0) {
	sysFree(data);
	OutOfMemoryError(env, "No memory for Graphics data");
	return;
    }

    NATIVE_LOCK(env);
    data->gc = XCreateGC(display, data->drawable, 0, NULL);

    if (data->gc == NULL) {
	NATIVE_UNLOCK(env);
	sysFree(data);
	OutOfMemoryError(env, "No memory for Graphics data");
	return;
    }

    SETFIELD(Int, obj, graphicsImpl, pData, (jint)data);
    NATIVE_UNLOCK(env);
}


JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_resetClip(JNIEnv *env,
					jobject obj,
					jobject region) 
{
    GraphicsImplData *_data_;
    NATIVE_LOCK(env);
    _data_ = (GraphicsImplData *) PDATA(obj, graphicsImpl);
    if (_data_ == NULL) {
	NATIVE_UNLOCK(env);
	return;
    }

    if ((_data_ != NULL) && (_data_->gc != NULL)) {
	SetupGC(env, _data_->gc, region, NULL, NULL, NULL);
    }

    NATIVE_UNLOCK(env);
}

/*
 * Class:     sun_awt_gfX_GraphicsImpl
 * Method:    disposeNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsImpl_disposeNative(JNIEnv *env,
					    jobject obj,
					    jint pData)
{
    GraphicsImplData *data = (GraphicsImplData *) pData;
    if (data != NULL) {
	NATIVE_LOCK(env);
	XFreeGC(display, data->gc);
	data->gc = NULL;
	data->drawable = NULL;
	sysFree(data);
	NATIVE_UNLOCK(env);
    }
}


extern int
gfX_imageDraw(Drawable win, GC gc, JNIEnv *env, jobject obj, Region clip,
	      jobject xorColor, jobject fgColor,
	      long x, long y,
	      long bx, long by, long bw, long bh,
	      jobject bgColor);

extern int
gfX_imageStretch(Drawable win, GC gc, JNIEnv *env, jobject obj, Region clip,
		 jobject xorColor, jobject fgColor,
		 long dx1, long dy1, long dx2, long dy2,
		 long sx1, long sy1, long sx2, long sy2,
		 jobject bgColor);

/* imageDraw and imageStretch, from ImageRepresentation            */
/* They make a bit more sense here, because of GRAPHICS_SETUP etc. */

/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    imageDraw
 * Signature: (Ljava/awt/Graphics;IILjava/awt/Color;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_image_ImageRepresentation_imageDraw(JNIEnv *env,
						 jobject this,
						 jobject obj /* gc */, 
						 jint x, jint y, 
						 jobject backgroundColor)
{
    jobject foreground = GETFIELD(Object, obj, graphicsImpl, foreground);
    jobject xorColor = GETFIELD(Object, obj, graphicsImpl, xorColor);
    jobject geometry = GETFIELD(Object, obj, graphicsImpl, geometry);

    /* Lock the drawing surface. */
    jint    validID  = CALLMETHOD(Int, geometry,
                                  sun_awt_DrawingSurfaceInfo, lock);
    jobject bounds   = CALLMETHOD(Object, geometry,
				  sun_awt_DrawingSurfaceInfo, getBounds);
    jobject region   = CALLMETHOD1(Object, obj, graphicsImpl, setupClip, bounds);
    Region clip = NULL;

    GRAPHICS_SETUP_WITH_CLIP(backgroundColor, data, x, y, &clip) else {
        NATIVE_UNLOCK(env);
	CALLMETHOD(Object, geometry, sun_awt_DrawingSurfaceInfo, unlock);
	return;
    }

    if (clip != NULL) {
	gfX_imageDraw(data->drawable, data->gc, env, this, clip,
		      xorColor, foreground,
		      x, y, 0, 0, -1, -1, backgroundColor);
    }

    GRAPHICS_CLEANUP(clip);
    if (validID == 0) {
	fprintf(stderr, "ValidID is 0.\n");
    }
    CALLMETHOD(Object, geometry, sun_awt_DrawingSurfaceInfo, unlock);
}

/*
 * Class:     sun_awt_image_ImageRepresentation
 * Method:    imageStretch
 * Signature: (Ljava/awt/Graphics;IIIIIIIILjava/awt/Color;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_image_ImageRepresentation_imageStretch(JNIEnv *env,
						    jobject this,
						    jobject obj /* gc */,
						    jint dx1, jint dy1, 
						    jint dx2, jint dy2,
						    jint sx1, jint sy1, 
						    jint sx2, jint sy2, 
						    jobject backgroundColor)
{
    jint xoff = 0, yoff = 0;
    long w, h;
    jobject foreground = GETFIELD(Object, obj, graphicsImpl, foreground);
    jobject xorColor = GETFIELD(Object, obj, graphicsImpl, xorColor);
    jobject geometry = GETFIELD(Object, obj, graphicsImpl, geometry);

    /* Lock the drawing surface. */
    jint    validID  = CALLMETHOD(Int, geometry,
                                  sun_awt_DrawingSurfaceInfo, lock);

    jobject bounds   = CALLMETHOD(Object, geometry,
				  sun_awt_DrawingSurfaceInfo, getBounds);
    jobject region   = CALLMETHOD1(Object, obj, graphicsImpl, setupClip, bounds);
    Region clip = NULL;

    GRAPHICS_SETUP_WITH_CLIP(backgroundColor, data, xoff, yoff, &clip) else {
        NATIVE_UNLOCK(env);
	CALLMETHOD(Object, geometry, sun_awt_DrawingSurfaceInfo, unlock);
	return;
    }

    if (clip != NULL) {
	dx1 += xoff; dx2 += xoff;
	dy1 += yoff; dy2 += yoff;

	w = dx2 - dx1;
	h = dy2 - dy1;
	if (w == sx2 - sx1 && h == sy2 - sy1) {
	    /* Make sure rectangles aren't upside down. */
	    if (w < 0) {
		dx1 = dx2;
		sx1 = sx2;
		w = -w;
	    }
	    if (h < 0) {
		dy1 = dy2;
		sy1 = sy2;
		h = -h;
	    }
	    gfX_imageDraw(data->drawable, data->gc, env, this, clip,
			  xorColor, foreground,
			  dx1, dy1, sx1, sy1, w, h, backgroundColor);
	} else {
	    gfX_imageStretch(data->drawable, data->gc, env, this, clip,
			     xorColor, foreground,
			     dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2,
			     backgroundColor);
	}
    }

    GRAPHICS_CLEANUP(clip);
    if (validID == 0) {
	fprintf(stderr, "validID is 0\n");
    }
    CALLMETHOD(Object, geometry, sun_awt_DrawingSurfaceInfo, unlock);
}

