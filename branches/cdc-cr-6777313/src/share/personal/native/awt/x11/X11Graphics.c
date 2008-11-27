/*
 * @(#)X11Graphics.c	1.25 06/10/10
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


#include "awt.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "java_awt_X11Graphics.h"


JNIEXPORT jint JNICALL
Java_java_awt_X11Graphics_createGC(JNIEnv *env, jobject this, jint drawable)
{
    GC gc = XCreateGC(xawt_display, (Drawable)drawable, 0, 0);

    if(gc==0)
      (*env)->ThrowNew (env, XCachedIDs.OutOfMemoryError, NULL);

    return (jint)gc;
}


JNIEXPORT jint JNICALL
Java_java_awt_X11Graphics_copyGC(JNIEnv *env, jobject this, jint drawable, jint gc)
{
    GC newgc;
    
    if (gc == 0)
    {
    	(*env)->ThrowNew (env, XCachedIDs.NullPointerException, "gc is null");
    	return 0;
    }

    newgc = XCreateGC(xawt_display, (Drawable)drawable, 0, 0);

    if(newgc==0)
    {
    	(*env)->ThrowNew (env, XCachedIDs.OutOfMemoryError, NULL);
    	return 0;
    }


    /* Check copy was good */
    XCopyGC(xawt_display, (GC)gc, GCForeground | GCBackground | GCFont | GCFunction | GCClipMask, newgc);

    return (jint)newgc;
}



JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_destroyGC(JNIEnv *env, jobject this, jint gc)
{
    XFreeGC(xawt_display, (GC)gc);

}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_disposeImpl(JNIEnv *env, jobject this)
{

}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pSetFont(JNIEnv *env, jobject this, jint gc, jbyteArray fName)
{



}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pSetForeground(JNIEnv *env, jobject this, jint gc, jobject color)
{
  XSetForeground(xawt_display, (GC)gc, xawt_getPixel(env, color));
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pSetPaintMode(JNIEnv *env, jobject this, jint gc, jobject color)
{
    XSetFunction(xawt_display, (GC)gc, GXcopy);
    XSetForeground(xawt_display, (GC)gc, xawt_getPixel(env, color));
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pSetXORMode(JNIEnv *env, jobject this, jint gc, jobject color)
{
    XSetFunction(xawt_display, (GC)gc, GXxor);
    XSetForeground(xawt_display, (GC)gc, xawt_getPixel(env, color));
}




JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pClearRect(JNIEnv *env, jobject this, jint drawable, jint x, jint y, jint w, jint h)
{
    XClearArea(xawt_display, (Drawable)drawable, x, y, w, h, False);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pFillRect(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h)
{
    XFillRectangle(xawt_display, (Drawable)drawable, (GC)gc, x, y, w, h);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawRect(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h)
{
    XDrawRectangle(xawt_display, (Drawable)drawable, (GC)gc, x, y, w, h);
}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pCopyArea(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h, jint dx, jint dy)
{

    XSetGraphicsExposures(xawt_display, (GC)gc, False);

    XCopyArea(xawt_display, (Drawable)drawable, drawable, (GC)gc, x, y, w, h, dx, dy);

    XFlush(xawt_display);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawLine(JNIEnv *env, jobject this, jint drawable, jint gc, jint x1, jint y1, jint x2, jint y2)
{
    XDrawLine(xawt_display, (Drawable)drawable, (GC)gc, x1, y1, x2, y2);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawPolygon(JNIEnv *env, jobject this, jint drawable, jint gc, jintArray xp, jintArray yp, jint nPoints)
{
    XPoint *points;
    jint  *xpoints, *ypoints;
    int i;

    points = (XPoint *)calloc(nPoints, sizeof(XPoint));

    xpoints = (*env)->GetIntArrayElements(env, xp, NULL);
    ypoints = (*env)->GetIntArrayElements(env, yp, NULL);

    for(i=0;i<nPoints;i++) {
      points[i].x = xpoints[i];
      points[i].y = ypoints[i];
    }

    XDrawLines(xawt_display, (Drawable)drawable, (GC)gc, points, nPoints, CoordModeOrigin);

    (*env)->ReleaseIntArrayElements(env, xp, xpoints, JNI_ABORT);
    (*env)->ReleaseIntArrayElements(env, yp, ypoints, JNI_ABORT);

    free(points);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pFillPolygon(JNIEnv *env, jobject this, jint drawable, jint gc, jintArray xp, jintArray yp, jint nPoints)
{
    XPoint *points;
    jint *xpoints, *ypoints;
    int i;

    points = (XPoint *)calloc(nPoints, sizeof(XPoint));

    xpoints = (*env)->GetIntArrayElements(env, xp, NULL);
    ypoints = (*env)->GetIntArrayElements(env, yp, NULL);

    for(i=0;i<nPoints;i++) {
      points[i].x = xpoints[i];
      points[i].y = ypoints[i];
    }

    XFillPolygon(xawt_display, (Drawable)drawable, (GC)gc, points, nPoints, Complex, CoordModeOrigin);

    (*env)->ReleaseIntArrayElements(env, xp, xpoints, JNI_ABORT);
    (*env)->ReleaseIntArrayElements(env, yp, ypoints, JNI_ABORT);

    free(points);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawArc(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h, jint start, jint end)
{

    XDrawArc(xawt_display, (Drawable)drawable, (GC)gc, x, y, w, h, start, end);

}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pFillArc(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h, jint start, jint end)
{
	XFillArc(xawt_display, (Drawable)drawable, (GC)gc, x, y, w, h, start, end);
}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawRoundRect(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h, jint arcW, jint arcH)
{
    XArc arcs[4];
    XSegment lines[4];

    int xlineShort = (w - arcW)/2;
    int ylineShort  = (h - arcH)/2;

  
    arcs[0].x = x;
    arcs[0].y = y;
    arcs[0].width = arcW;
    arcs[0].height = arcH;
    arcs[0].angle1 = 90*64;
    arcs[0].angle2 = 90*64;

    arcs[1].x = x;
    arcs[1].y = y+h-arcH;
    arcs[1].width = arcW;
    arcs[1].height = arcH;
    arcs[1].angle1 = 180*64;
    arcs[1].angle2 = 90*64;

    arcs[2].x = x+w-arcW;
    arcs[2].y = y+h-arcH;
    arcs[2].width = arcW;
    arcs[2].height = arcH;
    arcs[2].angle1 = 270*64;
    arcs[2].angle2 = 90*64;

    arcs[3].x = x-arcW;
    arcs[3].y = y;
    arcs[3].width = arcW;
    arcs[3].height = arcH;
    arcs[3].angle1 = 0*64;
    arcs[3].angle2 = 90*64;


    lines[0].x1 = x+xlineShort;
    lines[0].y1 = y;
    lines[0].x2 = (x+w)-xlineShort;
    lines[0].y2 = y;

    lines[1].x1 = x;
    lines[1].y1 = y+ylineShort;
    lines[1].x2 = x;
    lines[1].y2 = (y+h)-ylineShort;

    lines[2].x1 = x+xlineShort;
    lines[2].y1 = y+h;
    lines[2].x2 = (x+w)-xlineShort;
    lines[2].y2 = y+h;

    lines[3].x1 = x+w;
    lines[3].y1 = y+ylineShort;
    lines[3].x2 = x+w;
    lines[3].y2 = (y+h)-ylineShort;

    XDrawArcs(xawt_display, (Drawable)drawable, (GC)gc, arcs, 4);
    XDrawSegments(xawt_display, (Drawable)drawable, (GC)gc, lines, 4);

}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pFillRoundRect(JNIEnv *env, jobject this, jint drawable, jint gc, jint x, jint y, jint w, jint h, jint arcW, jint arcH)
{
    XArc arcs[4];
    XPoint points[12];

    int xlineShort = (w - arcW)/2;
    int ylineShort  = (h - arcH)/2;


    arcs[0].x = x;
    arcs[0].y = y;
    arcs[0].width = arcW;
    arcs[0].height = arcH;
    arcs[0].angle1 = 90*64;
    arcs[0].angle2 = 90*64;

    arcs[1].x = x;
    arcs[1].y = y+h-arcH;
    arcs[1].width = arcW;
    arcs[1].height = arcH;
    arcs[1].angle1 = 180*64;
    arcs[1].angle2 = 90*64;

    arcs[2].x = x+w-arcW;
    arcs[2].y = y+h-arcH;
    arcs[2].width = arcW;
    arcs[2].height = arcH;
    arcs[2].angle1 = 270*64;
    arcs[2].angle2 = 90*64;

    arcs[3].x = x-arcW;
    arcs[3].y = y;
    arcs[3].width = arcW;
    arcs[3].height = arcH;
    arcs[3].angle1 = 0*64;
    arcs[3].angle2 = 90*64;


    points[0].x = x+xlineShort;
    points[0].y = y;
    points[1].x = x+xlineShort;
    points[1].y = y+ylineShort;
    points[2].x = x;
    points[2].y = y+ylineShort;

    points[3].x = x;
    points[3].y = (y+h)-ylineShort;
    points[4].x = x+xlineShort;
    points[4].y = (y+h)+ylineShort;
    points[5].x = x+xlineShort;
    points[5].y = y+h;

    points[6].x = (x+w)-xlineShort;
    points[6].y = (y+h);
    points[7].x = (x+w)-xlineShort;
    points[7].y = (y+h)-ylineShort;
    points[8].x = x+w;
    points[8].y = (y+h)-ylineShort;

    points[9].x = x+w;
    points[9].y = y+ylineShort;
    points[10].x = (x+w)-xlineShort;
    points[10].y = y+ylineShort;
    points[11].x = (x+w)-xlineShort;
    points[11].y = y;

    XDrawArcs(xawt_display, (Drawable)drawable, (GC)gc, arcs, 4);
    XFillPolygon(xawt_display, (Drawable)drawable, (GC)gc, points, 12, Nonconvex, CoordModeOrigin);

}



JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawBytes(JNIEnv *env, jobject this, jint drawable, jint gc, jint fontset, jbyteArray string, jint stringLen, jint x, jint y)
{
    jbyte *str;

    if(fontset==0) return;

    str = (*env)->GetByteArrayElements(env, string, NULL);

    XmbDrawString(xawt_display, (Drawable)drawable, (XFontSet)fontset, (GC)gc, x, y, (char *)str, stringLen);

    (*env)->ReleaseByteArrayElements(env, string, str, JNI_ABORT);

}

JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pChangeClipRect(JNIEnv *env, jobject this, jint gc, jint x, jint y, jint w, jint h)
{
   XRectangle clip[1];

   clip[0].x = (short)x;
   clip[0].y = (short)y;
   clip[0].width = (short)w;
   clip[0].height = (short)h;

   XSetClipRectangles(xawt_display, (GC)gc, 0, 0, clip, 1, Unsorted);

}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pRemoveClip(JNIEnv *env, jobject this, jint gc)
{
    XSetClipMask(xawt_display, (GC)gc, None);
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawImage(JNIEnv *env, jobject this, jint src, jint dest, jint clipmask, jint x, jint y, jint w, jint h, jint sx, jint sy) {

  XImage *clip = (XImage *)clipmask;
  Drawable clipPix = XCreatePixmap(xawt_display, xawt_root, w, h, 1);
  GC tmpGC = XCreateGC(xawt_display, (Drawable)dest, 0, 0);
  GC clipGC = XCreateGC(xawt_display, (Drawable)clipPix, 0, 0);

  XPutImage(xawt_display, clipPix, clipGC, clip, sx, sy, 0, 0, w, h);

  XSetClipMask(xawt_display, tmpGC, clipPix);
  XSetClipOrigin(xawt_display, tmpGC, x, y);
  XSetGraphicsExposures(xawt_display, tmpGC, False);

  XCopyArea(xawt_display, (Drawable)src, (Drawable)dest, (GC)tmpGC, sx, sy, w, h, x, y);

  XFreeGC(xawt_display, tmpGC);
  XFreeGC(xawt_display, clipGC);
  XFreePixmap(xawt_display, clipPix);

  XFlush(xawt_display);

}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawImageBG(JNIEnv *env, jobject this, jint src, jint dest, jint clipmask, jint x, jint y, jint w, jint h, jint sx, jint sy, jobject color) {

  XImage *clip = (XImage *)clipmask;
  Drawable clipPix = XCreatePixmap(xawt_display, xawt_root, w, h, 1);
  GC clipGC = XCreateGC(xawt_display, clipPix, 0, 0);
  GC tmpGC = XCreateGC(xawt_display, (Drawable)dest, 0, 0);

  if(color!=NULL) {
    XSetForeground(xawt_display, tmpGC, xawt_getPixel(env, color));
    XFillRectangle(xawt_display, (Drawable)dest, tmpGC, x, y, w, h);
  }

  XPutImage(xawt_display, clipPix, clipGC, clip, sx, sy, 0, 0, w, h);

  XSetClipMask(xawt_display, tmpGC, clipPix);
  XSetClipOrigin(xawt_display, tmpGC, x, y);
  XSetGraphicsExposures(xawt_display, tmpGC, False);

  XCopyArea(xawt_display, (Drawable)src, (Drawable)dest, (GC)tmpGC, sx, sy, w, h, x, y);

  XFreeGC(xawt_display, tmpGC);
  XFreeGC(xawt_display, clipGC);
  XFreePixmap(xawt_display, clipPix);

  XFlush(xawt_display);

}


static void DrawImageScale(Drawable src, Drawable dest, XImage *clipmask, GC newgc, int x, int y, int w, int h, int newW, int newH, int sx, int sy, int clipW, int clipH)
{

  int xCount, yCount;
  int ox, oy;
  int pox, poy;

  XImage *img;

  XImage *newImg;
  XImage *newClip;

  GC clipGC;

  char *newImgData;
  char *newClipData;

  Drawable clipPix;
  Pixel c, clipc;


  img = XGetImage(xawt_display, (Drawable)src, sx, sy, w-sx, h-sy, XAllPlanes(), ZPixmap);

  newImgData = (char *)malloc(clipW*clipH*(img->bitmap_pad/8));
  newImg = XCreateImage(xawt_display, xawt_visual, img->depth,
			img->format, 0, newImgData, clipW, clipH, img->bitmap_pad, 0);
  XInitImage(newImg);

  newClipData = (char *)calloc(clipW*clipH, 1);
  newClip = XCreateImage(xawt_display, xawt_visual, 1, XYBitmap, 0, newClipData,
				 clipW, clipH, 8, 0);
  XInitImage(newClip);

  pox = -1;
  poy = -1;
  c = 0;
  clipc = 0;
  for(yCount=0;yCount<clipH;yCount++) {

      oy = (yCount * h) / newH;

      for(xCount=0;xCount<clipW;xCount++) {

	ox = (xCount * w) / newW;

	if((pox!=ox)||(poy!=oy)) {
	  c = XGetPixel(img, ox, oy);

	  clipc = XGetPixel(clipmask, ox, oy);

	  pox = ox;
	  poy = oy;
	}

	  XPutPixel(newImg, xCount, yCount, c);

	  if(clipc!=xawt_blackPixel)
	    XPutPixel(newClip, xCount, yCount, 0xFF);
      }
  }

  clipPix = XCreatePixmap(xawt_display, xawt_root, clipW, clipH, 1);
  clipGC = XCreateGC(xawt_display, clipPix, 0, 0);

  XPutImage(xawt_display, clipPix, clipGC, newClip, 0, 0, 0, 0, clipW, clipH);
  XSetClipMask(xawt_display, newgc, clipPix);
  XSetClipOrigin(xawt_display, newgc, x, y);

  XPutImage(xawt_display, dest, newgc, newImg, 0, 0, x, y, clipW, clipH);

  XFreePixmap(xawt_display, clipPix);
  XFreeGC(xawt_display, clipGC);

  XDestroyImage(img);

  XDestroyImage(newClip);
  XDestroyImage(newImg);

}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawImageScale(JNIEnv *env, jobject this, jint src, jint dest, jint clipmask, jint gc, jint x, jint y, jint w, jint h, jint newW, jint newH, jint sx, jint sy, jint clipW, jint clipH) {

  GC newGC = XCreateGC(xawt_display, (Drawable)src, 0, 0);

  XCopyGC(xawt_display, (GC)gc, GCForeground | GCBackground | GCFont | GCFunction, newGC);

  DrawImageScale((Drawable)src, (Drawable)dest, (XImage *)clipmask, newGC, x, y, w, h, newW, newH, sx, sy, clipW, clipH);

  XFreeGC(xawt_display, newGC);

}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawImageScaleBG(JNIEnv *env, jobject this, jint src, jint dest, jint clipmask, jint gc, jint x, jint y, jint w, jint h, jint newW, jint newH, jint sx, jint sy, jint clipW, jint clipH, jobject color) {

  GC newGC = XCreateGC(xawt_display, (Drawable)src, 0, 0);

  XCopyGC(xawt_display, (GC)gc, GCForeground | GCBackground | GCFont | GCFunction, newGC);

  if(color!=NULL) {
    XSetForeground(xawt_display, newGC, xawt_getPixel(env, color));
    XFillRectangle(xawt_display, (Drawable)dest, newGC, x, y, w, h);
  }

  DrawImageScale((Drawable)src, (Drawable)dest, (XImage *)clipmask, newGC, x, y, w, h, newW, newH, sx, sy, clipW, clipH);
  XFreeGC(xawt_display, newGC);


}


static void DrawImageSub(Drawable src, Drawable dest, XImage *clipmask, GC newgc, int x1, int y1, int x2, int y2, int nx1, int ny1, int nx2, int ny2, int originx, int originy) {


  XImage *img;
  XImage *newImg;
  XImage *newClip;

  Pixel c, clipc;

  Drawable clipPix;
  GC clipGC;

  char *newImgData;
  char *newClipData;

  int xDir, yDir, nxDir, nyDir;
  int w, h, newW, newH;
  int ox, oy;
  int pox, poy;
  int xCount, yCount;
  int oxCount, oyCount;
  int poxCount, poyCount;
  int offNx, offNy;

  int tmp;

  xDir = x2-x1>0?1:-1;
  yDir = y2-y1>0?1:-1;
  nxDir = nx2-nx1>0?1:-1;
  nyDir = ny2-ny1>0?1:-1;

  w = (x2-x1)*xDir+1;
  h = (y2-y1)*yDir+1;
  newW = (nx2-nx1)*nxDir+1;
  newH = (ny2-ny1)*nyDir+1;

  offNx = (nxDir==1)?nx1:nx2;
  offNy = (nyDir==1)?ny1:ny2;

  img = XGetImage(xawt_display, (Drawable)src, (xDir>0)?x1:x2, (yDir>0)?y1:y2, w, h, XAllPlanes(), ZPixmap);

  newImgData = (char *)calloc(newW*newH, img->bitmap_pad/8);

  newImg = XCreateImage(xawt_display, xawt_visual, img->depth,
			img->format, 0, newImgData, newW, newH, img->bitmap_pad, 0);
  XInitImage(newImg);

  newClipData = (char *)calloc(newW*newH, 1);
  newClip = XCreateImage(xawt_display, xawt_visual, 1, XYBitmap, 0, newClipData, newW, newH, 8, 0);
  XInitImage(newClip);

  nx2+=nxDir;
  ny2+=nyDir;

  poy = (ny1*h)/newH;

  poxCount = -1;
  poyCount = -1;

  tmp = 0;

  for(yCount=ny1, oyCount=y1;yCount!=ny2;yCount+=nyDir) {

      pox = (nx1*w)/newW;

      for(xCount=nx1,oxCount=x1;xCount!=nx2;xCount+=nxDir) {

	if((poxCount!=oxCount)||(poyCount!=oyCount)) {

	  c = XGetPixel(img, oxCount, oyCount);

	  clipc = XGetPixel(clipmask, oxCount, oyCount);

	  poxCount = oxCount;
	  poyCount = oyCount;

	}

	XPutPixel(newImg, xCount-offNx, yCount-offNy, c);

	if(clipc!=xawt_blackPixel)
	  XPutPixel(newClip, xCount-offNx, yCount-offNy, 0xFF);

	ox = (xCount*w)/newW;

	oxCount+=xDir*nxDir*(ox-pox);
	pox = ox;

	tmp++;

      }

      oy = (yCount*h)/newH;

      oyCount+=yDir*nyDir*(oy-poy);
      poy = oy;

  }

  clipPix = XCreatePixmap(xawt_display, xawt_root, newW, newH, 1);
  clipGC = XCreateGC(xawt_display, clipPix, 0, 0);

  XPutImage(xawt_display, clipPix, clipGC, newClip, 0, 0, 0, 0, newW, newH);

  XSetClipMask(xawt_display, newgc, clipPix);
  XSetClipOrigin(xawt_display, newgc, originx+offNx, originy+offNy);

  XPutImage(xawt_display, (Drawable)dest, (GC)newgc, newImg, 0, 0, originx+offNx, originy+offNy, newW, newH);

  XFreePixmap(xawt_display, clipPix);
  XFreeGC(xawt_display, clipGC);

  XDestroyImage(img);
  XDestroyImage(newImg);

  XDestroyImage(newClip);

}




JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawImageSub(JNIEnv *env, jobject this, jint src, jint dest, jint clipmask, jint gc, jint x1, jint y1, jint x2, jint y2, jint nx1, jint ny1, jint nx2, jint ny2, jint originX, jint originY) {


  GC newGC = XCreateGC(xawt_display, (Drawable)src, 0, 0);

  XCopyGC(xawt_display, (GC)gc, GCForeground | GCBackground | GCFont | GCFunction, newGC);

  DrawImageSub((Drawable)src, (Drawable)dest, (XImage *)clipmask, newGC, x1, y1, x2, y2, nx1, ny1, nx2, ny2, originX, originY);

  XFreeGC(xawt_display, newGC);

 
}


JNIEXPORT void JNICALL
Java_java_awt_X11Graphics_pDrawImageSubBG(JNIEnv *env, jobject this, jint src, jint dest, jint clipmask, jint gc, jint x1, jint y1, jint x2, jint y2, jint nx1, jint ny1, jint nx2, jint ny2, jint originX, jint originY, jobject color) {


  GC newGC = XCreateGC(xawt_display, (Drawable)src, 0, 0);

  int nxDir = (nx2-nx1)>0?1:-1;
  int nyDir = (ny2-ny1)>0?1:-1;

  int w = (nx2-nx1)*nxDir;
  int h = (ny2-ny1)*nyDir;

  XCopyGC(xawt_display, (GC)gc, GCForeground | GCBackground | GCFont | GCFunction, newGC);

  if(color!=NULL) {
    XSetForeground(xawt_display, newGC, xawt_getPixel(env, color));
    XFillRectangle(xawt_display, (Drawable)dest, newGC, (nxDir==1)?nx1:nx2, (nyDir==1)?ny1:ny2, w, h);
  }

  DrawImageSub((Drawable)src, (Drawable)dest, (XImage *)clipmask, newGC, x1, y1, x2, y2, nx1, ny1, nx2, ny2, originX, originY);

  XFreeGC(xawt_display, newGC);

}




