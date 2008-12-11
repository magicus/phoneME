/*
 * @(#)OffScreenImageSource.cc	1.10 06/10/25
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
#include "jni.h"
#include "awt.h"
#include "ImageRepresentation.h"
#include "java_awt_image_ImageConsumer.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include <assert.h>
#include "QtApplication.h"

#include <stdio.h>

JNIEXPORT void JNICALL
Java_sun_awt_image_OffScreenImageSource_sendPixels(JNIEnv * env, 
                                                   jobject thisObj)
{
    jobject imageRep, theConsumer, colorModel;
    jint availInfo;
    ImageRepresentation *imageRepData;
    jint x, y;
    jobject pixelArray;
    jint width, height;
    imageRep = env->GetObjectField(thisObj,
				   QtCachedIDs.sun_awt_image_OffScreenImageSource_baseIRFID); 
  
    if (imageRep == NULL) {   
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }
  
    availInfo = env->GetIntField (imageRep,
                QtCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID); 
  
    if ((availInfo & IMAGE_SIZE_INFO) != IMAGE_SIZE_INFO)
        return;
  
    imageRepData = ImageRepresentation::getImageRepresentationFromJNI(env, 
                                                              imageRep, 
                                                              NULL);
  
    theConsumer = env->GetObjectField (thisObj,
		QtCachedIDs.sun_awt_image_OffScreenImageSource_theConsumerFID); 
  
    if (theConsumer == NULL)
        return;

    // 6255338 Bug Fix 
    //
    // The following were the issues with the old code
    // - It hardoded a 565 color model, which did not reflect the the
    //   depth of the image.
    // - It assumed that if the depth was 8, then QImage::pixel(x,y) returns
    //   the 8 bit index, which is not true. In fact QImage.pixel(x,y) 
    //   returns the pixel data in 24bit depth irrespective of the QImage's
    //   depth (in case of Qt/X11) and in the same depth of the QImage 
    //   (in case of QWS)
    //
    // The fix is as follows
    // - Always create a colorModel that represents 24bit depth, since
    //   QImage::pixel(x,y) gives the pixel data in that format for any
    //   depth (Qt/X11) and for QWS, create a colormodel with pixel size
    //   as 32 (to reflect the return value of QImage::pixel()) with
    //   rgb masks refelecting the depth of the QImage.
    // - Removed the code that was handling 8 bit depth, i.e creating
    //   byte arrays. Now we simply create an int[]
#ifdef QWS
    switch (QPixmap::defaultDepth()) {
    case 16 :
        colorModel = 
        env->NewObject(QtCachedIDs.java_awt_image_DirectColorModelClass,
			   QtCachedIDs.java_awt_image_DirectColorModel_constructorMID,
			   32,
               0x00f80000,
               0x0000fc00,
               0x000000f8);
        break;
    case 15 :
        colorModel = 
        env->NewObject(QtCachedIDs.java_awt_image_DirectColorModelClass,
			   QtCachedIDs.java_awt_image_DirectColorModel_constructorMID,
			   32,
               0x00f80000,
               0x0000f800,
               0x000000f8);
        break;
    default :
        colorModel = ImageRepresentation::defaultColorModel(env);
        break;
    }
#else
    colorModel = env->NewObject(
             QtCachedIDs.java_awt_image_DirectColorModelClass,
             QtCachedIDs.java_awt_image_DirectColorModel_constructorMID,
             32,
             0x00ff0000,   // red mask
             0x0000ff00,   // green mask
             0x000000ff);  // blue mask
#endif

    if (env->ExceptionCheck())
        return;
  
    env->CallVoidMethod (theConsumer,
         QtCachedIDs.java_awt_image_ImageConsumer_setColorModelMID, 
                         colorModel); 
  
    if (env->ExceptionCheck())
        return;
  
    env->CallVoidMethod (theConsumer, 
                         QtCachedIDs.java_awt_image_ImageConsumer_setHintsMID,
                         java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |
                         java_awt_image_ImageConsumer_COMPLETESCANLINES |
                         java_awt_image_ImageConsumer_SINGLEPASS);
  
    if (env->ExceptionCheck ())
        return;
  
    width = imageRepData->getWidth();
    height = imageRepData->getHeight();
  
	jint *intPixels;
	pixelArray = env->NewIntArray (width);
				
	if (pixelArray == NULL)
	    return;
    
	intPixels = env->GetIntArrayElements ((jintArray)pixelArray,
                                          NULL); 
    
	if (intPixels == NULL)
	    return;
    
	for (y = 0; y < height; y++) {      

	    AWT_QT_LOCK;
          
	    // reading the pixel value at a given x,y location
	    if (imageRepData->isDirtyPixmap()) {
            imageRepData->updateImage();
	    }

	    QImage* image = imageRepData->getImage(); 
      
	    for (x = 0; x < width; x++) { 
            unsigned int pixel = image->pixel(x, y);
            intPixels[x] = (jint)pixel;
	    }
                  
	    AWT_QT_UNLOCK;
      
	    env->ReleaseIntArrayElements ((jintArray)pixelArray, 
                                      intPixels, 
                                      JNI_COMMIT);      
	    env->CallVoidMethod (theConsumer, 
				 QtCachedIDs.java_awt_image_ImageConsumer_setPixels2MID,
				 0, y, width, 1, colorModel,
				 (jintArray)pixelArray, 0, width); 
          
	    if (env->ExceptionCheck())
            return;
	}
    
	env->ReleaseIntArrayElements ((jintArray)pixelArray,
                                  intPixels, JNI_ABORT); 
}
