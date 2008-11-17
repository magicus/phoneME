/*
 * @(#)awt_Graphics.c	1.116 06/10/10
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
#include <X11/keysym.h>

#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Rectangle.h"

#include "sun_awt_motif_X11Graphics.h"
#include "java_awt_Canvas.h"
#include "sun_awt_motif_MCanvasPeer.h"
#include "sun_awt_motif_MComponentPeer.h"

#include "awt_p.h"
#include "color.h"

#include "awt_Font.h"
#include "image.h"

#include "multi_font.h"


#define INIT_GC(env, disp, gdata, this)\
if (gdata==0 || (gdata->gc == 0 && !awt_init_gc(env, disp,gdata,this))) {\
    AWT_UNLOCK();\
    return;\
}

#define INIT_GC0(env, disp, gdata, this)\
if (gdata==0 || (gdata->gc == 0 && !awt_init_gc(env, disp,gdata,this))) {\
    return;\
}

#define INIT_GC2(env, disp, gdata, rval, this)\
if (gdata==0 || (gdata->gc == 0 && !awt_init_gc(env, disp,gdata,this))) {\
    AWT_UNLOCK();\
    return rval;\
}

#define POLYTEMPSIZE		(256 / sizeof(XPoint))

#define ABS(n)			(((n) < 0) ? -(n) : (n))
#define OX(env, this)		((*env)->GetIntField(env, this, MCachedIDs.X11Graphics_originXFID))
#define OY(env, this)		((*env)->GetIntField(env, this, MCachedIDs.X11Graphics_originYFID))
#define TX(env, this, x)	((x) + OX(env, this))
#define TY(env, this, y)	((y) + OY(env, this))

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.X11GraphicsClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.X11Graphics_originXFID = (*env)->GetFieldID(env, cls, "originX", "I");
	MCachedIDs.X11Graphics_originYFID = (*env)->GetFieldID(env, cls, "originY", "I");
	MCachedIDs.X11Graphics_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.X11Graphics_peerFID = (*env)->GetFieldID(env, cls, "peer", "Lsun/awt/motif/MComponentPeer;");
	MCachedIDs.X11Graphics_imageFID = (*env)->GetFieldID(env, cls, "image", "Lsun/awt/image/Image;");
}

static int
awt_init_gc(JNIEnv * env, Display * display, struct GraphicsData * gdata, jobject this)
{
	if (gdata->drawable == 0) {
		struct ComponentData *cdata;
		Widget          w;

		jobject         peer = (*env)->GetObjectField(env, this, MCachedIDs.X11Graphics_peerFID);

		if (peer == NULL) {
			/* this should never happen */
			return 0;
		}
		cdata = (struct ComponentData *) (*env)->GetIntField(env, peer, MCachedIDs.MComponentPeer_pDataFID);
		if (cdata == NULL) {
			/*
			 * native widget for component peer has been
			 * destroyed
			 */
			return 0;
		}
		w = cdata->widget;

		if (w != NULL && XtIsRealized(w)) {
			gdata->drawable = XtWindow(w);
		}
		if (gdata->drawable == 0) {
			return 0;
		}
	}

	if (gdata->gc == 0) {
	  int r;

		if ((gdata->gc = XCreateGC(display, gdata->drawable, 0, 0)) != 0) {
		        XSetGraphicsExposures(display, gdata->gc, False);
			r= 1;
		} else
		  {
			/* Could not allocate a GC for this Graphics. */
			r= 0;
		  }

		return r;
	} else
		/* Already have a GC for this Graphics. */
		return 1;

	/* If it gets here, something went wrong. */
	/* return 0; */
}

