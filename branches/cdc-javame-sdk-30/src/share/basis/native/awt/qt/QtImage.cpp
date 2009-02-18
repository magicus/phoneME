/*
 * @(#)QtImage.cpp	1.24 06/10/10
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "java_awt_QtImage.h"
#include "java_awt_AlphaComposite.h"

#include "QtInterface.h"
#include "QtBackEnd.h"
#include "QtApplication.h"

/*
#ifdef QWS
#include "QPatchedPixmap.h"
#endif
*/

extern "C" {
   long getImageDataPtr(JNIEnv *env, jobject qtimage);
}

JNIEXPORT void JNICALL
Java_java_awt_QtImage_initIDs (JNIEnv * env, jclass cls)
{
	GET_FIELD_ID (QtImage_widthFID, "width", "I");
	GET_FIELD_ID (QtImage_heightFID, "height", "I");
	GET_FIELD_ID (QtImage_psdFID, "psd", "I");

	FIND_CLASS ("java/awt/image/ColorModel");
	GET_METHOD_ID (java_awt_image_ColorModel_getRGBMID, "getRGB", "(I)I");

	FIND_CLASS ("java/awt/image/IndexColorModel");
	GET_FIELD_ID (java_awt_image_IndexColorModel_rgbFID, "rgb", "[I");

	FIND_CLASS ("java/awt/image/DirectColorModel");
	GET_FIELD_ID (java_awt_image_DirectColorModel_red_maskFID, "red_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_red_offsetFID, "red_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_red_scaleFID, "red_scale", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_green_maskFID, "green_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_green_offsetFID, "green_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_green_scaleFID, "green_scale", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_blue_maskFID, "blue_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_blue_offsetFID, "blue_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_blue_scaleFID, "blue_scale", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_maskFID, "alpha_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_offsetFID, "alpha_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_scaleFID, "alpha_scale", "I");
}

