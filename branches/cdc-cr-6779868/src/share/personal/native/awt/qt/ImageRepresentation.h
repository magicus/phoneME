/*
 * @(#)ImageRepresentation.h	1.7 06/10/25
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
#ifndef _IMAGE_REPRESENTATION_H_
#define _IMAGE_REPRESENTATION_H_

#include <qimage.h>
#include <qpixmap.h>
#include <qcolor.h>
#include <jni.h>
#include "QtGraphics.h"
#include "java_awt_image_ImageObserver.h"

class ImageRepresentation
{
 public:
    ~ImageRepresentation();

    static ImageRepresentation* getImageRepresentationFromJNI(JNIEnv* env,
                                                          jobject thisObj,
                                                          jobject background,
                                                          QImage *image);
    static ImageRepresentation* getImageRepresentationFromJNI(JNIEnv* env,
                                                          jobject thisObj,
                                                          jobject background);
    static ImageRepresentation* createImageRepresentationFromJNI(JNIEnv* env,
                                                              jobject thisObj,
                                                              jobject background);
    static ImageRepresentation
	*getImageRepresentationForSetPixels(JNIEnv *env, 
					    jobject thisObj, 
					    jint x,
					    jint y,
					    jint w, 
					    jint h,
					    jobject colorModel,
					    jobject pixels,
					    jint offset,
					    jint scansize);
    static bool getDataForImageDrawing(JNIEnv *env,
				       jobject thisObj,
				       jobject g,
				       ImageRepresentation **imgRep,
				       QtGraphics **graphics);

    static jint defaultImageType();
    static jobject defaultColorModel(JNIEnv *env);

    void drawUnscaledImage(JNIEnv *env, 
			   QtGraphics *graphicsData, 
			   int xsrc, int ysrc, 
			   int xdest, int ydest, 
			   int width, int height);

    QPixmap *getPixmap(void);
    void setPixmap(QPixmap *pixmap);

    QImage *getImage(void);
    void setImage(QImage *image);

    QColor getBackground(void);
    void setBackground(const QColor &color);

    int getWidth(void);
    void setWidth(int width);
    int getHeight(void);
    void setHeight(int height);

    int getNumLines(void);
    void setNumLines(int numLines);

    bool isDirtyPixmap();
    void setDirtyPixmap(bool isDirty);
    bool isDirtyImage();
    void setDirtyImage(bool isDirty);

    void updatePixmap();
    void updateImage();

    void setPixel(int x, int y, int red, int green, int blue, 
		  int alpha);
    void finish();
    
 private:
    static jobject defaultIndexColorModel(JNIEnv *env, jint type, int depth);

    ImageRepresentation(int width, int height, QPixmap *pixmap, QImage
			*image);
    /* static void scaleImage(QImage *srcImage, QImage *destImage, double
       scaleX, double scaleY); */

    /* The pixmap that represents this image. 
       A pixmap is an offscreen image on the server side. */
    QPixmap *pixmap;
    bool dirtyPixmap;
    
    /* QImage is used in order to avail the direct pixel 
       access/manipulation feature it provides.*/       
    QImage *image;
    bool dirtyImage;
    
    /* The width and height of the pixmap. */    
    int width, height;	
    
    /* The number of lines that have been stored in the pixbuf so far. */    
    int numLines;

    /* The default background color, used for offscreen image in
       QtGraphics.clearRect(). */
    QColor backgroundColor;
    
    bool hasAlpha;
};

/* These are moved here because in the generated/jni/<file>.h, these 
 * were defined only for "C" and not __cplusplus
 */
#ifdef __cplusplus 
#undef java_awt_image_ImageObserver_WIDTH
#define java_awt_image_ImageObserver_WIDTH 1L
#undef java_awt_image_ImageObserver_HEIGHT
#define java_awt_image_ImageObserver_HEIGHT 2L
#undef java_awt_image_ImageObserver_PROPERTIES
#define java_awt_image_ImageObserver_PROPERTIES 4L
#undef java_awt_image_ImageObserver_SOMEBITS
#define java_awt_image_ImageObserver_SOMEBITS 8L
#undef java_awt_image_ImageObserver_FRAMEBITS
#define java_awt_image_ImageObserver_FRAMEBITS 16L
#undef java_awt_image_ImageObserver_ALLBITS
#define java_awt_image_ImageObserver_ALLBITS 32L
#undef java_awt_image_ImageObserver_ERROR
#define java_awt_image_ImageObserver_ERROR 64L
#undef java_awt_image_ImageObserver_ABORT
#define java_awt_image_ImageObserver_ABORT 128L
#endif

/* Defines for ImageRepresentation.availInfo field to describe parts of the image that have been retrieved. */

#define IMAGE_SIZE_INFO (java_awt_image_ImageObserver_WIDTH |	\
					    java_awt_image_ImageObserver_HEIGHT)
					    
#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)


#endif	/* _IMAGE_REPRESENTATION_H_ */