static void
awt_drawArc(JNIEnv * env, jobject this,
	    struct GraphicsData * gdata,
 jint x, jint y, jint w, jint h, jint startAngle, jint endAngle, int filled)
{
	long            s, e;

	if (w < 0 || h < 0) {
		return;
	}
	if (gdata == NULL) {
		gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

		INIT_GC0(env, awt_display, gdata, this);
	}
	if (endAngle >= 360 || endAngle <= -360) {
		s = 0;
		e = 360 * 64;
	} else {
		s = (startAngle % 360) * 64;
		e = endAngle * 64;
	}

	
	if (filled == 0) {
		XDrawArc(awt_display, gdata->drawable, gdata->gc, TX(env, this, x), TY(env, this, y), w, h, s, e);
	} else {
		XFillArc(awt_display, gdata->drawable, gdata->gc, TX(env, this, x), TY(env, this, y), w, h, s, e);
	}
	

}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_createFromComponent(JNIEnv * env, jobject this, jobject canvas)
{
	struct GraphicsData *gdata;
	struct ComponentData *cdata;

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *)XtCalloc(1, sizeof(struct GraphicsData));
	(*env)->SetIntField(env, this, MCachedIDs.X11Graphics_pDataFID, (jint) gdata);

	cdata = (struct ComponentData *) (*env)->GetIntField(env, canvas, MCachedIDs.MComponentPeer_pDataFID);

	if (gdata == NULL || cdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	gdata->gc = 0;
	gdata->drawable = XtWindow(cdata->widget);
	gdata->clipset = False;

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_createFromGraphics(JNIEnv * env, jobject this, jobject g)
{
	struct GraphicsData *odata;
	struct GraphicsData *gdata;

	AWT_LOCK();

	odata = (struct GraphicsData *) (*env)->GetIntField(env, g, MCachedIDs.X11Graphics_pDataFID);

	if (g == NULL || odata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	gdata = (struct GraphicsData *)XtCalloc(1, sizeof(struct GraphicsData));

	(*env)->SetIntField(env, this, MCachedIDs.X11Graphics_pDataFID, (jint) gdata);

	if (gdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	INIT_GC(env, awt_display, odata, this);

	gdata->drawable = odata->drawable;

	INIT_GC(env, awt_display, gdata, this);

	/* copy the gc */

      	XCopyGC(awt_display, odata->gc, GCForeground | GCBackground | GCFont | GCFunction, gdata->gc);

	/* Set the clip rect */
	gdata->clipset = odata->clipset;
	if (gdata->clipset) {
		gdata->cliprect = odata->cliprect;
		XSetClipRectangles(awt_display, gdata->gc, 0, 0, &gdata->cliprect, 1, YXBanded);
	}

	

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_imageCreate(JNIEnv * env, jobject this, jobject ir)
{
	struct GraphicsData *gdata;
	XID             pixmap;

	AWT_LOCK();
	if (this == NULL || ir == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	gdata = (struct GraphicsData *)XtCalloc(1, sizeof(struct GraphicsData));

	(*env)->SetIntField(env, this, MCachedIDs.X11Graphics_pDataFID, (jint) gdata);

	if (gdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	pixmap = image_getIRDrawable(env, ir);

	if (pixmap == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	gdata->gc = 0;
	gdata->drawable = pixmap;
	gdata->clipset = False;

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_pSetFont(JNIEnv * env, jobject this, jobject font)
{


}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_pSetForeground(JNIEnv * env, jobject this, jobject c)
{
	Pixel           p1;
	struct GraphicsData *gdata;

	if (c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	p1 = awt_getColor(env, c);
	gdata->fgpixel = p1;
	if (gdata->xormode) {
		p1 ^= gdata->xorpixel;
	}

	
	XSetForeground(awt_display, gdata->gc, p1);
	
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_disposeImpl(JNIEnv * env, jobject this)
{
	struct GraphicsData *gdata;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	if (gdata == NULL) {
		AWT_UNLOCK();
		return;
	}
	if (gdata->gc) {
    		XFreeGC(awt_display, gdata->gc);
		
	}

	XtFree((char *)gdata);

	(*env)->SetIntField(env, this, MCachedIDs.X11Graphics_pDataFID, (jint) 0);

	AWT_UNLOCK();
}


#ifdef NETSCAPE
#include "nspr/prcpucfg.h"
#endif


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawBytes(JNIEnv * env, jobject this, jobject font, jbyteArray text,
					 jint x, jint y)
{
	jbyte           *bytereg;
	int             blen;
	struct GraphicsData *gdata;
	XmString        xim;
	XmFontList      xfl;


	if (text == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	blen = (*env)->GetArrayLength(env, text);

	bytereg = (jbyte *)XtCalloc(blen+1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, text, 0, blen, bytereg);
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree(bytereg);

	xfl = getFontList(env, font);

	XmStringDraw(awt_display,
		     gdata->drawable,
		     xfl,
		     xim,
		     gdata->gc,
		     TX(env, this, x),
		     TY(env, this, y),
		     0,
		     XmALIGNMENT_BEGINNING,
		     XmSTRING_DIRECTION_L_TO_R,
		     NULL);

	XmStringFree(xim);

	/* AWT_FLUSH_UNLOCK(); */
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawLine(JNIEnv * env, jobject this, jint x1, jint y1, jint x2, jint y2)
{
	struct GraphicsData *gdata;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	

	XDrawLine(awt_display,
		  gdata->drawable,
		  gdata->gc, TX(env, this, x1), TY(env, this, y1), TX(env, this, x2), TY(env, this, y2));

	

	/* AWT_FLUSH_UNLOCK(); */
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_clearRect(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct GraphicsData *gdata;
	jobject         peer;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	if (gdata == NULL) {
		AWT_UNLOCK();
		return;
	}
	INIT_GC(env, awt_display, gdata, this);

	if (gdata->clipset) {
		long            cx, cy, cw, ch;

		cx = gdata->cliprect.x - OX(env, this);
		cy = gdata->cliprect.y - OY(env, this);
		cw = gdata->cliprect.width;
		ch = gdata->cliprect.height;
		if (x < cx) {
			w = x + w - cx;
			x = cx;
		}
		if (y < cy) {
			h = y + h - cy;
			y = cy;
		}
		if (x + w > cx + cw) {
			w = cx + cw - x;
		}
		if (y + h > cy + ch) {
			h = cy + ch - y;
		}
	}
	if (w <= 0 || h <= 0) {
		AWT_UNLOCK();
		return;
	}

	peer = (*env)->GetObjectField(env, this, MCachedIDs.X11Graphics_peerFID);
	if (peer == NULL) {

		/*
		 * this is an offscreen graphics object so we can't use
		 * XClearArea. Instead we fill a rectangle with the
		 * background color from the Component since graphics objects
		 * don't have a background color.
		 */
		extern Pixel    awt_white;
		GC              imagegc;

		jobject         image, bgh;

		imagegc = awt_getImageGC(gdata->drawable);

		image = (*env)->GetObjectField(env, this, MCachedIDs.X11Graphics_imageFID);
		bgh = (*env)->CallObjectMethod(env, image, MCachedIDs.sun_awt_image_Image_getBackgroundMID);

       		if (bgh!=NULL) {
			XSetForeground(awt_display, imagegc, awt_getColor(env, bgh));
			XFillRectangle(awt_display, gdata->drawable, imagegc, TX(env, this, x), TY(env, this, y), w, h);
			XSetForeground(awt_display, imagegc, awt_white);
		}

	} else {
		XClearArea(awt_display, gdata->drawable, TX(env, this, x), TY(env, this, y), w, h, False);
      	}


	/* AWT_FLUSH_UNLOCK(); */
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_fillRect(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct GraphicsData *gdata;

	if (w <= 0 || h <= 0) {
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	

	XFillRectangle(awt_display, gdata->drawable, gdata->gc, TX(env, this, x), TY(env, this, y), w, h);

	

	/* AWT_FLUSH_UNLOCK(); */
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawRect(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct GraphicsData *gdata;

	if (w < 0 || h < 0) {
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

      	XDrawRectangle(awt_display, gdata->drawable, gdata->gc, TX(env, this, x), TY(env, this, y), w, h);

	/* AWT_FLUSH_UNLOCK(); */
}



JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawRoundRect(JNIEnv * env, jobject this,
	      jint x, jint y, jint w, jint h, jint arcWidth, jint arcHeight)
{
	struct GraphicsData *gdata;
	jint            tx, txw, ty, ty1, ty2, tyh, tx1, tx2;

	if (w <= 0 || h <= 0) {
		return;
	}
	arcWidth = ABS(arcWidth);
	arcHeight = ABS(arcHeight);
	if (arcWidth > w)
		arcWidth = w;
	if (arcHeight > h)
		arcHeight = h;
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	tx = TX(env, this, x);
	txw = TX(env, this, x + w);
	ty = TY(env, this, y);
	tyh = TY(env, this, y + h);

	ty1 = TY(env, this, y + (arcHeight / 2));
	ty2 = TY(env, this, y + h - (arcHeight / 2));
	tx1 = TX(env, this, x + (arcWidth / 2));
	tx2 = TX(env, this, x + w - (arcWidth / 2));

	awt_drawArc(env, this, gdata, x, y, arcWidth, arcHeight, 90, 90, 0);
	awt_drawArc(env, this, gdata, x + w - arcWidth, y, arcWidth, arcHeight, 0, 90, 0);
	awt_drawArc(env, this, gdata, x, y + h - arcHeight, arcWidth, arcHeight, 180, 90, 0);
	awt_drawArc(env, this, gdata, x + w - arcWidth, y + h - arcHeight, arcWidth, arcHeight, 270, 90, 0);


      	XDrawLine(awt_display, gdata->drawable, gdata->gc, tx1 + 1, ty, tx2 - 1, ty);
	XDrawLine(awt_display, gdata->drawable, gdata->gc, txw, ty1 + 1, txw, ty2 - 1);

	XDrawLine(awt_display, gdata->drawable, gdata->gc, tx1 + 1, tyh, tx2 - 1, tyh);
	XDrawLine(awt_display, gdata->drawable, gdata->gc, tx, ty1 + 1, tx, ty2 - 1);

	

	/* AWT_FLUSH_UNLOCK(); */
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_fillRoundRect(JNIEnv * env, jobject this,
	      jint x, jint y, jint w, jint h, jint arcWidth, jint arcHeight)
{
	struct GraphicsData *gdata;
	jint            tx, txw, ty, ty1, ty2, tyh, tx1, tx2;

	if (w <= 0 || h <= 0) {
		return;
	}
	arcWidth = ABS(arcWidth);
	arcHeight = ABS(arcHeight);
	if (arcWidth > w)
		arcWidth = w;
	if (arcHeight > h)
		arcHeight = h;
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);
	tx = TX(env, this, x);
	txw = TX(env, this, x + w);
	ty = TY(env, this, y);
	ty1 = TY(env, this, y + (arcHeight / 2));
	ty2 = TY(env, this, y + h - (arcHeight / 2));
	tyh = TY(env, this, y + h);
	tx1 = TX(env, this, x + (arcWidth / 2));
	tx2 = TX(env, this, x + w - (arcWidth / 2));

	awt_drawArc(env, this, gdata, x, y, arcWidth, arcHeight, 90, 90, 1);
	awt_drawArc(env, this, gdata, x + w - arcWidth, y, arcWidth, arcHeight, 0, 90, 1);
	awt_drawArc(env, this, gdata, x, y + h - arcHeight, arcWidth, arcHeight, 180, 90, 1);
	awt_drawArc(env, this, gdata, x + w - arcWidth, y + h - arcHeight, arcWidth, arcHeight, 270, 90, 1);


      	XFillRectangle(awt_display, gdata->drawable, gdata->gc, tx1, ty, tx2 - tx1, tyh - ty);
	XFillRectangle(awt_display, gdata->drawable, gdata->gc, tx, ty1, tx1 - tx, ty2 - ty1);
	XFillRectangle(awt_display, gdata->drawable, gdata->gc, tx2, ty1, txw - tx2, ty2 - ty1);

	

	/* AWT_FLUSH_UNLOCK(); */
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_setPaintMode(JNIEnv * env, jobject this)
{
	struct GraphicsData *gdata;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);
	gdata->xormode = False;

       	XSetFunction(awt_display, gdata->gc, GXcopy);
	XSetForeground(awt_display, gdata->gc, gdata->fgpixel);

       	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_setXORMode(JNIEnv * env, jobject this, jobject c)
{
	Pixel           p1;
	struct GraphicsData *gdata;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	if (c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	INIT_GC(env, awt_display, gdata, this);
	p1 = awt_getColor(env, c);
	gdata->xorpixel = p1;
	p1 ^= gdata->fgpixel;
	gdata->xormode = True;

	
	XSetFunction(awt_display, gdata->gc, GXxor);
	XSetForeground(awt_display, gdata->gc, p1);

	
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_changeClip(JNIEnv * env, jobject this,
			       jint x, jint y, jint w, jint h, jboolean set)
{
	struct GraphicsData *gdata;
	int             x1, y1, x2, y2;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	/* compute the union */
	x1 = TX(env, this, x);
	y1 = TY(env, this, y);
	if (w <= 0 || h <= 0) {
		x2 = x1;
		y2 = y1;
	} else {
		x2 = x1 + w;
		y2 = y1 + h;
	}

	if (!set && gdata->clipset) {
		x1 = max(gdata->cliprect.x, x1);
		y1 = max(gdata->cliprect.y, y1);
		x2 = min(gdata->cliprect.x + gdata->cliprect.width, x2);
		y2 = min(gdata->cliprect.y + gdata->cliprect.height, y2);
		if (x1 > x2) {
			x2 = x1;
		}
		if (y1 > y2) {
			y2 = y1;
		}
	}
	gdata->cliprect.x = (Position) x1;
	gdata->cliprect.y = (Position) y1;
	gdata->cliprect.width = (Dimension) (x2 - x1);
	gdata->cliprect.height = (Dimension) (y2 - y1);
	gdata->clipset = True;

       	XSetClipRectangles(awt_display, gdata->gc, 0, 0, &gdata->cliprect, 1, YXBanded);

        AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_removeClip(JNIEnv * env, jobject this)
{
	struct GraphicsData *gdata;

	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	gdata->clipset = False;


       	XSetClipMask(awt_display, gdata->gc, None);


	AWT_UNLOCK();
}

JNIEXPORT jobject JNICALL
Java_sun_awt_motif_X11Graphics_getClipBounds(JNIEnv * env, jobject this)
{
	int             set;
	jint            x = 0, y = 0, w = 0, h = 0;
	jobject         clip = NULL;
	struct GraphicsData *gdata = NULL;

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);
	if (gdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return NULL;
	}
	/*
	 * Deadlock prevention: copy data to stack and then unlock for the
	 * constructor.
	 */

	AWT_LOCK();
	set = gdata->clipset;
	if (set) {
		x = gdata->cliprect.x - OX(env, this);
		y = gdata->cliprect.y - OY(env, this);
		w = gdata->cliprect.width;
		h = gdata->cliprect.height;
	}
	AWT_UNLOCK();

	if (set) {
		clip = (*env)->NewObject(env,
					 MCachedIDs.java_awt_RectangleClass,
		  MCachedIDs.java_awt_Rectangle_constructorMID, x, y, w, h);
	}
	return clip;
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_copyArea(JNIEnv * env, jobject this,
		  jint x, jint y, jint width, jint height, jint dx, jint dy)
{
	struct GraphicsData *gdata;

	if (width <= 0 || height <= 0) {
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);
	dx += x;
	dy += y;


      	XCopyArea(awt_display, gdata->drawable, gdata->drawable, gdata->gc,
		  TX(env, this, x), TY(env, this, y), width, height, TX(env, this, dx), TY(env, this, dy));

      	/* AWT_FLUSH_UNLOCK(); */
}


static XPoint  *
transformPoints(JNIEnv * env, jobject this,
		jintArray xcoords, jintArray ycoords, XPoint * points, jint * pNpoints, jboolean close)
{
	int             i;
	long            npoints = *pNpoints;
	jint           *x, *y;

	x = (*env)->GetIntArrayElements(env, xcoords, NULL);
	if ((x == NULL) || (*env)->ExceptionCheck(env))
		return 0;

	y = (*env)->GetIntArrayElements(env, ycoords, NULL);
	if ((y == NULL) || (*env)->ExceptionCheck(env)) {
		(*env)->ReleaseIntArrayElements(env, xcoords, x, JNI_ABORT);
		return 0;
	}
	if (close) {
		close = (npoints > 2 && (x[npoints - 1] != x[0] || y[npoints - 1] != y[0]));
		if (close) {
			npoints++;
			*pNpoints = npoints;
		}
	}
	if (npoints > POLYTEMPSIZE) {
		points = (XPoint *) XtMalloc(sizeof(XPoint) * npoints);
		if (points == NULL) {

			(*env)->ReleaseIntArrayElements(env, xcoords, x, JNI_ABORT);
			(*env)->ReleaseIntArrayElements(env, ycoords, y, JNI_ABORT);

			return 0;
		}
	}
	i = npoints;

	if (close) {
		--i;
		points[i].x = TX(env, this, x[0]);
		points[i].y = TY(env, this, y[0]);
	}
	while (--i >= 0) {
		points[i].x = TX(env, this, x[i]);
		points[i].y = TY(env, this, y[i]);
	}

	(*env)->ReleaseIntArrayElements(env, xcoords, x, JNI_ABORT);
	(*env)->ReleaseIntArrayElements(env, ycoords, y, JNI_ABORT);

	return points;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawPoly(JNIEnv * env, jobject this,
	 jintArray xcoords, jintArray ycoords, jint npoints, jboolean close)
{
	struct GraphicsData *gdata;
	XPoint          pTmp[POLYTEMPSIZE], *points;

	if (xcoords == 0 || ycoords == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	if ((*env)->GetArrayLength(env, ycoords) < npoints || (*env)->GetArrayLength(env, xcoords) < npoints) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		return;
	}
	points = transformPoints(env, this, xcoords, ycoords, pTmp, &npoints, close);
	if (points == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	
	XDrawLines(awt_display, gdata->drawable, gdata->gc, points, npoints, CoordModeOrigin);

       
	/* AWT_FLUSH_UNLOCK(); */

	if (points != pTmp) {
		XtFree((char *)points);
		points = NULL;
	}
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_fillPolygon(JNIEnv * env, jobject this,
			 jintArray xcoords, jintArray ycoords, jint npoints)
{
	struct GraphicsData *gdata;
	XPoint          pTmp[POLYTEMPSIZE], *points;

	if (xcoords == 0 || ycoords == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	if ((*env)->GetArrayLength(env, ycoords) < npoints || (*env)->GetArrayLength(env, xcoords) < npoints) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		return;
	}
	points = transformPoints(env, this, xcoords, ycoords, pTmp, &npoints, FALSE);
	if (points == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, this, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, this);

	XFillPolygon(awt_display, gdata->drawable, gdata->gc, points, npoints, Complex, CoordModeOrigin);

	/* AWT_FLUSH_UNLOCK(); */

	if (points != pTmp) {
		XtFree((char *)points);
		points = NULL;
	}
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawOval(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	AWT_LOCK();
	awt_drawArc(env, this, 0, x, y, w, h, 0, 360, 0);
	/* AWT_FLUSH_UNLOCK(); */
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_fillOval(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	AWT_LOCK();
	awt_drawArc(env, this, 0, x, y, w, h, 0, 360, 1);
	/* AWT_FLUSH_UNLOCK(); */
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_drawArc(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h,
				       jint startAngle, jint endAngle)
{
	AWT_LOCK();
	awt_drawArc(env, this, 0, x, y, w, h, startAngle, endAngle, 0);
	/* AWT_FLUSH_UNLOCK(); */
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Graphics_fillArc(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h,
				       jint startAngle, jint endAngle)
{
	AWT_LOCK();
	awt_drawArc(env, this, 0, x, y, w, h, startAngle, endAngle, 1);
	/* AWT_FLUSH_UNLOCK();  */
}

JNIEXPORT void  JNICALL
Java_sun_awt_image_ImageRepresentation_imageDraw(JNIEnv * env, jobject this,
				       jobject g, jint x, jint y, jobject c)
{
	struct GraphicsData *gdata;


	if (g == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	if (!(*env)->IsInstanceOf(env, g, MCachedIDs.X11GraphicsClass)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, g, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, g);

	if ((gdata->gc == 0) || (gdata->drawable == 0)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_imageDraw(env, this, c, gdata->drawable, gdata->gc,
		      gdata->xormode, gdata->xorpixel, gdata->fgpixel,
		      TX(env, g, x), TY(env, g, y), 0, 0, -1, -1, (gdata->clipset ? &gdata->cliprect : 0));

	/* AWT_FLUSH_UNLOCK(); */

}

JNIEXPORT void  JNICALL
Java_sun_awt_image_ImageRepresentation_imageStretch(JNIEnv * env, jobject this, jobject g,
						    jint dx1, jint dy1, jint dx2, jint dy2, jint sx1, jint sy1, jint sx2, jint sy2, jobject c)
{
	struct GraphicsData *gdata;
	jint            w, h;


	if (g == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	if (!(*env)->IsInstanceOf(env, g, MCachedIDs.X11GraphicsClass)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	gdata = (struct GraphicsData *) (*env)->GetIntField(env, g, MCachedIDs.X11Graphics_pDataFID);

	INIT_GC(env, awt_display, gdata, g);

	if ((gdata->gc == 0) || (gdata->drawable == 0)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
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
		awt_imageDraw(env, this, c, gdata->drawable, gdata->gc,
			    gdata->xormode, gdata->xorpixel, gdata->fgpixel,
			   TX(env, g, dx1), TY(env, g, dy1), sx1, sy1, w, h,
			      (gdata->clipset ? &gdata->cliprect : 0));
	} else {
		awt_imageStretch(env, this, c, gdata->drawable, gdata->gc,
			    gdata->xormode, gdata->xorpixel, gdata->fgpixel,
				 TX(env, g, dx1), TY(env, g, dy1), TX(env, g, dx2), TY(env, g, dy2),
		sx1, sy1, sx2, sy2, (gdata->clipset ? &gdata->cliprect : 0));
	}
	/* AWT_FLUSH_UNLOCK(); */

}