JNIEXPORT void JNICALL
Java_java_awt_QtImage_imageCompleteLoadBuffer (JNIEnv * env, jclass cls, jint qtImageDesc, jboolean dispose)
{
	if(qtImageDesc<=0) return;
	
	assert(QtImageDescPool[qtImageDesc].loadBuffer!=NULL);

    AWT_QT_LOCK;
	((QPixmap *)QtImageDescPool[qtImageDesc].qpd)->convertFromImage(*(QtImageDescPool[qtImageDesc].loadBuffer),
                Qt::ThresholdDither | Qt::ThresholdAlphaDither | 
                Qt::AvoidDither);
										
	QtImageDescPool[qtImageDesc].mask = ((QPixmap *)QtImageDescPool[qtImageDesc].qpd)->mask();
	
	if(QtImageDescPool[qtImageDesc].mask == NULL)
	{	
		int width = QtImageDescPool[qtImageDesc].width;
		int height = QtImageDescPool[qtImageDesc].height;
		QBitmap bm(width, height);
		
		QPainter qp(&bm);
		qp.fillRect(0, 0, width, height, Qt::color1);
		
		((QPixmap *)QtImageDescPool[qtImageDesc].qpd)->setMask(bm);
		QtImageDescPool[qtImageDesc].mask = ((QBitmap *)QtImageDescPool[qtImageDesc].qpd)->mask();
	}
	
	if(dispose == JNI_TRUE) {
		delete QtImageDescPool[qtImageDesc].loadBuffer;
		QtImageDescPool[qtImageDesc].loadBuffer = NULL;
	}
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetColorModelBytePixels (JNIEnv * env,
jclass cls,
jint qtImageDesc,
jint x, jint y, jint w, jint h,
jobject colorModel,
jbyteArray pixels, jint offset, jint scansize)
{
	jint m, n;					 /* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned char pixel;
	unsigned char lastPixel;
	jint index = offset;
	jbyte *pixelArray = NULL;
	unsigned long rgb=0;
	
	pixelArray = env->GetByteArrayElements (pixels, NULL);

	if (pixelArray == NULL)
		return;


	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

    AWT_QT_LOCK;
	QImage *qi = QtImageDescPool[qtImageDesc].loadBuffer;
    assert(qi != NULL);
	
	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned char) pixelArray[index++];

			if (lastPixel != pixel)
			{
				rgb = (unsigned long)
					env->CallIntMethod (colorModel, QtCachedIDs.java_awt_image_ColorModel_getRGBMID,
					(jint) pixel);

				if (env->ExceptionCheck()) {
                    goto exit_method;
				}

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;				

			}

			qi->setPixel(m+x, n+y, rgb);
		}

		index += (scansize-m);
	}
 exit_method:
    AWT_QT_UNLOCK;
	env->ReleaseByteArrayElements (pixels, pixelArray, JNI_ABORT);
}


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetColorModelIntPixels (JNIEnv * env,
jclass cls,
jint qtImageDesc,
jint x, jint y, jint w, jint h,
jobject colorModel,
jintArray pixels, jint offset, jint scansize)
{
	unsigned long pixel;
	unsigned long lastPixel;
	jint index = offset;
	jint *pixelArray = NULL;
	int n, m;
	unsigned long rgb = 0;

	pixelArray = env->GetIntArrayElements (pixels, NULL);

	if (pixelArray == NULL)
		return;

	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

    AWT_QT_LOCK;
	QImage *qi = QtImageDescPool[qtImageDesc].loadBuffer;
    assert(qi != NULL);
	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned long) pixelArray[index++];

			if (lastPixel != pixel)
			{
				rgb = (unsigned long)
					env->CallIntMethod (colorModel, QtCachedIDs.java_awt_image_ColorModel_getRGBMID,
					pixel);

				if (env->ExceptionCheck()) {
                    goto exit_method;
				}

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;
			}

			qi->setPixel(m+x, n+y, rgb);
		}

		index += (scansize-m);
	}
 exit_method:
    AWT_QT_UNLOCK;
	env->ReleaseIntArrayElements (pixels, pixelArray, JNI_ABORT);
}


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetIndexColorModelBytePixels (JNIEnv * env,
                                                     jclass cls,
                                                     jint qtImageDesc,
                                                     jint x, jint y, 
                                                     jint w, jint h,
                                                     jobject colorModel,
                                                     jbyteArray pixels, 
                                                     jint offset, 
                                                     jint scansize)
{
	jintArray rgbs;
	jint *rgbArray;				 /* Array of RGB values retrieved from IndexColorModel. */
	jint m, n;					 /* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned char pixel;
	jint index = offset;
	jbyte *pixelArray = NULL;

	rgbs = (jintArray)env->GetObjectField (colorModel, QtCachedIDs.java_awt_image_IndexColorModel_rgbFID);
	rgbArray = env->GetIntArrayElements (rgbs, NULL);

	if (rgbArray == NULL)
		return;

	pixelArray = env->GetByteArrayElements (pixels, NULL);

	if (pixelArray == NULL) {
		env->ReleaseIntArrayElements (rgbs, rgbArray, JNI_ABORT);
		return;
	}

    AWT_QT_LOCK;
	QImage *qi = QtImageDescPool[qtImageDesc].loadBuffer;
    assert(qi != NULL);
	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned char) pixelArray[index++];

			/* Get and store red, green, blue values. */

			qi->setPixel(m+x, n+y, (unsigned long)rgbArray[pixel]);
		}

		index += (scansize-m);
	}
    AWT_QT_UNLOCK;
	
	if (pixelArray != NULL)
		env->ReleaseByteArrayElements (pixels, pixelArray, JNI_ABORT);

	env->ReleaseIntArrayElements (rgbs, rgbArray, JNI_ABORT);
}


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetIndexColorModelIntPixels (JNIEnv * env,
                                                    jclass cls,
                                                    jint qtImageDesc,
                                                    jint x, jint y, 
                                                    jint w, jint h,
                                                    jobject colorModel,
                                                    jintArray pixels, 
                                                    jint offset, 
                                                    jint scansize)
{
	jintArray rgbs;
	jint *rgbArray;				 /* Array of RGB values retrieved from IndexColorModel. */
	jint m, n;					 /* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned long pixel;
	jint *pixelArray = NULL;
	jint index = offset;

	rgbs = (jintArray)env->GetObjectField (colorModel, QtCachedIDs.java_awt_image_IndexColorModel_rgbFID);
	rgbArray = env->GetIntArrayElements (rgbs, NULL);

	if (rgbArray == NULL)
		return;

	pixelArray = env->GetIntArrayElements (pixels, NULL);

	if (pixelArray == NULL) {
		env->ReleaseIntArrayElements (rgbs, rgbArray, JNI_ABORT);
		return;
	}

    AWT_QT_LOCK;
	QImage *qi = QtImageDescPool[qtImageDesc].loadBuffer;
    assert(qi != NULL);
	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned long) pixelArray[index++];

			/* Get and store red, green, blue values. */

			qi->setPixel(m+x, n+y, (unsigned long)rgbArray[pixel]);
		}

		index += (scansize-m);
	}
    AWT_QT_UNLOCK;

	if (pixelArray != NULL)
		env->ReleaseIntArrayElements (pixels, pixelArray, JNI_ABORT);

	env->ReleaseIntArrayElements (rgbs, rgbArray, JNI_ABORT);
}


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetDirectColorModelPixels (JNIEnv * env,
												  jclass cls,
												  jint qtImageDesc,
												  jint x, jint y, 
                                                  jint w, jint h,
												  jobject colorModel,
												  jintArray pixels, 
                                                  jint offset, 
                                                  jint scansize, 
                                                  jint totalMask)
{
	
	jint *pixelArray = env->GetIntArrayElements (pixels, NULL);
	
	if (pixelArray == NULL) return;
	
    AWT_QT_LOCK;
	QImage *qi = QtImageDescPool[qtImageDesc].loadBuffer;
    assert(qi != NULL);
	
	/* For each pixel in the supplied pixel array calculate its rgb value 
     * from the colorModel. */
	
	if(((unsigned long)totalMask) == 0xFFFFFFFF) {
		int widthlinesize = w * sizeof(QRgb);
		QRgb *pa = ((QRgb *)pixelArray) + offset + scansize * (h - 1);
		for(int i=h-1;i>=0;i--) {
			memcpy((QRgb*)qi->scanLine(i+y)+x, pa, widthlinesize);
			pa -= scansize;
		}
    }
    else {
        /* x, y position in pixel array - same variable names as Javadoc 
         * comment for setIntPixels. */
        jint m, n;					 
		   
        jint index = offset;
        unsigned long rgb = 0;
        unsigned long pixel;
        unsigned long lastPixel;
		   
        unsigned long alpha, red, green, blue;
		   
        unsigned long red_mask = (unsigned long)env->GetIntField (colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_red_maskFID);
		   
        unsigned long green_mask = (unsigned long)env->GetIntField (colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_green_maskFID);
		   
        unsigned long blue_mask = (unsigned long) env->GetIntField (colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_blue_maskFID);
		   
        unsigned long alpha_mask = (unsigned long)env->GetIntField(colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_alpha_maskFID);
		   
        unsigned long red_offset = (unsigned long)env->GetIntField (colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_red_offsetFID);
        unsigned long red_scale = (unsigned long) env->GetIntField (colorModel,
				 QtCachedIDs.java_awt_image_DirectColorModel_red_scaleFID);
		   
        unsigned long green_offset = (unsigned long)env->GetIntField(
                 colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_green_offsetFID);
        unsigned long green_scale = (unsigned long) env->GetIntField (
                 colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_green_scaleFID);
		   
        unsigned long blue_offset = (unsigned long) env->GetIntField (
                 colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_blue_offsetFID);
        unsigned long blue_scale = (unsigned long) env->GetIntField (
                 colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_blue_scaleFID);
		   
        unsigned long alpha_offset = (unsigned long) env->GetIntField (
                 colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_alpha_offsetFID);
        unsigned long alpha_scale = (unsigned long) env->GetIntField (
                 colorModel,
                 QtCachedIDs.java_awt_image_DirectColorModel_alpha_scaleFID);
		   
        /* Make sure first time round the loop we actually get the 
         * pixel value. */
		   
        lastPixel = ~(pixelArray[index]);
		   
        for (n = 0; n < h; n++) {
            for (m = 0; m < w; m++) {
                /* Get the pixel at m, n. */
				   
                pixel = (unsigned long) pixelArray[index++];
				   
                /* If pixel value is not the same as the last one then get 
                 * red, green, blue and set the foreground for drawing the 
                 * point in the image. */
				   
                if (pixel != lastPixel) {
                    red = ((pixel & red_mask) >> red_offset);
					
                    if (red_scale != 0)
                        red = red * 255 / red_scale;
					   
                    green = ((pixel & green_mask) >> green_offset);
					   
                    if (green_scale != 0)
                        green = green * 255 / green_scale;
					   
                    blue = ((pixel & blue_mask) >> blue_offset);
					   
                    if (blue_scale != 0)
                        blue = blue * 255 / blue_scale;
					   
                    if (alpha_mask == 0)
                        alpha = 255;
                    else {
                        alpha = ((pixel & alpha_mask) >> alpha_offset);
						   
                        if (alpha_scale != 0)
                            alpha = alpha * 255 / alpha_scale;
                    }
					   
                    rgb = alpha << 24 | red << 16 | green << 8 | blue;
					   
                    /* Remember last pixel value for optimisation purposes. */
					   
                    lastPixel = pixel;
					
                }
				   
                qi->setPixel(m+x, n+y, rgb);
            }
			   
            index += (scansize-m);
        }
    }
    AWT_QT_UNLOCK;

	if (pixelArray != NULL)
		env->ReleaseIntArrayElements (pixels, pixelArray, JNI_ABORT);
}


