/*
 * @(#)QtImageRepresentation.cc	1.22 06/10/25
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
#include <qt.h>
#include <qwidget.h>
#include "jni.h"
#include "awt.h"
#include "sun_awt_qt_QtImageRepresentation.h"
#include "java_awt_image_BufferedImage.h"
#include "java_awt_image_ImageObserver.h"
#include "ImageRepresentation.h"
#include "QtGraphics.h"
#include "QtApplication.h"
#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

#define IMAGE_DEPTH 32
#define ALPHA_OPAQUE 255

#define SCALE_FACTOR_5_TO_8 (255.0/31.0)
#define SCALE_FACTOR_6_TO_8 (255.0/63.0)

#define RSHIFT_8_TO_6 (8-6)
#define RSHIFT_8_TO_5 (8-5)

/**
 * Given RGB in 8bit, the following code will convert RGB to 565 format.
 * Since the floating point operation is very expensive in terms of
 * cycles, we have created two lookup tables with the values. This 
 * speeds up the performance of setting pixels. For example :- to set
 * scan line of 249 pixels it took 6ms where as with the lookup table it
 * took 360us
 *
 * // These will give the values according to the number of bits
 * // allocated for each color. 
 *   red = (int) (red / RMULT);
 *   green = (int) (green / GMULT);
 *   blue = (int) (blue / BMULT);
 *   
 *  // Now, we will get the actual RGB that Qt/Embedded will see in
 *  // its 16-bit format. 
 *   red = red << QT_RSHIFT;
 *   green = green << QT_GSHIFT;
 *   blue = blue << QT_BSHIFT;
 *
 * @see get565DirectColorModelRGB()
 * @see getQtPixel()
 * @see ImageRepresentation :: setPixel()
 */
unsigned char SCALE_TABLE_8_TO_5[] = {
  0,   0,   0,   0,   0,   0,   0,   0,   0,   8,   8,   8,   8,   8,   8,   8,
  8,  16,  16,  16,  16,  16,  16,  16,  16,  24,  24,  24,  24,  24,  24,  24,
 24,  32,  32,  32,  32,  32,  32,  32,  32,  32,  40,  40,  40,  40,  40,  40,
 40,  40,  48,  48,  48,  48,  48,  48,  48,  48,  56,  56,  56,  56,  56,  56,
 56,  56,  64,  64,  64,  64,  64,  64,  64,  64,  64,  72,  72,  72,  72,  72,
 72,  72,  72,  80,  80,  80,  80,  80,  80,  80,  80,  88,  88,  88,  88,  88,
 88,  88,  88,  96,  96,  96,  96,  96,  96,  96,  96, 104, 104, 104, 104, 104,
104, 104, 104, 104, 112, 112, 112, 112, 112, 112, 112, 112, 120, 120, 120, 120,
120, 120, 120, 120, 128, 128, 128, 128, 128, 128, 128, 128, 136, 136, 136, 136,
136, 136, 136, 136, 136, 144, 144, 144, 144, 144, 144, 144, 144, 152, 152, 152,
152, 152, 152, 152, 152, 160, 160, 160, 160, 160, 160, 160, 160, 168, 168, 168,
168, 168, 168, 168, 168, 176, 176, 176, 176, 176, 176, 176, 176, 176, 184, 184,
184, 184, 184, 184, 184, 184, 192, 192, 192, 192, 192, 192, 192, 192, 200, 200,
200, 200, 200, 200, 200, 200, 208, 208, 208, 208, 208, 208, 208, 208, 208, 216,
216, 216, 216, 216, 216, 216, 216, 224, 224, 224, 224, 224, 224, 224, 224, 232,
232, 232, 232, 232, 232, 232, 232, 240, 240, 240, 240, 240, 240, 240, 240, 240
} ;

unsigned char SCALE_TABLE_8_TO_6[] = {
  0,   0,   0,   0,   0,   4,   4,   4,   4,   8,   8,   8,   8,  12,  12,  12,
 12,  16,  16,  16,  16,  20,  20,  20,  20,  24,  24,  24,  24,  28,  28,  28,
 28,  32,  32,  32,  32,  36,  36,  36,  36,  40,  40,  40,  40,  44,  44,  44,
 44,  48,  48,  48,  48,  52,  52,  52,  52,  56,  56,  56,  56,  60,  60,  60,
 60,  64,  64,  64,  64,  68,  68,  68,  68,  72,  72,  72,  72,  76,  76,  76,
 76,  80,  80,  80,  80,  84,  84,  84,  84,  84,  88,  88,  88,  88,  92,  92,
 92,  92,  96,  96,  96,  96, 100, 100, 100, 100, 104, 104, 104, 104, 108, 108,
108, 108, 112, 112, 112, 112, 116, 116, 116, 116, 120, 120, 120, 120, 124, 124,
124, 124, 128, 128, 128, 128, 132, 132, 132, 132, 136, 136, 136, 136, 140, 140,
140, 140, 144, 144, 144, 144, 148, 148, 148, 148, 152, 152, 152, 152, 156, 156,
156, 156, 160, 160, 160, 160, 164, 164, 164, 164, 168, 168, 168, 168, 168, 172,
172, 172, 172, 176, 176, 176, 176, 180, 180, 180, 180, 184, 184, 184, 184, 188,
188, 188, 188, 192, 192, 192, 192, 196, 196, 196, 196, 200, 200, 200, 200, 204,
204, 204, 204, 208, 208, 208, 208, 212, 212, 212, 212, 216, 216, 216, 216, 220,
220, 220, 220, 224, 224, 224, 224, 228, 228, 228, 228, 232, 232, 232, 232, 236,
236, 236, 236, 240, 240, 240, 240, 244, 244, 244, 244, 248, 248, 248, 248, 252
} ;

/* Qt/Embedded encodes the RGB in 565 format, or rrrrrggggggbbbbb.
   Here is the decoding procedure:
   red = rrrrr000
   green = gggggg00
   blue = bbbbb000
   Here, we tried to reconstruct the pixel from the QRgb, and
   then use it as the argument to ColorModel.getRGB().
*/	   
inline jint get565DirectColorModelRGB(const int red, 
				      const int green,
				      const int blue) 
{
    int tr = red >> RSHIFT_8_TO_5;
    int tg = green >> RSHIFT_8_TO_6;
    int tb = blue >> RSHIFT_8_TO_5;

    tr = (int) (tr * SCALE_FACTOR_5_TO_8);
    tg = (int) (tg * SCALE_FACTOR_6_TO_8);
    tb = (int) (tb * SCALE_FACTOR_5_TO_8);
    
    return (jint) qRgb(tr, tg, tb);
}

#ifndef QWS
inline jint get555DirectColorModelRGB(const int red, 
				      const int green,
				      const int blue) 
{
    int tr = red >> RSHIFT_8_TO_5;
    int tg = green >> RSHIFT_8_TO_5;
    int tb = blue >> RSHIFT_8_TO_5;

    tr = (int) (tr * SCALE_FACTOR_5_TO_8);
    tg = (int) (tg * SCALE_FACTOR_5_TO_8);
    tb = (int) (tb * SCALE_FACTOR_5_TO_8);
    
    return (jint) qRgb(tr, tg, tb);
}

inline jint getDirectColorModelRGB(const int red,
                                   const int green,
                                   const int blue)
{
    jint imageType = ImageRepresentation::defaultImageType();
    if (imageType == java_awt_image_BufferedImage_TYPE_USHORT_565_RGB) {
	return get565DirectColorModelRGB(red, green, blue);
    } else if (imageType == java_awt_image_BufferedImage_TYPE_USHORT_555_RGB) {
        return get555DirectColorModelRGB(red, green, blue);
    } else {
        return (jint) qRgb(red, green, blue);
    }
}
#endif /* ifndef QWS */

inline QRgb getQtPixel(int red, int green, int blue, int alpha) 
{
#ifdef QWS
    red = (int)(SCALE_TABLE_8_TO_5[red]);
    green = (int)(SCALE_TABLE_8_TO_6[green]);
    blue = (int)(SCALE_TABLE_8_TO_5[blue]);
#else
    jint imageType = ImageRepresentation::defaultImageType();
    if (imageType == java_awt_image_BufferedImage_TYPE_USHORT_565_RGB) {
	red = (int)(SCALE_TABLE_8_TO_5[red]);
	green = (int)(SCALE_TABLE_8_TO_6[green]);
	blue = (int)(SCALE_TABLE_8_TO_5[blue]);
    } else if (imageType == java_awt_image_BufferedImage_TYPE_USHORT_555_RGB) {
        red = (int)(SCALE_TABLE_8_TO_5[red]);
	green = (int)(SCALE_TABLE_8_TO_5[green]);
	blue = (int)(SCALE_TABLE_8_TO_5[blue]);
    }
#endif
    return qRgba(red, green, blue, alpha);
}

