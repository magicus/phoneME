/*
 * @(#)X11Image.c	1.16 06/10/10
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

#include <stdlib.h>

#include "awt.h"
#include "java_awt_X11Image.h"


JNIEXPORT void JNICALL
Java_java_awt_X11Image_initIDs(JNIEnv *env, jclass cls)
{
  FIND_CLASS("java/awt/image/IndexColorModel");
  GET_FIELD_ID(java_awt_image_IndexColorModel_rgbFID, "rgb", "[I");

  FIND_CLASS("java/awt/image/DirectColorModel");
  GET_FIELD_ID(java_awt_image_DirectColorModel_red_maskFID, "red_mask", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_red_offsetFID, "red_offset", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_red_scaleFID, "red_scale", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_green_maskFID, "green_mask", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_green_offsetFID, "green_offset", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_green_scaleFID, "green_scale", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_blue_maskFID, "blue_mask", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_blue_offsetFID, "blue_offset", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_blue_scaleFID, "blue_scale", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_alpha_maskFID, "alpha_mask", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_alpha_offsetFID, "alpha_offset", "I");
  GET_FIELD_ID(java_awt_image_DirectColorModel_alpha_scaleFID, "alpha_scale", "I");
}


JNIEXPORT jint JNICALL
Java_java_awt_X11Image_pCreatePixmap(JNIEnv *env, jobject this, jint width, jint height, jobject bg)
{
  Drawable newPixmap;
  GC tmpGC;

  newPixmap = XCreatePixmap(xawt_display, xawt_root, width, height, xawt_depth);

  tmpGC = XCreateGC(xawt_display, xawt_root, 0, 0);

  XSetForeground(xawt_display, tmpGC, xawt_whitePixel);
  XFillRectangle(xawt_display, newPixmap, tmpGC, 0, 0, width, height);
  XFreeGC(xawt_display, tmpGC);

  return (jint)newPixmap;

}


JNIEXPORT jint JNICALL
Java_java_awt_X11Image_pCreatePixmapImg(JNIEnv *env, jobject this, jint ximage, jint w, jint h)
{
  Drawable newPixmap;
  GC tmpGC;

  newPixmap = XCreatePixmap(xawt_display, xawt_root, w, h, xawt_depth);

  tmpGC = XCreateGC(xawt_display, xawt_root, 0, 0);

  XPutImage(xawt_display, newPixmap, tmpGC, (XImage *)ximage, 0, 0, 0, 0, w, h);

  XDestroyImage((XImage *)ximage);

  XFreeGC(xawt_display, tmpGC);

  return (jint)newPixmap;
}


JNIEXPORT jint JNICALL
Java_java_awt_X11Image_pGetSubPixmapImage(JNIEnv *env, jobject this, jint pixmap, jint x, jint y, jint w, jint h)
{

  char *newImageData = malloc(w*h*XBitmapPad(xawt_display));
  XImage *newImage = XCreateImage(xawt_display, xawt_visual, xawt_depth, ZPixmap, 0, newImageData, w, h, XBitmapPad(xawt_display), 0);

  XGetSubImage(xawt_display, (Drawable)pixmap, x, y, w, h, XAllPlanes(), ZPixmap, newImage, 0, 0);

  return (jint)newImage;
}


JNIEXPORT jint JNICALL
Java_java_awt_X11Image_pGetSubImage(JNIEnv *env, jobject this, jint ximage, jint x, jint y, jint w, jint h)
{
  XImage *newImage = XSubImage((XImage *)ximage, x, y, w, h);

  return (jint)newImage;
}




JNIEXPORT jint JNICALL
Java_java_awt_X11Image_pCreateClipmask(JNIEnv *env, jobject this, jint width, jint height)
{
  char *clipData = calloc(width*height, 1);
  XImage *newClip = XCreateImage(xawt_display, xawt_visual, 1, XYBitmap, 0, clipData,
				 width, height, 8, 0);

  XInitImage(newClip);

  return (jint)newClip;
}


JNIEXPORT jintArray JNICALL
Java_java_awt_X11Image_pGetPixels(JNIEnv *env, jobject this, jint src, jint clipmask, jintArray xpixels, jint x, jint y, jint w, jint h, jint scansize, jint offset)
{

  XImage *pixBuf;

  jint *xpixelArray;
  int len, i;
  int nx, ny;

  unsigned long c;

  len = w*h;

  pixBuf = XGetImage(xawt_display, (Drawable)src, x, y, w, h, XAllPlanes(), ZPixmap);

  xpixelArray = (jint *)malloc(len*sizeof(jint));

  for(i=0;i<len;i++) {
    nx = (x+i)%scansize;
    ny = y+(i/scansize);

    c = XGetPixel(pixBuf, nx-x, ny-y);

    xpixelArray[i] = (jint)colorDeCvtFunc(c, XGetPixel((XImage *)clipmask, nx, ny));
  
  }

  XDestroyImage(pixBuf);

  if(xpixels==NULL)
    xpixels = (*env)->NewIntArray(env, offset+len);

  (*env)->SetIntArrayRegion(env, xpixels, offset, len, xpixelArray);

  return xpixels;
}



JNIEXPORT void JNICALL
Java_java_awt_X11Image_pSetPixels(JNIEnv *env, jobject this, jint src, jint clipmask, jintArray xpixels, jint x, jint y, jint w, jint h, jint scansize)
{

  XImage *pixBuf;
  GC tmpGC;

  jint *xpixelArray;
  int len, i;
  int nx, ny;

  unsigned long c;
  char *pixBufData;

  if(xpixels==NULL) return;

  if((xpixelArray = (*env)->GetIntArrayElements(env, xpixels, NULL))==NULL) {
    (*env)->ExceptionDescribe(env);
    return;
  }

  len = w*h;

  pixBufData = (char *)malloc(len * XBitmapPad(xawt_display));
  pixBuf = XCreateImage(xawt_display, xawt_visual, xawt_depth, ZPixmap, 0, pixBufData, w, h, XBitmapPad(xawt_display), 0);
  XInitImage(pixBuf);


  for(i=0;i<len;i++) {
    nx = x+(i%scansize);
    ny = y+(i/scansize);

    c = (unsigned long)xpixelArray[i];

    XPutPixel(pixBuf, nx-x, ny-y, colorCvtFunc((c&0x00ff0000)>>16, (c&0x0000ff00)>>8, c&0x000000ff));

    if((c&0xff000000)<0xff000000) {

      XPutPixel((XImage *)clipmask, nx, ny, 0x01);

    }
  }

  tmpGC = XCreateGC(xawt_display, (Drawable)src, 0, 0);
  XPutImage(xawt_display, (Drawable)src, tmpGC, pixBuf, 0, 0, x, y, w, h);

  XFreeGC(xawt_display, tmpGC);

  XDestroyImage(pixBuf);

  (*env)->ReleaseIntArrayElements(env, xpixels, xpixelArray, JNI_ABORT);
}



JNIEXPORT jintArray JNICALL
Java_java_awt_X11Image_XIndexColorCvt__Ljava_awt_image_ColorModel_2_3BI(JNIEnv *env, jobject this, jobject colorModel, jbyteArray pixels, jint off)
{
		jobject rgbs;
		jint *rgbArray;
		jbyte *pixelArray;

		jint *rgbBuffer;

		jintArray newPixels;

		int i;
		unsigned long pixel;
		jsize len;

                rgbs = (*env)->GetObjectField (env, colorModel, XCachedIDs.java_awt_image_IndexColorModel_rgbFID);
                        
                if (rgbs == NULL)
		  return NULL;
                                
                if((rgbArray = (*env)->GetIntArrayElements (env, rgbs, NULL))==NULL) {
		  (*env)->ExceptionDescribe(env);
		  return NULL;
		}
                        
                if((pixelArray = (*env)->GetByteArrayElements (env, pixels, NULL))==NULL) {
		  (*env)->ReleaseIntArrayElements(env, rgbs, rgbArray, JNI_ABORT);
		  (*env)->ExceptionDescribe(env);
		  return NULL;
		}

                /* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

		len = (*env)->GetArrayLength(env, pixels);
		rgbBuffer = (jint *)calloc(len-off, sizeof(jint));

		for(i=off;i<len;i++) {

      		  pixel = (unsigned char)pixelArray[i];

		  rgbBuffer[i-off] = rgbArray[pixel];
		}

		if((newPixels = (*env)->NewIntArray(env, len-off))!=NULL)
		  (*env)->SetIntArrayRegion(env, newPixels, 0, len-off, rgbBuffer);
		else
		  (*env)->ExceptionDescribe(env);
		
		free(rgbBuffer);

		(*env)->ReleaseByteArrayElements(env, pixels, pixelArray, JNI_ABORT);
		(*env)->ReleaseIntArrayElements(env, rgbs, rgbArray, JNI_ABORT);
	
		return newPixels;
}



JNIEXPORT jintArray JNICALL
Java_java_awt_X11Image_XIndexColorCvt__Ljava_awt_image_ColorModel_2_3II(JNIEnv *env, jobject this, jobject colorModel, jintArray pixels, jint off)
{
		jobject rgbs;
		jint *rgbArray;
		jint *pixelArray;

		jint *rgbBuffer;

		jintArray newPixels;

		int i;
		unsigned long pixel;
		jsize len;

                rgbs = (*env)->GetObjectField (env, colorModel, XCachedIDs.java_awt_image_IndexColorModel_rgbFID);
                        
                if (rgbs == NULL)
		  return NULL;
                                
                if((rgbArray = (*env)->GetIntArrayElements (env, rgbs, NULL))==NULL) {
		  (*env)->ExceptionDescribe(env);
		  return NULL;
		}

                if((pixelArray = (*env)->GetIntArrayElements (env, pixels, NULL))==NULL) {
		  (*env)->ReleaseIntArrayElements(env, rgbs, rgbArray, JNI_ABORT);
		  (*env)->ExceptionDescribe(env);
		  return NULL;
		}

                /* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

		len = (*env)->GetArrayLength(env, pixels);
		rgbBuffer = (jint *)calloc(len-off, sizeof(jint));

		for(i=off;i<len;i++) {
      		  pixel = pixelArray[i];

		  rgbBuffer[i-off] = rgbArray[pixel];
		}

		if((newPixels = (*env)->NewIntArray(env, len-off))!=NULL)
		  (*env)->SetIntArrayRegion(env, newPixels, 0, len-off, rgbBuffer);
		else
		  (*env)->ExceptionDescribe(env);
		
		free(rgbBuffer);

		(*env)->ReleaseIntArrayElements(env, pixels, pixelArray, JNI_ABORT);
		(*env)->ReleaseIntArrayElements(env, rgbs, rgbArray, JNI_ABORT);
	
		return newPixels;
}



JNIEXPORT jintArray JNICALL
Java_java_awt_X11Image_XDirectColorCvt(JNIEnv *env, jobject this, jobject colorModel, jintArray pixels, jint off)
{
		jint *pixelArray;
		jint *rgbBuffer;

		jintArray newPixels;

		int i;
		jsize len;
		unsigned long pixel;

                jint red_mask, red_offset, red_scale;
                jint green_mask, green_offset, green_scale;
                jint blue_mask, blue_offset, blue_scale;
                jint alpha_mask, alpha_offset, alpha_scale;
                jint alpha, red, green, blue;
                
                red_mask = (*env)->GetIntField (env, colorModel
, XCachedIDs.java_awt_image_DirectColorModel_red_maskFID);
                red_offset = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_red_offsetFID);
                red_scale = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_red_scaleFID);
                

                green_mask = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_green_maskFID);
                green_offset = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_green_offsetFID);
                green_scale = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_green_scaleFID);
                
                blue_mask = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_blue_maskFID);
                blue_offset = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_blue_offsetFID);
                blue_scale = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_blue_scaleFID);
                
                alpha_mask = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_alpha_maskFID);
                alpha_offset = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_alpha_offsetFID);
                alpha_scale = (*env)->GetIntField (env, colorModel, XCachedIDs.java_awt_image_DirectColorModel_alpha_scaleFID);
                
                /* For each pixel in the supplied pixel array look up its rgb value from the c
olorModel. */


                if((pixelArray = (*env)->GetIntArrayElements (env, pixels, NULL))==NULL) {
		  (*env)->ExceptionDescribe(env);
		  return NULL;
		}

                /* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

		len = (*env)->GetArrayLength(env, pixels);
		rgbBuffer = (jint *)malloc((len-off)*sizeof(jint));

		/*
		red_scale = red_scale?red_scale<<8:0xFF;
		green_scale = green_scale?green_scale<<8:0xFF;
		blue_scale = blue_scale?blue_scale<<8:0xFF;
		alpha_scale = alpha_scale?alpha_scale<<8:0xFF;
		*/

		for(i=off;i<len;i++) {
      		  pixel = pixelArray[i];

		  red = ((pixel & red_mask) >> red_offset);
		  green = ((pixel & green_mask) >> green_offset);
		  blue = ((pixel & blue_mask) >> blue_offset);
                  alpha = ~((pixel & alpha_mask) >> alpha_offset);

		  /*
		  red /= red_scale;
		  green /= green_scale;
		  blue /= blue_scale;
		  alpha /= alpha_scale;
		  */
                                                                       
		  rgbBuffer[i-off] = (alpha<<24)|(red<<16)|(green<<8)|blue;
		}

		if((newPixels = (*env)->NewIntArray(env, len-off))!=NULL)
		  (*env)->SetIntArrayRegion(env, newPixels, 0, len-off, rgbBuffer);
		else
		  (*env)->ExceptionDescribe(env);
		
		free(rgbBuffer);

		(*env)->ReleaseIntArrayElements(env, pixels, pixelArray, JNI_ABORT);

		return newPixels;
}