extern void debug_dump_image(int i);

JNIEXPORT jint JNICALL
Java_java_awt_QtImage_pGetRGB(JNIEnv *env, jclass cls, jint qtImageDesc, jint x, jint y)
{
    jint pixel = 0 ;
    AWT_QT_LOCK; {
/*
#ifdef QWS
	QImage qi = ((QPatchedPixmap *)QtImageDescPool[qtImageDesc].qpd)->convertToImage();	
#else
*/
	QImage qi = ((QPixmap *)QtImageDescPool[qtImageDesc].qpd)->convertToImage();
//#endif
	
	pixel = (jint)qi.pixel(x, y);
    }
    AWT_QT_UNLOCK;
    return pixel;
}

JNIEXPORT void JNICALL
Java_java_awt_QtImage_pGetRGBArray(JNIEnv *env, jclass cls, jint qtImageDesc,
jint startX, jint startY, jint width, jint height, jintArray pixelArray,
int offset, int scansize)
{
	jint *pixelArrayElements, *pa;
	
    if ( qtImageDesc < 0 )
        return ;

	pa = pixelArrayElements = env->GetIntArrayElements( pixelArray, NULL);

	if (pixelArrayElements == NULL)
		return;
	
	pa += offset;

    AWT_QT_LOCK; {
/*
#   ifdef QWS
	QImage qi = ((QPatchedPixmap *)QtImageDescPool[qtImageDesc].qpd)->convertToImage();	
#   else
*/
	QImage qi = ((QPixmap *)QtImageDescPool[qtImageDesc].qpd)->convertToImage();
//#   endif /* QWS */
	
    QImage *qip = &qi ;
    QImage qi32bpp ;
    if ( qi.depth() <= 8 ) {
        qi32bpp = qi.convertDepth(32);
        qip = &qi32bpp;
    }

    assert(qip->depth() == 32);
	if(!qip->isNull()) {
		int widthlinesize = width * sizeof(QRgb);
		pa += scansize * (height - 1);
		for(int n=height-1;n>=0;n--) {
			memcpy(pa, (QRgb*)qip->scanLine(n+startY)+startX, widthlinesize);
			pa -= scansize;
		}
	}
    }
    AWT_QT_UNLOCK;

	env->ReleaseIntArrayElements(pixelArray, pixelArrayElements, 0);
}


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetRGB(JNIEnv *env, jclass cls, jint qtImageDesc, jint x, jint y, jint rgb)
{
	QtImageDesc *qid = &QtImageDescPool[qtImageDesc];
    AWT_QT_LOCK; {
	QPainter qp(qid->qpd);
		
	qp.setPen(QColor(rgb));
	qp.drawPoint(x, y);
    }
    AWT_QT_UNLOCK;
}