/*
 * Constructor of the class
 */
ImageRepresentation:: ImageRepresentation(int thisWidth,
					  int thisHeight,
					  QPixmap *thisPixmap,
					  QImage *thisImage) 
{
    this->width = thisWidth;
    this->height = thisHeight;
    this->pixmap = thisPixmap;
    this->dirtyPixmap = FALSE;
    this->image = thisImage;
    this->dirtyImage = FALSE;
    this->numLines = 0;
    this->backgroundColor = QColor(qRgba(0, 0, 0, 0));
    this->hasAlpha = FALSE;
}

ImageRepresentation::~ImageRepresentation() 
{
    if (this->pixmap != NULL) {
	delete(this->pixmap);
	this->pixmap = NULL;
    }
    if (this->image != NULL) {
	delete(this->image);
	this->image = NULL;
    }
}

QPixmap *
ImageRepresentation :: getPixmap(void)
{
    return this->pixmap;
}


QImage *
ImageRepresentation :: getImage(void)
{
    return this->image;
}

void 
ImageRepresentation :: setImage(QImage *newImage)
{
    if ( this->image != NULL )
        delete(this->image) ;
    this->image = newImage;
}

void 
ImageRepresentation :: setPixmap(QPixmap *newPixmap)
{
    this->pixmap = newPixmap;
}

QColor
ImageRepresentation :: getBackground(void) 
{
    return this->backgroundColor;
}

void
ImageRepresentation :: setBackground(const QColor &color) 
{
    this->backgroundColor = color;
}

int 
ImageRepresentation :: getWidth()
{
    return this->width;
}

void 
ImageRepresentation :: setWidth(int newWidth)
{
    this->width = newWidth;
}

int 
ImageRepresentation :: getHeight()
{
    return this->height;
}

void 
ImageRepresentation :: setHeight(int newHeight)
{
    this->height = newHeight;
}

int 
ImageRepresentation :: getNumLines(void) 
{
  return this->numLines;
}

void 
ImageRepresentation :: setNumLines(int numlines)
{
  this->numLines = numlines;
}

bool 
ImageRepresentation :: isDirtyPixmap() 
{
    return this->dirtyPixmap;
}

void 
ImageRepresentation :: setDirtyPixmap(bool isDirty) 
{
    this->dirtyPixmap = isDirty;
}

bool 
ImageRepresentation :: isDirtyImage() 
{
    return this->dirtyImage;
}

void 
ImageRepresentation :: setDirtyImage(bool isDirty) 
{
    this->dirtyImage = isDirty;
}

void 
ImageRepresentation :: updatePixmap() 
{
    if (this->pixmap != NULL) {
	if (this->image != NULL) {
            AWT_QT_LOCK;   //6174603
            this->pixmap->convertFromImage(*this->image, 
                                           Qt::ThresholdDither |
                                           Qt::ThresholdAlphaDither |
                                           Qt::AvoidDither);

            // 6322215
            //QtGraphics::updateCachedPainter(this->pixmap); // 6318187
            // 6322215

            AWT_QT_UNLOCK;   //6174603
	}
	this->dirtyImage = FALSE;
    }
}

void 
ImageRepresentation :: updateImage() 
{
    if (this->pixmap != NULL) {
	if (this->image) {
	    delete this->image;
	}
    AWT_QT_LOCK;   //6174603

// Following is the comment from Qt 3.3.3 for QPixmap::convertToImage().
/*!
    Converts the pixmap to a QImage. Returns a null image if it fails.

    If the pixmap has 1-bit depth, the returned image will also be 1
    bit deep. If the pixmap has 2- to 8-bit depth, the returned image
    has 8-bit depth. If the pixmap has greater than 8-bit depth, the
    returned image has 32-bit depth.

    Note that for the moment, alpha masks on monochrome images are
    ignored.

    \sa convertFromImage()
*/

    QImage img = this->pixmap->convertToImage();
    if (img.depth() != IMAGE_DEPTH) {
        this->image = new QImage(img.convertDepth(IMAGE_DEPTH));
    } else {
        this->image = new QImage(img);
    }
    AWT_QT_UNLOCK;   //6174603
    }
    this->dirtyPixmap = FALSE;
}

// Doesn't need AWT_QT_UN/LOCK as all of the places which call this are
// already locked.
void
ImageRepresentation :: setPixel(int x, int y, int red, int green, int blue, int alpha) 
{
    // Fixed 6186192
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }

    AWT_QT_LOCK;  //6212594
    this->getImage()->setPixel(x, y, getQtPixel(red, green, blue, alpha));
    AWT_QT_UNLOCK;  //6212594
    
    if (!this->hasAlpha && alpha < ALPHA_OPAQUE) {
	this->hasAlpha = TRUE;
    }
}

void
ImageRepresentation :: finish() 
{
    if (!this->hasAlpha) {
    AWT_QT_LOCK;  //6212594
	this->image->setAlphaBuffer(FALSE);
    AWT_QT_UNLOCK;  //6212594
    }
    
    this->updatePixmap();
}

ImageRepresentation *
ImageRepresentation::getImageRepresentationFromJNI(JNIEnv *env,
                                                   jobject thisObj,
                                                   jobject background) {
    return ImageRepresentation::getImageRepresentationFromJNI(env,
                                                              thisObj,
                                                              background,
                                                              NULL) ;
}

/* Gets the data associated with this ImageRepresentaion and ensures
   the pixmap is created. If the pixmap has not yet been created the
   size of the image is queried and the pixmap is created using this
   size. If the size of the image is not yet known then an exception
   is thrown. When the pixmap is created it will be filled with the
   background color supplied or, if background is NULL, then it will
   be filled with black. Returns the data with a valid pixmap or NULL
   if, and only if, an exception has been thrown. 
 */
ImageRepresentation *
ImageRepresentation::getImageRepresentationFromJNI(JNIEnv *env,
                                                   jobject thisObj,
                                                   jobject background,
                                                   QImage *image) {
    if (env->MonitorEnter(thisObj) < 0) {
	return NULL;
    }
    
    ImageRepresentation* imgRep = (ImageRepresentation*)
        env->GetIntField(thisObj, QtCachedIDs.
                         sun_awt_image_ImageRepresentation_pDataFID);

    /* If the imgRep hasn't yet been created then create a new one and
       set it on the object. */
    if (imgRep == NULL) {
	int availInfo, width, height;
	QPixmap *pixmap;
	QColor bgColor; 

	availInfo = env->GetIntField
	    (thisObj, 
	     QtCachedIDs.
	     sun_awt_image_ImageRepresentation_availinfoFID);
      
	if ((availInfo & IMAGE_SIZE_INFO) != IMAGE_SIZE_INFO) {     
	    env->ThrowNew(QtCachedIDs.java_awt_AWTErrorClass, 
			  "Cannot create pixmap! Image size not known");
	    env->MonitorExit(thisObj);
	    return NULL;
	}

	/* Get the width and height  */
	width = env->GetIntField
	    (thisObj, 
	     QtCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
	height = env->GetIntField
	    (thisObj, 
	     QtCachedIDs.sun_awt_image_ImageRepresentation_heightFID);

	if (width > 0 && height > 0) { 
	    AWT_QT_LOCK;

	    /* Create pixmap and image with required width, height & depth. */
	    pixmap = new QPixmap(width, 
				 height);
	    if (pixmap == NULL) {               
		env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
		AWT_QT_UNLOCK;
		env->MonitorExit(thisObj);
		return NULL;
	    }                  
	    
        /* if the image is not-null, we are typically called with a
         * QImage with all the pixels in it (see setImageNative())
         */
        if ( image == NULL ) {
            QPainter painter(pixmap);

            /* Fill the pixmap with the passed background color */
            if (background != NULL) {
                QColor *fillColor = QtGraphics::getColor(env,
                                                         background);
                bgColor = *fillColor;
                delete fillColor;
            } else {
                QRgb r = qRgba(0,0,0,0);
                bgColor = QColor(r);
            }  
            painter.fillRect(0, 0, width, height, bgColor);

            image = new QImage(width, 
                               height,
                               IMAGE_DEPTH);

            if (image == NULL) {               
                env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
                AWT_QT_UNLOCK;
                env->MonitorExit(thisObj);
                return NULL;
            }                  

            image->setAlphaBuffer(TRUE);
        }
      
	    imgRep = new ImageRepresentation(width, height, pixmap, image);
	    if (imgRep == NULL) {
		env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass,
			       NULL);
		AWT_QT_UNLOCK;      
		env->MonitorExit(thisObj);
		return NULL;
	    }

	    imgRep->setBackground(bgColor);

	    // Attach data to this object
	    env->SetIntField (thisObj, QtCachedIDs.
			      sun_awt_image_ImageRepresentation_pDataFID, 
			      (jint)imgRep);    
	    AWT_QT_UNLOCK;
	}
	/* Invalid width and height for image. */ 
	else {
	    env->ThrowNew (QtCachedIDs.java_awt_AWTErrorClass, 
			   "Cannot create pixmap as width and height must be > 0");
	    env->MonitorExit(thisObj);
	    return NULL;
	}
    }

    if (env->MonitorExit(thisObj) < 0) {
	env->SetIntField (thisObj, QtCachedIDs.
			  sun_awt_image_ImageRepresentation_pDataFID, 
			  (jint)0);
    	delete imgRep;
	return NULL;
    }
    return imgRep;
}