// should be called with lock held.
static void
setRGBArray(QPaintDevice *qpd, int startX, int startY,
            int width, int height, int *rgbArray) {
    QPainter qp(qpd);

    int c = 0 ;
    width  += startX ; // endX
    height += startY ; // endY
    for (int y = startY ; 
         y < height ;  // y < endY
         y ++ ) {
        for ( int x = startX ; 
              x < width ;  // x < endX
              x ++ ) {
            //printf("x=%d y=%d color=%x\n", x, y, rgbArray[c]);
            qp.setPen(QColor(rgbArray[c++]));
            qp.drawPoint(x, y);
        }
    }
}

JNIEXPORT void JNICALL
Java_java_awt_QtImage_pSetRGBArray(JNIEnv *env, jclass cls, 
                                   jint qtImageDesc, jint startX, 
                                   jint startY, jint width, jint height, 
                                   jintArray rgbArray, jint offset, 
                                   jint scansize)
{
	jint *pixelArrayElements, *pa;

	if(startX+width>QtImageDescPool[qtImageDesc].width)
		width -= (startX+width) - QtImageDescPool[qtImageDesc].width;
	if(startY+height>QtImageDescPool[qtImageDesc].height)
		height -= (startY+height) - QtImageDescPool[qtImageDesc].height;
	
	pa = pixelArrayElements = env->GetIntArrayElements( rgbArray, NULL);
	
	if (pixelArrayElements == NULL) {
		return;
    }
	
	pa += offset;
	
    AWT_QT_LOCK; {
    QPixmap *qpd = ((QPixmap *)QtImageDescPool[qtImageDesc].qpd) ;

    if ( qpd->depth() <= 8 ) {
        setRGBArray(qpd, 
                    (int)startX, (int)startY,
                    (int)width, (int)height,
                    pa);
    }
    else {
        QImage qi;
/*
#ifdef QWS
        qi = ((QPatchedPixmap *)qpd)->convertToImage();	
#else
*/
        qi = qpd->convertToImage();
//#endif

        if(!qi.isNull()) {	
            pa += scansize * (height - 1);
            int widthlinesize = width * sizeof(QRgb);
            for(int n=height-1;n>=0;n--) {	
                memcpy((QRgb*)qi.scanLine(n+startY)+startX, 
                       pa, 
                       widthlinesize);
                pa -= scansize;
            }
            
            qpd->convertFromImage(qi, 0);
		
#ifdef QT_AWT_1BIT_ALPHA
            QtImageDescPool[qtImageDesc].mask = ((QPixmap *)QtImageDescPool[qtImageDesc].qpd)->mask();

            if(QtImageDescPool[qtImageDesc].mask == NULL) {
                QBitmap bm(width, height);
                
                QPainter qp(&bm);
                qp.fillRect(0, 0, width, height, Qt::color1);
                    
                qpd->setMask(bm);
                QtImageDescPool[qtImageDesc].mask = 
                    ((QBitmap *)QtImageDescPool[qtImageDesc].qpd)->mask();
            }
#endif /* QT_AWT_1BIT_ALPHA */
            
        }
    }
    }
    AWT_QT_UNLOCK;
			
	env->ReleaseIntArrayElements(rgbArray, pixelArrayElements, JNI_ABORT);
	
}

/* 
#define ALPHA_VALUE(c)  (c>>24)
#define RED_VALUE(c)    ((c<<8)>>24)
#define GREEN_VALUE(c)  ((c<<16)>>24)
#define BLUE_VALUE(c)   ((c<<24)>>24)
 
#define BLEND_MULT(p, q) (((p)*(q) + 0x80 ) >> 8)
*/

// Note : Should be called with the lock held.
#ifdef QT_AWT_1BIT_ALPHA
static inline bool maskBitmap(int qtGraphDesc, QPainter &p)
{
	bool r;
		
	if(p.isActive()) p.end();
	
	p.begin((QPaintDevice *)(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].mask));	
	
	switch (QtGraphDescPool[qtGraphDesc].blendmode)
	{
		case java_awt_AlphaComposite_SRC_OVER:
			p.setRasterOp(Qt::OrROP);
			break;
		case java_awt_AlphaComposite_SRC:
			p.setRasterOp(Qt::CopyROP);
			break;
		case java_awt_AlphaComposite_CLEAR:
			p.setRasterOp(Qt::ClearROP);
			break;
		default:
			p.setRasterOp(Qt::NopROP);
			break;
	}
			
	if(QtGraphDescPool[qtGraphDesc].clipped)
	{	
		int *clip = QtGraphDescPool[qtGraphDesc].clip;
		p.setClipRect(clip[0], clip[1], clip[2], clip[3]);
    }

	r = true;
	
	return r;	
}
#else
#define maskBitmap(desc, painter) (FALSE)
#endif /* QT_AWT_1BIT_ALPHA */