/* 
 * Unsynchronized version of getImageRepresentationFromJNI()
 * without availInfo check.
 *
 * Called from getDataForImageDrawing(). 
 */
ImageRepresentation *
ImageRepresentation::createImageRepresentationFromJNI(
    JNIEnv *env, jobject thisObj, jobject background)
{
    int width, height;
    QPixmap *pixmap;
    QImage *image;
    QColor bgColor;
    ImageRepresentation* imgRep;

    /* Get the width and height  */
    width = env->GetIntField
        (thisObj,
         QtCachedIDs.sun_awt_image_ImageRepresentation_widthFID);

    height = env->GetIntField
        (thisObj,
         QtCachedIDs.sun_awt_image_ImageRepresentation_heightFID);

    if (width > 0 && height > 0) {
        AWT_QT_LOCK;   //6212594
        /* Create pixmap and image with required width, height & depth. */
        pixmap = new QPixmap(width, height);
        if (pixmap == NULL) {
            AWT_QT_UNLOCK;   //6212594
            env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
            return NULL;
        }

	/* Fill the pixmap with the passed background color */
	QPainter painter(pixmap);
	
	if (background != NULL) {
	    QColor *fillColor = QtGraphics::getColor(env,
						     background);
	    bgColor = *fillColor;
	    delete fillColor;
	} else {
	    QRgb r = qRgba(0,0,0,0);
	    bgColor = QColor(r);
	}            
	painter.fillRect(0, 0, width, height, bgColor);
	
	image = new QImage(width,
                           height,
                           IMAGE_DEPTH);

        if (image == NULL) {
            env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
            AWT_QT_UNLOCK;   //6212594
            return NULL;
        }

        image->setAlphaBuffer(FALSE);

        imgRep = new ImageRepresentation(width, height, pixmap, image);
        if (imgRep == NULL) {
            env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass,
                           NULL);
            AWT_QT_UNLOCK;   //6212594
            return NULL;
        }

        // Attach data to this object
        env->SetIntField (thisObj, QtCachedIDs.
                          sun_awt_image_ImageRepresentation_pDataFID,
                          (jint)imgRep);
        AWT_QT_UNLOCK;   //6212594
        return imgRep;    
    }

    /* Invalid width and height for image. */
    env->ThrowNew (QtCachedIDs.java_awt_AWTErrorClass,
                   "Cannot create pixmap as width and height must be > 0");
    return NULL;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_initIDs (JNIEnv *env, jclass cls)
{
    QtCachedIDs.QtImageRepresentationClass = (jclass)
	env->NewGlobalRef (cls);

    GET_FIELD_ID(QtImageRepresentation_finishCalledFID,
		 "finishCalled", "Z");
    GET_FIELD_ID(QtImageRepresentation_drawSucceededFID,
		 "drawSucceeded", "Z");

    cls = env->FindClass ("sun/awt/image/ImageRepresentation");
    
    if (cls == NULL)
	return;

    GET_FIELD_ID (sun_awt_image_ImageRepresentation_pDataFID, "pData", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_availinfoFID, "availinfo",
                  "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_widthFID, "width", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_heightFID, "height", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_hintsFID, "hints", "I");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_newbitsFID, "newbits", 
                  "Ljava/awt/Rectangle;");
    GET_FIELD_ID (sun_awt_image_ImageRepresentation_offscreenFID, "offscreen",
                  "Z");
	
    cls = env->FindClass ("java/awt/image/ColorModel");
	
    if (cls == NULL)
        return;
		
    GET_METHOD_ID (java_awt_image_ColorModel_getRGBMID, "getRGB", "(I)I");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_offscreenInit (JNIEnv *env, 
						     jobject thisObj , 
						     jobject background)
{
    ImageRepresentation *imgRep;
  
/* Don't return if background is not specified,
   getImageRepresentationFromJNI will automatically use a default
   background if unspecified. Typically this will be null for
   BufferedImage case
 */
#if 0
    if (background == NULL)
	return;
#endif
	
    env->SetIntField(thisObj, 
		     QtCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID, 
		     (jint) IMAGE_OFFSCRNINFO);
    
    imgRep = ImageRepresentation:: getImageRepresentationFromJNI(env,
								 thisObj,
								 background,
                                                                 NULL) ;
    if (imgRep == NULL || imgRep->getPixmap() == NULL)
	return;  

    imgRep->setNumLines(imgRep->getHeight());
}


/* This convenience method performs argument checking for the
   setBytePixels and setIntPixels methods and creates a buffer, if
   necessary, to store the pixel values in before drawing them to the
   pixmap. ThisObj code is defined here so as to share as much code as
   possible between the two methods. */
   
ImageRepresentation *
ImageRepresentation::getImageRepresentationForSetPixels (JNIEnv *env,
							 jobject thisObj,
							 jint x,
							 jint y,
							 jint w,
							 jint h,
							 jobject colorModel,
							 jobject pixels,
							 jint offset,
							 jint scansize)
{
    ImageRepresentation *imgRep;
    int numLines ;
    /* Perform some argument checking. */
  
    if (thisObj == NULL || colorModel == NULL || pixels == NULL) {  
	env->ThrowNew(QtCachedIDs.NullPointerExceptionClass, NULL);
	return NULL;
    }
  
    if (x < 0 || y < 0 || w < 0 || h < 0
	|| scansize < 0 || offset < 0) {
	env->ThrowNew(QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
	return NULL;
    }
	
    if (w == 0 || h == 0)
	return NULL;
  
    if ((env->GetArrayLength((jintArray)pixels) < (offset + w)) 
	|| ((scansize != 0) && 
	 (((env->GetArrayLength((jintArray)pixels) - w - offset) /
	   scansize) < (h - 1)))) {
	env->ThrowNew(QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
	return NULL;
    }
	
    /* The rgb data has now been created for the supplied pixels. 
       First create a pixbuf representing these pixels and then 
       render them into the offscreen pixmap. */  
    imgRep = ImageRepresentation::getImageRepresentationFromJNI(env,
								thisObj,
								NULL,
                                                                NULL);	
    if (imgRep == NULL)
	return NULL;		

    /* Remember how many lines we have processed so far. */
    numLines = y + h;
    
    if (numLines > imgRep->getNumLines())
      imgRep->setNumLines(numLines);

    if (imgRep->isDirtyPixmap()) {
	imgRep->updateImage();
    }
    return imgRep;

}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_disposeImageNative (JNIEnv *env, jobject thisObj)
{      
    ImageRepresentation *imgRep = (ImageRepresentation *)env->
	GetIntField (thisObj, 
		     QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID); 
    if (imgRep != NULL) {
	AWT_QT_LOCK;
	delete imgRep;
	env->SetIntField (thisObj, 
			  QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID,
			  (jint)0);    
	AWT_QT_UNLOCK;
    }      
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_disposePixmapEntry (JNIEnv *env,
                                                          jobject thisObj,
                                                          jint p)
{      
    if (p != 0) {
        AWT_QT_LOCK;
	delete (QPixmap*)p;
	AWT_QT_UNLOCK;
    }
}

/* As the image is read in and passed through the java image filters thisObj 
 * method gets called to store the pixels in the native image representation. 
 * The GDK pixbuf library is used to store the pixels in a pixmap (a server 
 * side off screen image). The pixbuf library does the necessary scaling, 
 * alpha transparency and dithering. Typically, thisObj method is called for 
 * each line of the image so the whole image is not in memory at once. 
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtImageRepresentation_setBytePixels (JNIEnv *env,
                                                      jobject thisObj,
                                                      jint x,
                                                      jint y,
                                                      jint w,
                                                      jint h,
                                                      jobject colorModel,
                                                      jbyteArray pixels,
                                                      jint offset,
                                                      jint scansize)
{
    ImageRepresentation* imgRep;
    /* Stores red, green, blue bytes for each pixel */
    /* x, y position in pixel array - same variable names as Javadoc comment
     * for setIntPixels. 
     */
    jint m, n;		
    jint pixel;
    jbyte *pixelArray = NULL;

    jintArray rgbs = NULL;
    jint* rgbArray = NULL;
	
    imgRep = ImageRepresentation::
	getImageRepresentationForSetPixels (env, thisObj, x, y, w, h,
					    colorModel, pixels, offset,
					    scansize);

    if (imgRep == NULL || imgRep->getImage() == NULL) { 
        return JNI_FALSE;
    }
	
    pixelArray = env->GetByteArrayElements(pixels, NULL);
	
    if (pixelArray == NULL) {
        return JNI_FALSE;
    }	

    int red, green, blue, alpha;
    red = green = blue = alpha = 0;        

    /* Optimization to prevent callbacks into java for index color model. */
    if (env->IsInstanceOf (colorModel, QtCachedIDs.
                           java_awt_image_IndexColorModelClass)) {

        jint rgb;
        rgbs = (jintArray)env->GetObjectField(colorModel, QtCachedIDs.
					      java_awt_image_IndexColorModel_rgbFID);
			
        if (rgbs == NULL) {
            goto finish;
        }
				
        rgbArray = env->GetIntArrayElements(rgbs, NULL);
        
        if (rgbArray == NULL) {
            goto finish;
        }
			
	AWT_QT_LOCK;          

        /* For each pixel in the supplied pixel array look up its rgb value 
         * from the colorModel. 
         */
        for (n = 0; n < h; n++) {
	    for (m = 0; m < w; m++) {
		//Get the pixel at m, n.
		pixel = ((unsigned char)pixelArray[n * scansize + m + offset]);
		rgb = rgbArray[pixel];
            
		//Get and store red, green, blue values.             
		red = ((rgb & 0xff0000) >> 16);
		green = ((rgb & 0xff00) >> 8);
		blue = (rgb & 0xff);
		alpha = ((rgb >> 24) & 0xff);

		imgRep->setPixel(x+m, y+n, red, green, blue, alpha);
	    }          
        }        

	imgRep->setDirtyImage(TRUE);

        AWT_QT_UNLOCK;
    }
    /* No optimization possible - call back into Java. */
    else {
        jint rgb;
	AWT_QT_LOCK;
        for (n = 0; n < h; n++) {
            for (m = 0; m < w; m++) {

                /* Get the pixel at m, n. */
                pixel = ((unsigned char)pixelArray[n * scansize + m + offset]);
                rgb = env->CallIntMethod (colorModel, 
					  QtCachedIDs.java_awt_image_ColorModel_getRGBMID, pixel);
			
                if (env->ExceptionCheck()) {
		    AWT_QT_UNLOCK;           
                    goto finish;
                }
                
                /* Get and store red, green, blue values. */
                red = ((rgb & 0xff0000) >> 16);
                green = ((rgb & 0xff00) >> 8);
                blue = (rgb & 0xff);
                alpha = ((rgb >> 24) & 0xff);
                
		imgRep->setPixel(x+m, y+n, red, green, blue, alpha);
            }		  
        }     

	imgRep->setDirtyImage(TRUE);
    
        AWT_QT_UNLOCK;           
    }

 finish:	/* Perform tidying up code and return. */

    if (pixelArray != NULL)
        env->ReleaseByteArrayElements(pixels, pixelArray, JNI_ABORT);
    
    if (rgbArray != NULL)
        env->ReleaseIntArrayElements(rgbs, rgbArray, JNI_ABORT);
    return JNI_TRUE;
}

/* As the image is read in and passed through the java image filters thisObj method gets called
   to store the pixels in the native image representation. The GDK pixbuf library is used to
   store the pixels in a pixmap (a server side off screen image). The pixbuf library does the
   necessary scaling, alpha transparency and dithering. Typically, thisObj method is called for each
   line of the image so the whole image is not in memory at once. */

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtImageRepresentation_setIntPixels (JNIEnv *env,
                                                     jobject thisObj,
                                                     jint x,
                                                     jint y,
                                                     jint w,
                                                     jint h,
                                                     jobject colorModel,
                                                     jintArray pixels,
                                                     jint offset,
                                                     jint scansize)
{
    ImageRepresentation *imgRep;
    /* Stores red, green, blue bytes for each pixel */

    /* x, y position in pixel array - same variable names 
       as Javadoc comment for setIntPixels. */
    jint m, n;				
    jint pixel;
    jint *pixelArray = NULL;
    jintArray rgbs = NULL;
    jint *rgbArray = NULL;
    imgRep = ImageRepresentation::
	getImageRepresentationForSetPixels(env, thisObj, 
					   x, y, w, h, 
					   colorModel, 
					   pixels, 
					   offset, 
					   scansize);
  
    if (imgRep == NULL || imgRep->getImage() == NULL) {
	return JNI_FALSE;
    }
 
    pixelArray = env->GetIntArrayElements (pixels, NULL);
  
    if (pixelArray == NULL)
	goto finish;  

    /* Optimization to prevent callbacks into java for direct color model. */
    jint alpha, red, green, blue;
    if (env->IsInstanceOf (colorModel, 
			   QtCachedIDs.java_awt_image_DirectColorModelClass)) {	
	jint red_mask, red_offset, red_scale;
	jint green_mask, green_offset, green_scale;
	jint blue_mask, blue_offset, blue_scale;
	jint alpha_mask, alpha_offset, alpha_scale;
    
	red_mask = env->GetIntField(colorModel, QtCachedIDs.
				    java_awt_image_DirectColorModel_red_maskFID);
	red_offset = env->GetIntField(colorModel, QtCachedIDs.
				      java_awt_image_DirectColorModel_red_offsetFID);
	red_scale = env->GetIntField(colorModel, QtCachedIDs.
				     java_awt_image_DirectColorModel_red_scaleFID);
    
	green_mask = env->GetIntField(colorModel, QtCachedIDs.
				      java_awt_image_DirectColorModel_green_maskFID);
	green_offset = env->GetIntField(colorModel, QtCachedIDs.
					java_awt_image_DirectColorModel_green_offsetFID);
	green_scale = env->GetIntField(colorModel, QtCachedIDs.
				       java_awt_image_DirectColorModel_green_scaleFID);
    
	blue_mask = env->GetIntField(colorModel, QtCachedIDs.
				     java_awt_image_DirectColorModel_blue_maskFID);
	blue_offset = env->GetIntField(colorModel, QtCachedIDs.
				       java_awt_image_DirectColorModel_blue_offsetFID);
	blue_scale = env->GetIntField(colorModel, QtCachedIDs.
				      java_awt_image_DirectColorModel_blue_scaleFID);
    
	alpha_mask = env->GetIntField(colorModel, QtCachedIDs.
				      java_awt_image_DirectColorModel_alpha_maskFID);
	alpha_offset = env->GetIntField(colorModel, QtCachedIDs.
					java_awt_image_DirectColorModel_alpha_offsetFID);
	alpha_scale = env->GetIntField(colorModel, QtCachedIDs.
				       java_awt_image_DirectColorModel_alpha_scaleFID);
    
	AWT_QT_LOCK;		    

	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++) {		
	    for (m = 0; m < w; m++) { /* Get the pixel at m, n. */
		pixel = pixelArray[n * scansize + m + offset];
            
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

		imgRep->setPixel(x+m, y+n, red, green, blue, alpha);
	    }              
	}
	imgRep->setDirtyImage(TRUE);

	AWT_QT_UNLOCK;		    
	
    } /* Optimization to prevent callbacks into java for index color model. */
    else if (env->IsInstanceOf(colorModel, QtCachedIDs.
			       java_awt_image_IndexColorModelClass)) {	
	jint rgb;
      
	rgbs = (jintArray)env->GetObjectField(colorModel, QtCachedIDs.
					      java_awt_image_IndexColorModel_rgbFID);
      
	if (rgbs == NULL) {
	    goto finish;
	}
      
	rgbArray = env->GetIntArrayElements ( rgbs, NULL);
      
	if (rgbArray == NULL) {
	    goto finish;
	}
      
	AWT_QT_LOCK;
      
	/* For each pixel in the supplied pixel array look up 
	   its rgb value from the colorModel. */
	for (n = 0; n < h; n++) {
	    for (m = 0; m < w; m++) {
		/* Get the pixel at m, n. */
              
		pixel = pixelArray[n * scansize + m + offset];
		rgb = rgbArray[pixel];
              
		/* Get and store red, green, blue values. */
              
		red = ((rgb & 0xff0000) >> 16);
		green = ((rgb & 0xff00) >> 8);
		blue = (rgb & 0xff);
		alpha = ((rgb >> 24) & 0xff);
                
		imgRep->setPixel(x+m, y+n, red, green, blue, alpha);
	    }          
	}
	imgRep->setDirtyImage(TRUE);

	AWT_QT_UNLOCK;		    
    }
  
    /* No optimization possible - call back into Java. */
  
    else {    
	jint rgb;
            
	AWT_QT_LOCK;

	for (n = 0; n < h; n++) {	       
	    for (m = 0; m < w; m++) {
              
		/* Get the pixel at m, n. */        
		pixel = pixelArray[n * scansize + m + offset];
		rgb = env->CallIntMethod(colorModel, QtCachedIDs.
					 java_awt_image_ColorModel_getRGBMID, 
					 pixel);
              
		if (env->ExceptionCheck ()) {
		    AWT_QT_UNLOCK;		    
		    goto finish;
		}
              
		/* Get and store red, green, blue values. */              
		red = ((rgb & 0xff0000) >> 16);
		green = ((rgb & 0xff00) >> 8);
		blue = (rgb & 0xff);
		alpha = ((rgb >> 24) & 0xff);
              
		imgRep->setPixel(x+m, y+n, red, green, blue, alpha);
	    }          
	}
	imgRep->setDirtyImage(TRUE);

	AWT_QT_UNLOCK;		    
    }
  
 finish:	/* Perform tidying up code and return. */
  
    if (pixelArray != NULL)
	env->ReleaseIntArrayElements ( pixels, pixelArray, JNI_ABORT);
  
    if (rgbArray != NULL)
	env->ReleaseIntArrayElements ( rgbs, rgbArray, JNI_ABORT);

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtImageRepresentation_finish (JNIEnv *env, jobject thisObj, jboolean force)
{	   
    ImageRepresentation* imgRep;    

    /*
     * If setImageNative() has been called then we simply return 
     */
    int isFinished = env->GetIntField(thisObj,
                          QtCachedIDs.QtImageRepresentation_finishCalledFID) ;
    if ( isFinished == JNI_TRUE )
        goto done ;

    AWT_QT_LOCK;		    

    imgRep = ImageRepresentation:: getImageRepresentationFromJNI(env,
                                                                 thisObj,
                                                                 NULL,
                                                                 NULL);	
    if (imgRep != NULL) {        
        imgRep->finish();
        env->SetIntField (thisObj,
			  QtCachedIDs.QtImageRepresentation_finishCalledFID, 
			  JNI_TRUE);
    }
    AWT_QT_UNLOCK;		    

done :
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_setNativeImage(JNIEnv *env, 
                                                     jobject thisObj, 
                                                     jint qimage) {
    ImageRepresentation* imgRep;    

    /*
     * We need to make a copy of the QImage. QImage.data which contains
     * the pixel data is QShared, so it does not get copied and just
     * gets reference counted, so we dont take up lot of space
     */
    QImage *image = new QImage(*(QImage *)qimage) ;

    imgRep = ImageRepresentation:: getImageRepresentationFromJNI(env,
								                                 thisObj,
                                                                 NULL,
                                                                 image) ;
    if (imgRep != NULL) {        
        /* finish() sets the image's alpha buffer to FALSE if 
         * ImageRepresentation::hasAlpha flag is FALSE. The flag will be false
         * in the native decoder case, since we bypass setPixel() where it
         * can be turned on. Next thing finish() peforms is updating the
         * pixmap. Since we dont want to turn off alpha  if the decoded 
         * image has already alpha turned on we directly call 
         * "updatePixmap()"
         *
         * This fixes the issue of rendering transparent PNG prblem
         */
        //imgRep->finish();
        AWT_QT_LOCK;		    
        imgRep->updatePixmap();
        AWT_QT_UNLOCK;		    
        env->SetIntField (thisObj,
			  QtCachedIDs.QtImageRepresentation_finishCalledFID, 
			  JNI_TRUE);
    }


    return ;
}

/* Gets the native data for the image representation and the graphics object
 * for drawing the image. Also checks to make sure the data is suitable for 
 * drawing images and throws exceptions if there is a problem. ThisObj function
 * is defined so as to share as much code as possible between the two image 
 * drawing methods of ImageRepresentation (imageDraw and imageStretch). 
 */
bool
ImageRepresentation::getDataForImageDrawing (JNIEnv *env,
                                             jobject thisObj,
                                             jobject g,
                                             ImageRepresentation **imgRep,
                                             QtGraphics** gfx)
{
    jint availInfo;

    if (g != NULL) {
       if (env->IsInstanceOf(g, QtCachedIDs.QtGraphicsClass)) {
           // Check width and height of image is known.
           availInfo = env->GetIntField(thisObj,
                    QtCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

           if ((availInfo & IMAGE_SIZE_INFO) == IMAGE_SIZE_INFO) {
               *gfx = (QtGraphics*)env->GetIntField(g,
                              QtCachedIDs.QtGraphics_dataFID);

               if (*gfx != NULL && (*gfx)->hasPaintDevice() != FALSE ) {
                   *imgRep = (ImageRepresentation*)env->GetIntField(thisObj,
                       QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
                   if (*imgRep != NULL && (*imgRep)->getPixmap() != NULL) {
                       return TRUE;
                   } else if (*imgRep == NULL) {
                       *imgRep = ImageRepresentation::
                                 createImageRepresentationFromJNI(env,
                                                                 thisObj,
                                                                 NULL);
                       if (*imgRep != NULL && (*imgRep)->getPixmap() != NULL) {
                           return TRUE;
                       }
                   }
                   return FALSE;
               } else {
                   env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
                   return FALSE;
               }
           }
           return FALSE;
       }
       env->ThrowNew (QtCachedIDs.IllegalArgumentExceptionClass, NULL);
       return FALSE;
    }
    env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
    return FALSE;
}

void
ImageRepresentation::drawUnscaledImage (JNIEnv* env, 
					QtGraphics* gfx, 
					int xsrc, 
					int ysrc,
					int xdest, 
					int ydest, 
					int drawWidth, 
					int drawHeight)
{
    /* check if loading an image - need to convert to pixmap */
    if (this->isDirtyImage()) {
	this->updatePixmap();
    }
    gfx->drawPixmap( xdest, ydest, *this->pixmap,
		     xsrc, ysrc, drawWidth, drawHeight );
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_imageDraw (JNIEnv * env, 
						 jobject thisObj,
						 jobject g, 
						 jint x, 
						 jint y, 
						 jobject c)
{
    QtGraphics* gfx;
    ImageRepresentation *imgRep;

    AWT_QT_LOCK;

    if (g != NULL) {
       if (env->IsInstanceOf(g, QtCachedIDs.QtGraphicsClass)) {
           // Check width and height of image is known.
           jint availInfo = env->GetIntField(thisObj,
                    QtCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

           if ((availInfo & IMAGE_SIZE_INFO) == IMAGE_SIZE_INFO) {
               gfx = (QtGraphics*)env->GetIntField(g,
                              QtCachedIDs.QtGraphics_dataFID);

               if (gfx != NULL && gfx->hasPaintDevice() != FALSE ) {
                   imgRep = (ImageRepresentation*)env->GetIntField(thisObj,
                       QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
                   if (imgRep != NULL && imgRep->getPixmap() != NULL) {
                       goto draw_image;
                   } else if (imgRep == NULL) {
                       imgRep = ImageRepresentation::
                                createImageRepresentationFromJNI(env,
                                                                thisObj,
                                                                NULL);
                       if (imgRep != NULL && imgRep->getPixmap() != NULL) {
                           goto draw_image;
                       }
                   }
                   goto done;
               } else {
                   env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
                   goto done;
               }
           }
           goto done;
       }
       env->ThrowNew (QtCachedIDs.IllegalArgumentExceptionClass, NULL);
       goto done;
    }
    env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
    goto done;

draw_image:
    env->SetIntField (thisObj,
		      QtCachedIDs.QtImageRepresentation_drawSucceededFID, JNI_TRUE);

    x += env->GetIntField (g, QtCachedIDs.QtGraphics_originXFID);
    y += env->GetIntField (g, QtCachedIDs.QtGraphics_originYFID);

    if (!gfx->hasClipping()) {
        /* Draw whole image */
        if (c != NULL) {
            QColor* color = QtGraphics::getColor(env, c);
            gfx->setColor(color);
            gfx->drawRect(x, y, imgRep->getWidth(), 
                          imgRep->getHeight(), TRUE);
        }
        imgRep->drawUnscaledImage (env, gfx, 0, 0,
                            x, y, imgRep->getWidth(), imgRep->getHeight());
        AWT_QT_UNLOCK;
        return;
    } else {
        /* If a clip rectangle has been set then calculate the intersection of
         * the clip rectangle with the image and only draw this portion of the
         * image.
         */
        int xsrc, ysrc, xdest, ydest, width, height;
        QRect imageRect, intersection;
        imageRect.setX(x);
        imageRect.setY(y);
        imageRect.setWidth(imgRep->getWidth());
        imageRect.setHeight(imgRep->getHeight());
        intersection = imageRect.intersect(*gfx->getClipBounds());
   
        if (imageRect.intersects(*gfx->getClipBounds())) {
            xdest = intersection.x();
            ydest = intersection.y();
            xsrc = xdest - x;
            ysrc = ydest - y;
            width = intersection.width();
            height = intersection.height();

            if (c != NULL) {
                QColor* color;
                color = QtGraphics::getColor(env, c);
                gfx->setColor(color);
                gfx->drawRect(xdest, ydest, width, height, TRUE);
            }

            imgRep->drawUnscaledImage (env, gfx, xsrc, ysrc,
                               xdest, ydest, width, height);
        }
    }
done:
    AWT_QT_UNLOCK;
}

/* Optimized version of imageDraw */
JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtImageRepresentation_imageDrawDirect (JNIEnv * env, 
						       jobject thisObj,
						       jint gpointer, 
						       jint x, 
						       jint y, 
						       jobject c)
{
    QtGraphics* gfx = (QtGraphics *) gpointer;
    /* Shouldn't be null */
    ImageRepresentation *imgRep = (ImageRepresentation*)
	env->GetIntField(thisObj,
			 QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
    
    if (imgRep == NULL) {
	return JNI_FALSE;
    }

    AWT_QT_LOCK;

    if (!gfx->hasClipping()) {
        /* Draw whole image */
        if (c != NULL) {
            QColor* color = QtGraphics::getColor(env, c);
            gfx->setColor(color);
            gfx->drawRect(x, y, imgRep->getWidth(), 
                          imgRep->getHeight(), TRUE);
        }
        imgRep->drawUnscaledImage (env, gfx, 0, 0,
                            x, y, imgRep->getWidth(), imgRep->getHeight());
        AWT_QT_UNLOCK;
        return JNI_TRUE;
    } else {
        /* If a clip rectangle has been set then calculate the intersection of
         * the clip rectangle with the image and only draw this portion of the
         * image.
         */
        int xsrc, ysrc, xdest, ydest, width, height;
        QRect imageRect, intersection;
        imageRect.setX(x);
        imageRect.setY(y);
        imageRect.setWidth(imgRep->getWidth());
        imageRect.setHeight(imgRep->getHeight());
        intersection = imageRect.intersect(*gfx->getClipBounds());
   
        if (imageRect.intersects(*gfx->getClipBounds())) {
            xdest = intersection.x();
            ydest = intersection.y();
            xsrc = xdest - x;
            ysrc = ydest - y;
            width = intersection.width();
            height = intersection.height();

            if (c != NULL) {
                QColor* color;
                color = QtGraphics::getColor(env, c);
                gfx->setColor(color);
                gfx->drawRect(xdest, ydest, width, height, TRUE);
            }

            imgRep->drawUnscaledImage (env, gfx, xsrc, ysrc,
                               xdest, ydest, width, height);
        }
    }
    AWT_QT_UNLOCK;
    return JNI_TRUE;
}

/* Scales the sourceImage to the destImage using the supplied scaling factors.*/
/*
  void
  ImageRepresentation::scaleImage (QImage *sourceImage, QImage *destImage, 
  double scaleX, double scaleY)
  {
  int x, y;
  int sourceX, sourceY;
  int sourceWidth = sourceImage->width() - 1, 
  sourceHeight = sourceImage->height() - 1;
  int destWidth = destImage->width(), 
  destHeight = destImage->height();
  int sx, sy;
  
  scaleX = 1.0 / scaleX;
  scaleY = 1.0 / scaleY;
  
  sx = (scaleX < 0) ? sourceWidth : 0;
  sy = (scaleY < 0) ? sourceHeight : 0;
  
  //printf("In scaleImage \n");
  for (y = 0; y < destHeight; y++)
  {
  for (x = 0; x < destWidth; x++)
  {
  sourceX = sx + (int)(x * scaleX);
  sourceY = sy + (int)(y * scaleY);
          
  if (sourceX > sourceWidth)
  sourceX = sourceWidth;
          
  if (sourceY > sourceHeight)
  sourceY = sourceHeight;
          
  destImage->setPixel(x, y, sourceImage->pixel(sourceX, sourceY));
  }
  }
  }
*/

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_imageStretch (JNIEnv * env,
                                                     jobject thisObj,
                                                     jobject g,
                                                     jint dx1,
                                                     jint dy1,
                                                     jint dx2,
                                                     jint dy2,
                                                     jint sx1,
                                                     jint sy1,
                                                     jint sx2,
                                                     jint sy2,
                                                     jobject c)
{
    QtGraphics* gfx;
    ImageRepresentation *imgRep;
    jint originX, originY;
    jint sourceWidth, sourceHeight, destWidth, destHeight;
    double scaleX, scaleY;
    QImage sourceImage, destImage;
    QRect imageRect, sourceRect, destRect, intersection;
    bool hMirror, vMirror;
  
    AWT_QT_LOCK;
    
    if (!ImageRepresentation::
	getDataForImageDrawing (env, thisObj, g, &imgRep, &gfx)) {
	AWT_QT_UNLOCK;
	return;
    }

    /* Translate destination area. */
  
    originX = env->GetIntField (g, QtCachedIDs.QtGraphics_originXFID);
    originY = env->GetIntField (g, QtCachedIDs.QtGraphics_originYFID);
    dx1 += originX;
    dx2 += originX;
    dy1 += originY;
    dy2 += originY;
  
    /* Calculate scale factors (negative scales imply a flip). */
  
    sourceWidth = sx2 - sx1;
  
    if (sourceWidth >= 0)
	sourceWidth++;
  
    else sourceWidth--;

    /* A QImage's width and height are limited to 32767, so we make
       sure that we don't scale to greater than that. */
  
    destWidth = dx2 - dx1;

    if (destWidth >= 0) {
	if (destWidth >= 32767) {
	    destWidth = 32767;
	} else {
	    destWidth++;
	}
    } else {
	if (destWidth <= -32767) {
	    destWidth = -32767;
	} else {
	    destWidth--;
	}
    }
  
    sourceHeight = sy2 - sy1;
  
    if (sourceHeight >= 0)
	sourceHeight++;
  
    else sourceHeight--;
  
    destHeight = dy2 - dy1;
  
    if (destHeight >= 0) {
	if (destHeight >= 32767) {
	    destHeight = 32767;
	} else {
	    destHeight++;
	}
    } else {
	if (destHeight <= -32767) {
	    destHeight = -32767;
	} else {
	    destHeight--;
	}
    }
  
    scaleX = ((double)destWidth) / sourceWidth;
    scaleY = ((double)destHeight) / sourceHeight;
    hMirror = ( scaleX < 0 );
    vMirror = ( scaleY < 0 );

    destRect.setX(QMIN(dx1, dx2));
    destRect.setY(QMIN(dy1, dy2));
    destRect.setWidth(abs(destWidth));
    destRect.setHeight(abs(destHeight));
  
    sourceRect.setX(QMIN(sx1, sx2));
    sourceRect.setY(QMIN(sy1, sy2));
    sourceRect.setWidth(abs(sourceWidth));
    sourceRect.setHeight(abs(sourceHeight));
  
    /* Calculate intersection of destination rectangle with the 
       clip bounds on the graphics if one has been set. */
  
    if (gfx->hasClipping()) {    
	intersection = destRect.intersect(*gfx->getClipBounds());
	if (destRect.intersects(*gfx->getClipBounds())) {        
	    int x1, y1, x2, y2;
	    QRect tempRect;
          
	    destRect = intersection;
          
	    /* 
             * Calculate what portion of the source image this
             * represents. The first source coordinate corresponds to
             * the first dest coordinate and the second source
             * coordinate corresponds to the second dest coordinate
             * always (flipping the image if necessary). 
             */
          
	    x1 = sx1 + (int)((destRect.x() - dx1) / scaleX);
	    y1 = sy1 + (int)((destRect.y() - dy1) / scaleY);
	    x2 = sx1 + (int)((destRect.x() + destRect.width() - 1 - dx1) / scaleX);
	    y2 = sy1 + (int)((destRect.y() + destRect.height() - 1 - dy1) / scaleY);
          
	    tempRect.setX(QMIN(x1, x2));
	    tempRect.setY(QMIN(y1, y2));
	    tempRect.setWidth(abs(x2 - x1) + 1);
	    tempRect.setHeight(abs(y2 - y1) + 1);
	    intersection = sourceRect.intersect(tempRect);
	    if (sourceRect.intersects(tempRect))
		sourceRect = intersection;
      
	    else {
		/* No intersection - nothing to draw. */
		AWT_QT_UNLOCK;
		return;	
	    }
	}
      
	else {
	    /* No intersection - nothing to draw. */
	    AWT_QT_UNLOCK;
	    return;
	}
    }
  
    /*
     * Make sure source area intersects with the image. We use
     * numLines instead of height as we only want to draw areas
     * of the image for which we have received pixels.
     */
  
    imageRect.setX(0);
    imageRect.setY (0);
    imageRect.setWidth(imgRep->getWidth());
    imageRect.setHeight(imgRep->getNumLines());
    intersection = sourceRect.intersect(imageRect);

    if (sourceRect.intersects(imageRect)) {
        sourceRect = intersection;
        /* 
         * 6643917:
         * Adjusting dest rect to match source rect
         * if the latter exceeds the source image size.
         */
        destRect.setWidth((int)(sourceRect.width() * scaleX));
        destRect.setHeight((int)(sourceRect.height() * scaleY));
    }
    else {
	/* No intersection - nothing to draw. */
	AWT_QT_UNLOCK;
	return;	
    }
  
    if (sourceRect.width() == 0 || sourceRect.height() == 0 || 
	destRect.width() == 0 || destRect.height() == 0) {
	AWT_QT_UNLOCK;
	return;
    }

    /* Fill with background color. */
  
    if (c != NULL) {    
	QColor* color;	
    
	color = QtGraphics::getColor(env, c);       
	gfx->setColor(color);
	gfx->drawRect(destRect.x(), destRect.y(), destRect.width(), 
		      destRect.height(), TRUE);
    }
  
    /* Optimization to draw image unscaled if we can. */
  
    if (scaleX == 1.0 && scaleY == 1.0) { 
	int minw = QMIN(sourceRect.width(), destRect.width());
	int minh = QMIN(sourceRect.height(), destRect.height());
	imgRep->drawUnscaledImage (env,
				   gfx,
				   sourceRect.x(),
				   sourceRect.y(),
				   destRect.x(),
				   destRect.y(),
				   minw,
				   minh);
	AWT_QT_UNLOCK;
	return;
    }

    if (imgRep->isDirtyPixmap()) {
	imgRep->updateImage();
    }
  
    sourceImage = imgRep->getImage()->copy(sourceRect.x(), 
					   sourceRect.y(), 
					   sourceRect.width(), 
					   sourceRect.height());
    /*
     * Optimization. If our scaling factors are exactly -1.0
     * and/or 1.0, then this is a simple image flip and doesn't
     * need to be scaled.
     */
    if ( ( scaleX == 1.0 || scaleX == -1.0 ) &&
	 ( scaleY == 1.0 || scaleY == -1.0 ) ) {
	destImage = sourceImage.mirror( hMirror, vMirror );
    } else {
	// We are scaling and perhaps inverting.
	destImage = sourceImage.smoothScale(destRect.width(), 
					    destRect.height());
	  
	if ( vMirror || hMirror ) {
	    destImage = destImage.mirror( hMirror, vMirror );
	}
    }
    
    // Create a temporary pixmap to hold the changed image.
    // We don't want to set the bits in the imgRep because
    // we'd be changing the source image itself.
    QPixmap tmpPm;
    
    tmpPm.convertFromImage( destImage );
    gfx->drawPixmap( destRect.x(), destRect.y(), 
		     tmpPm );
    AWT_QT_UNLOCK;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtImageRepresentation_createScaledQPixmap (JNIEnv * env,
                                                           jobject thisObj,
                                                           jint w,
                                                           jint h)
{
    ImageRepresentation *imgRep;
  
    AWT_QT_LOCK;
    
    imgRep = (ImageRepresentation*)env->
        GetIntField(thisObj,
                    QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);

    if (imgRep == NULL) {
        AWT_QT_UNLOCK;
	return 0;
    }

    if (imgRep->isDirtyPixmap()) {
        imgRep->updateImage();
    }

    QImage* src = imgRep->getImage();

    QPixmap* pm = new QPixmap();

    if (pm == NULL) {
        env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass,
                      NULL);
        AWT_QT_UNLOCK;
        return 0;
    }

#ifdef QWS
                                                                              
    // CR 6287712.
    // According to the QImage documentation, its width and height
    // is limited to 32767, for qt 2.x releases.  
    // But cutoff at 32767 does not work on
    // zaurus and results in a seg fault.  So we cutoff at the screen
    // width and height.
    /*
    if (w > 32767) w = 32767;
    if (h > 32767) h = 32767;
    */
    QWidget *d = QApplication::desktop();
    int sw = d->width();        // returns screen width
    int sh = d->height();       // returns screen height
                                                                              
    if (w > sw) {  w = sw; }
    if (w < -sw) {  w = -sw; }
    if (h > sh) {  h = sh; }
    if (h < -sh) {  h = -sh; }
                                                                              
#endif /* QWS */

    pm->convertFromImage(src->smoothScale(w, h));

    AWT_QT_UNLOCK;

    return (jint)pm;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_drawQPixmap (JNIEnv * env,
                                                   jobject thisObj,
                                                   jobject g,
                                                   jint p,
                                                   jint x,
                                                   jint y,
                                                   jobject c)
{
    QtGraphics* gfx = NULL;
    QPixmap* pm = (QPixmap*)p;

    AWT_QT_LOCK;
    
    if (g != NULL) {
        gfx = (QtGraphics*)
            env->GetIntField(g, QtCachedIDs.QtGraphics_dataFID);
    }

    if (gfx == NULL || pm == NULL) {
        AWT_QT_UNLOCK;
        return;
    }

    // Fixed 6178052: the DemoFrame program not rendered correctly problem
    // where the Duke images never appear on the GraphicsDemo.
    // The translation was not done.

    /* Translates to the graphics context origin first. */
  
    jint originX = env->GetIntField (g, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (g, QtCachedIDs.QtGraphics_originYFID);

    x += originX;
    y += originY;

    if (c != NULL) {
        QColor* color;
        color = QtGraphics::getColor(env, c);
        gfx->setColor(color);
        gfx->drawRect((int)x, (int)y, pm->width(), pm->height(), TRUE);
    }

    gfx->drawPixmap((int)x, (int)y, *pm);

    AWT_QT_UNLOCK;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtImageRepresentation_getRGB (JNIEnv * env,
					      jobject thisObj,
					      jobject cm,
					      jint x,
					      jint y) 
{
    ImageRepresentation *imgRep = (ImageRepresentation *)env->
	GetIntField (thisObj, 
		     QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID); 

    if (imgRep != NULL) {    
	if (x < 0 || y < 0 || x >= imgRep->getWidth() || 
	    y >= imgRep->getHeight()) {
	    env->ThrowNew
		(QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass, 
		 "Value out of bounds");
	    return 0;
	}

	AWT_QT_LOCK;

	// we do this so that we get the color conversion properly
	if (imgRep->isDirtyPixmap()) {
	    imgRep->updateImage();
	}

	QImage img;
	
	img = *imgRep->getImage();

	QRgb pixel = img.pixel(x, y);
	jint rgb;
#ifdef QWS
        rgb = get565DirectColorModelRGB(qRed(pixel),
                                        qGreen(pixel), 
                                        qBlue(pixel));
#else
	rgb = getDirectColorModelRGB(qRed(pixel),
                                     qGreen(pixel), 
                                     qBlue(pixel));
#endif
	/*
	jint rgb = env->CallIntMethod(cm,
				      QtCachedIDs.java_awt_image_ColorModel_getRGBMID, 
				      (jint) pixel);
	*/

	AWT_QT_UNLOCK;

	return rgb;
    } else {
	env->ThrowNew(QtCachedIDs.NullPointerExceptionClass, NULL);
	return 0;
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageRepresentation_getRGBs (JNIEnv * env,
					       jobject thisObj,
					       jobject cm,
					       jint startX,
					       jint startY,
					       jint w,
					       jint h,
					       jintArray rgbArray,
					       jint offset,
					       jint scansize) 
{
    jint *pixelArrayElements;

    ImageRepresentation *imgRep = (ImageRepresentation *)env->
	GetIntField (thisObj, 
		     QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID); 

    if (imgRep != NULL) {    
	if (startX < 0 || 
	    startY < 0 || 
	    (startX + w) > imgRep->getWidth() || 
	    (startY + h) > imgRep->getHeight()) {
	    env->ThrowNew
		(QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass, 
		 "Value out of bounds");
	    return;
	}

	if ((pixelArrayElements = 
	     env->GetIntArrayElements(rgbArray,
				      NULL)) == NULL) {
	    env->ThrowNew(QtCachedIDs.NullPointerExceptionClass, NULL);
	    return;
	}

	AWT_QT_LOCK;

	// we do this so that we get the color conversion properly
	if (imgRep->isDirtyPixmap()) {
	    imgRep->updateImage();
	}
	
	QImage img;
	
	img = *imgRep->getImage();

	for (int y = startY; y < startY + h; y++) {
	    for (int x  = startX; x < startX + w; x++) {
		QRgb pixel = img.pixel(x, y);
		jint rgb;
#ifdef QWS
                rgb = get565DirectColorModelRGB(qRed(pixel),
                                                qGreen(pixel), 
                                                qBlue(pixel));
#else
                rgb = getDirectColorModelRGB(qRed(pixel),
                                             qGreen(pixel), 
                                             qBlue(pixel));
#endif
		/*
		jint rgb = env->CallIntMethod(cm,
					      QtCachedIDs.java_awt_image_ColorModel_getRGBMID, 
					      (jint) pixel);
		*/
		pixelArrayElements[offset + (y - startY) * scansize +
				       (x - startX)] = rgb;
	    }
	}

	AWT_QT_UNLOCK;

	env->ReleaseIntArrayElements(rgbArray, pixelArrayElements, 0);
    } else {
	env->ThrowNew(QtCachedIDs.NullPointerExceptionClass, NULL);
    }
}

// 6223113: PP-TCK AlphaComposite test failure.
//
// The test failure was due to the fact that on Qt/X11 platforms, the color
// model returned was #ifdef'ed to be always a direct color model with image
// type as java_awt_image_BufferedImage_TYPE_INT_RGB.
//
// The proper way is to call Qt API QPixmap::defaultDepth() to check the
// the hardware depth of the current video mode and drill down if on an X11
// platform to check the default Visual (Xlib API)'s rgb masks for a depth
// of 16 to see whether it is 565 or 555.
//
// The JNI method sun.awt.qt.QtToolkit.getColorModelNative() now delegates
// to ImageRepresentation::defaultColorModel().

jobject
ImageRepresentation::defaultColorModel(JNIEnv *env)
{
    jobject colorModel = NULL;

    jint imageType = ImageRepresentation::defaultImageType();

    if (imageType == java_awt_image_BufferedImage_TYPE_USHORT_565_RGB) {
	colorModel =
	    env->NewObject(QtCachedIDs.java_awt_image_DirectColorModelClass,
			   QtCachedIDs.java_awt_image_DirectColorModel_constructorMID,
			   16,
			   0xf800,  // red: 5
			   0x07e0,  // green: 6
			   0x001f); // blue: 5
    } else if (imageType == java_awt_image_BufferedImage_TYPE_USHORT_555_RGB) {
	colorModel =
	    env->NewObject(QtCachedIDs.java_awt_image_DirectColorModelClass,
			   QtCachedIDs.java_awt_image_DirectColorModel_constructorMID,
			   15,	    // 15 significant bits
			   0x7c00,  // red: 5
			   0x03e0,  // green: 5
			   0x001f); // blue: 5
    } else if (imageType == java_awt_image_BufferedImage_TYPE_INT_RGB) {
	colorModel =
	    env->NewObject(QtCachedIDs.java_awt_image_DirectColorModelClass,
			   QtCachedIDs.java_awt_image_DirectColorModel_constructorMID,
			   32,
			   0x00ff0000,
			   0x0000ff00,
			   0x000000ff);
    } else if (imageType == java_awt_image_BufferedImage_TYPE_BYTE_INDEXED) {
	colorModel = defaultIndexColorModel(env, imageType, QPixmap::defaultDepth());
    }

    return colorModel;
}

jint
ImageRepresentation::defaultImageType()
{
    static bool defaultKnown = false;
    static jint defaultType = -1;

    if (defaultKnown) {
        return defaultType;
    }

    AWT_QT_LOCK;

    int depth = QPixmap::defaultDepth();

    jint type = java_awt_image_BufferedImage_TYPE_INT_RGB;

#ifdef Q_WS_X11
    Visual* visual;
#endif /* ifdef Q_WS_X11 */

    switch (depth) {
    case 8:
        // Qt 3.3.3 assumes that 8bpp == pseudocolor?
        type = java_awt_image_BufferedImage_TYPE_BYTE_INDEXED;
        break;
    case 15:
        type = java_awt_image_BufferedImage_TYPE_USHORT_555_RGB;
        break;
    case 16:
#ifdef Q_WS_X11
        visual = (Visual*) QPaintDevice::x11AppVisual();
        if (visual->green_mask == 0x7e0) {
            type = java_awt_image_BufferedImage_TYPE_USHORT_565_RGB;
        } else if (visual->green_mask == 0x3e0) {
            type = java_awt_image_BufferedImage_TYPE_USHORT_555_RGB;
        }
#else
        type = java_awt_image_BufferedImage_TYPE_USHORT_565_RGB;
#endif /* ifdef Q_WS_X11 */
        break;
    case 24:
        break;
    case 32:
        break;
    default:
        break;
    }

    AWT_QT_UNLOCK;

    defaultKnown = true;
    defaultType = type;

    return type;
}

jobject
ImageRepresentation::defaultIndexColorModel(JNIEnv *env, jint type, int depth)
{
    jobject colorModel = NULL;

    if (type == java_awt_image_BufferedImage_TYPE_BYTE_INDEXED) {

/*
 * Qt/X11 has QImage::colorTable() API to return the color table for the
 * qimage object.  But the color table returned is specific to the pixel
 * data in the qimage object.  So just creating an empty qpixmap and then
 * converting it into a qimage and expecting its color table to contain
 * the full default system color map is not enough.  So we go to Xlib to
 * query the default application color map entries.
 */

        int numColors = 0;

#ifdef Q_WS_X11
        // Find out the number of entries in the default color map.
        numColors = QPaintDevice::x11AppCells();
#endif /* ifdef Q_WS_X11 */

        if (numColors <= 0) {
            return NULL;
        }

#ifdef Q_WS_X11
        // Populate the XColor array before calling XQueryColors().
        XColor* carr = new XColor[numColors];
        for (int i = 0; i < numColors; i++) {
            carr[i].pixel = i;
        }

        // Get default colormap.
        XQueryColors(QPaintDevice::x11AppDisplay(),
                     QPaintDevice::x11AppColormap(),
                     carr,
                     numColors);
#endif /* ifdef Q_WS_X11 */

        jbyteArray redArray = env->NewByteArray(numColors);
        jbyteArray greenArray = env->NewByteArray(numColors);
        jbyteArray blueArray = env->NewByteArray(numColors);

        if (redArray != NULL && greenArray != NULL && blueArray != NULL) {

            jbyte val;
            for (int i = 0; i < numColors; i++) {
#ifdef Q_WS_X11
                val = (jbyte) ((carr[i].red >> 8) & 255);
#endif /* ifdef Q_WS_X11 */
                env->SetByteArrayRegion(redArray, i, 1, &val);

#ifdef Q_WS_X11
                val = (jbyte) ((carr[i].green >> 8) & 255);
#endif /* ifdef Q_WS_X11 */
                env->SetByteArrayRegion(greenArray, i, 1, &val);

#ifdef Q_WS_X11
                val = (jbyte) ((carr[i].blue >> 8) & 255);
#endif /* ifdef Q_WS_X11 */
                env->SetByteArrayRegion(blueArray, i, 1, &val);
            }

            colorModel = env->NewObject(QtCachedIDs.java_awt_image_IndexColorModelClass,
                                        QtCachedIDs.java_awt_image_IndexColorModel_constructorMID,
                                        (jint) depth,
                                        (jint) numColors,
                                        redArray,
                                        greenArray,
                                        blueArray);
        }

        env->DeleteLocalRef(redArray);
        env->DeleteLocalRef(greenArray);
        env->DeleteLocalRef(blueArray);
    }

    return colorModel;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtImageRepresentation_getType (JNIEnv * env,
                                               jobject thisObj)
{
    return ImageRepresentation::defaultImageType();
}