JNIEXPORT void JNICALL
Java_java_awt_QtImage_pDrawImage (JNIEnv * env, jclass cls, jint qtGraphDescDest, jint qtImageDescSrc, jint x, jint y, jobject bg)
{
    AWT_QT_LOCK; {
    //6206231
    // If the raster operation is NopROP, then the alpha value is less then 1.
    // If the alpha value is less then 1, don't draw the source...dest only
    // should display.
    if (QtGraphDescPool[qtGraphDescDest].rasterOp == Qt::NopROP) {
        AWT_QT_UNLOCK;
        return;
    }
    //6206231

	QtImageDesc *qidd = &QtImageDescPool[QtGraphDescPool[qtGraphDescDest].qid];
	QtImageDesc *qids = &QtImageDescPool[qtImageDescSrc];

	QPainter p(qidd->qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDescDest].rasterOp);
	if(QtGraphDescPool[qtGraphDescDest].clipped)
	{	
		int *clip = QtGraphDescPool[qtGraphDescDest].clip;
		p.setClipRect(clip[0], clip[1], clip[2], clip[3]);
	}

	if (bg != NULL)
	{
		jint rgb = env->GetIntField(bg, QtCachedIDs.Color_valueFID);

		p.setBackgroundColor(QColor(rgb));
		p.eraseRect(x, y, qids->width, qids->height);
	}	
		
    //6206231
    QPixmap pm;
    pm = *((QPixmap *)(qids->qpd));
    /* If the raster operation is CLEAR, then alpha value should
       be 0 and the pixel color should be 0.
    */
    if (QtGraphDescPool[qtGraphDescDest].rasterOp == Qt::ClearROP) {
        QtGraphDescPool[qtGraphDescDest].currentalpha = 0;
        pm.fill(Qt::black);
    }
    //6206231

	p.drawPixmap(x, y, pm);
	
	if(maskBitmap(qtGraphDescDest, p))
		p.drawPixmap(x, y, *((QBitmap *)(qids->mask)));	
    }
    AWT_QT_UNLOCK;
	
//	p.drawPixmap(x, y, *((QBitmap *)(qids->mask)));
}


#define SWAP(A, B) {A^=B; B^=A; A^=B;}

JNIEXPORT void JNICALL
Java_java_awt_QtImage_pDrawImageScaled(JNIEnv * env, jclass cls, jint qtGraphDest, jint dx1, jint dy1, jint dx2, jint dy2, jint qtImageSrc, jint sx1, jint sy1, jint sx2, jint sy2, jobject bg)
{
/* Sanity has to prevail and big images have to be capped */
#define ARBITRARY_IMPOSED_DIMENSION 2000
	
    AWT_QT_LOCK; {
    //6206231
    // If the raster operation is NopROP, then the alpha value is less then 1.
    // If the alpha value is less then 1, don't draw the source...dest only
    // should display.
    if (QtGraphDescPool[qtGraphDest].rasterOp == Qt::NopROP) {
        AWT_QT_UNLOCK;
        return;
    }
    //6206231

	if(dx1<-ARBITRARY_IMPOSED_DIMENSION) dx1 = -ARBITRARY_IMPOSED_DIMENSION;
	if(dx1>ARBITRARY_IMPOSED_DIMENSION) dx1 = ARBITRARY_IMPOSED_DIMENSION;
	if(dx2<-ARBITRARY_IMPOSED_DIMENSION) dx2 = -ARBITRARY_IMPOSED_DIMENSION;
	if(dx2>ARBITRARY_IMPOSED_DIMENSION) dx2 = ARBITRARY_IMPOSED_DIMENSION;
	if(sy1<-ARBITRARY_IMPOSED_DIMENSION) sy1 = -ARBITRARY_IMPOSED_DIMENSION;
	if(sy1>ARBITRARY_IMPOSED_DIMENSION) sy1 = ARBITRARY_IMPOSED_DIMENSION;
	if(sy2<-ARBITRARY_IMPOSED_DIMENSION) sy2 = -ARBITRARY_IMPOSED_DIMENSION;
	if(sy2>ARBITRARY_IMPOSED_DIMENSION) sy2 = ARBITRARY_IMPOSED_DIMENSION;	
	
	int srcW = sx2 - sx1;
	int srcH = sy2 - sy1;
	int dstW = dx2 - dx1;
	int dstH = dy2 - dy1;
	
	if((srcW==0)||(srcH==0)||(dstW==0)||(dstH==0)) {
        AWT_QT_UNLOCK;
        return;
    }
	
	if(srcW<0) SWAP(sx1, sx2);
	if(srcH<0) SWAP(sy1, sy2);

	bool hflip = (dstH*srcH)<0;
	bool vflip = (dstW*srcW)<0;

	bool needImage = hflip || vflip || (srcW!=dstW) || (srcH!=dstH);
		
	// Look in QtGraphics.java there is some magic to avoid thread problems...
	QImage qidd;

	if(needImage) {	
/*
#ifdef QWS
	    QImage qis = ((QPatchedPixmap *)QtImageDescPool[qtImageSrc].qpd)->convertToImage();	
#else
*/
	    QImage qis = ((QPixmap *)QtImageDescPool[qtImageSrc].qpd)->convertToImage();
//#endif
		
	    QImage qiss = qis.copy(sx1, sy1, sx2-sx1, sy2-sy1);
	
	    if(qiss.isNull()) {
            AWT_QT_UNLOCK;
            return;
        }
	
	    QImage qid = qiss.smoothScale(dstW>0?dstW:-dstW, dstH>0?dstH:-dstH);
	    qidd = qid.mirror(hflip, vflip);

	    if(dstW<0) SWAP(dx1, dx2);
	    if(dstH<0) SWAP(dy1, dy2);
	}
	
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDest].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDest].rasterOp);
	if(QtGraphDescPool[qtGraphDest].clipped)
	{	
		int *clip = QtGraphDescPool[qtGraphDest].clip;
		p.setClipRect(clip[0], clip[1], clip[2], clip[3]);
	}
	
	if (bg != NULL)
	{
		jint rgb = env->GetIntField(bg, QtCachedIDs.Color_valueFID);
		
		p.setBackgroundColor(QColor(rgb));
		p.eraseRect(dx1, dy1, dx2-dx1, dy2-dy1);
	}		

	if(needImage)
	{
		QPixmap pm;
		pm.convertFromImage(qidd, Qt::ThresholdDither | Qt::ThresholdAlphaDither | Qt::AvoidDither);

        //6206231
        /* If the raster operation is CLEAR, then alpha value should
           be 0 and the pixel color should be 0.
        */
        if (QtGraphDescPool[qtGraphDest].rasterOp == Qt::ClearROP) {
            QtGraphDescPool[qtGraphDest].currentalpha = 0;
            pm.fill(Qt::black);
        }
        //6206231

		p.drawPixmap(dx1, dy1, pm);
		if((pm.mask() != NULL) && maskBitmap(qtGraphDest, p))
			p.drawPixmap(dx1, dy1, *(QPixmap *)pm.mask());
		
//		p.drawImage(dx1, dy1, qidd);
	}
	else
	{
        //6206231
        QPixmap pm = *((QPixmap *)QtImageDescPool[qtImageSrc].qpd);

        /* If the raster operation is CLEAR, then alpha value should
           be 0 and the pixel color should be 0.
        */
        if (QtGraphDescPool[qtGraphDest].rasterOp == Qt::ClearROP) {
            QtGraphDescPool[qtGraphDest].currentalpha = 0;
            pm.fill(Qt::black);
        }
        //6206231

		p.drawPixmap(dx1, dy1, pm, sx1, sy1, sx2-sx1, sy2-sy1);
		if(maskBitmap(qtGraphDest, p))
			p.drawPixmap(dx1, dy1, *((QBitmap *)(QtImageDescPool[qtImageSrc].mask)), sx1, sy1, sx2-sx1, sy2-sy1);

        /*
          p.drawPixmap(dx1, dy1, *((QPixmap *)QtImageDescPool[qtImageSrc].qpd), sx1, sy1, sx2-sx1, sy2-sy1);
          if(maskBitmap(qtGraphDest, p))
          p.drawPixmap(dx1, dy1, *((QBitmap *)(QtImageDescPool[qtImageSrc].mask)), sx1, sy1, sx2-sx1, sy2-sy1);
        */
	}
    }
    AWT_QT_UNLOCK;
}

long 
getImageDataPtr(JNIEnv *env, jobject qtimage) {
    long datap = 0 ;
    // get the image descriptor associated the qtimage
    int qtImageDesc = (int)
        env->GetIntField(qtimage, QtCachedIDs.QtImage_psdFID) ;
    if ( qtImageDesc >= 0 ) {
        QImage qi = *(QtImageDescPool[qtImageDesc].loadBuffer);
        datap = (long)qi.scanLine(0) ;
    }
    return datap;

}
