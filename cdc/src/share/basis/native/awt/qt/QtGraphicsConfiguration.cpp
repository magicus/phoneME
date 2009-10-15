/*
 * @(#)QtGraphicsConfiguration.cpp	1.9 06/10/25
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

#include "awt.h"
#include "java_awt_QtGraphicsConfiguration.h"
#include "java_awt_image_BufferedImage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "QtBackEnd.h"
#include "QtApplication.h"

JNIEXPORT jint JNICALL
Java_java_awt_QtGraphicsConfiguration_createCompatibleImageType (JNIEnv *env, jobject self , jint width, jint height, jboolean fresh)
{
    int i;
 
    AWT_QT_LOCK;
    for(i=NumImageDescriptors-1; i>0; i--)
        if(QtImageDescPool[i].count == 0) break;
    
    if(i==0) {
#ifndef QT_AWT_STATICPOOL
        if(fresh == JNI_TRUE) {
            QtImageDescPool = (QtImageDesc *)realloc(QtImageDescPool, (NumImageDescriptors + NUM_IMAGE_DESCRIPTORS) * sizeof(QtImageDesc));
			
            if(QtGraphDescPool != NULL) {
                memset(&QtImageDescPool[NumImageDescriptors], '\0', NUM_IMAGE_DESCRIPTORS * sizeof(QtImageDesc));
                NumImageDescriptors += NUM_IMAGE_DESCRIPTORS;
                i = NumImageDescriptors-1;
            } else {					
                env->ThrowNew(QtCachedIDs.OutOfMemoryError, "Dynamic Memory exhausted");
                AWT_QT_UNLOCK;
                return -1;
            }
        } else {
            env->ThrowNew(QtCachedIDs.OutOfMemoryError, "Image Descriptors Pool exhausted");
            AWT_QT_UNLOCK;
            return -1;
        }
#else			
			env->ThrowNew(QtCachedIDs.OutOfMemoryError, "Image Descriptors Pool exhausted");
            AWT_QT_UNLOCK;
			return -1;
#endif
    }

    QtImageDescPool[i].qpd = new QPixmap(width, height);
    if (QtImageDescPool[i].qpd == NULL) {
        memset(&QtImageDescPool[i], '\0', sizeof(QtImageDesc));
        env->ThrowNew (QtCachedIDs.OutOfMemoryError, "Can't allocate memory for pixmap data");
        AWT_QT_UNLOCK;
        return -1;
    }
				
    QtImageDescPool[i].loadBuffer = new QImage(width, height, 32);
    if (QtImageDescPool[i].loadBuffer == NULL) {
        if ( QtImageDescPool[i].qpd != NULL ) delete QtImageDescPool[i].qpd;
        memset(&QtImageDescPool[i], '\0', sizeof(QtImageDesc));
        env->ThrowNew (QtCachedIDs.OutOfMemoryError, "Can't allocate memory for image data");
        AWT_QT_UNLOCK;
        return -1;
    }
    QtImageDescPool[i].loadBuffer->setAlphaBuffer(TRUE);

#ifdef QT_AWT_1BIT_ALPHA
    { // ensures that the QPainter destructor is called with the lock held
        QBitmap bm(width, height);

        QPainter qp(&bm);
        qp.fillRect(0, 0, width, height, Qt::color1);
        ((QPixmap *)QtImageDescPool[i].qpd)->setMask(bm);
    }
		
    QtImageDescPool[i].mask = ((QBitmap *)QtImageDescPool[i].qpd)->mask();

    if (QtImageDescPool[i].mask == NULL) {
        delete QtImageDescPool[i].qpd;
        delete QtImageDescPool[i].loadBuffer;
        memset(&QtImageDescPool[i], '\0', sizeof(QtImageDesc));
        env->ThrowNew (QtCachedIDs.OutOfMemoryError, "Can't allocate memory for mask data");
        AWT_QT_UNLOCK;
        return -1;
    }
#endif /* QT_AWT_1BIT_ALPHA */
				
    QtImageDescPool[i].width = width;
    QtImageDescPool[i].height = height;
    QtImageDescPool[i].count = 1;
    AWT_QT_UNLOCK;

    return (jint)i;
}

JNIEXPORT jint JNICALL
Java_java_awt_QtGraphicsConfiguration_pClonePSD(JNIEnv *env, jobject self, jint qtImageDesc)
{
    AWT_QT_LOCK;
	QtImageDescPool[qtImageDesc].count++;
    AWT_QT_UNLOCK;
	return qtImageDesc;
}

JNIEXPORT jint JNICALL
Java_java_awt_QtGraphicsConfiguration_pDisposePSD(JNIEnv *env, jobject self, jint qtImageDesc)
{
	jint r = qtImageDesc;
    AWT_QT_LOCK;
	QtImageDescPool[qtImageDesc].count--;

	if(QtImageDescPool[qtImageDesc].count <= 0) {

        if ( QtImageDescPool[qtImageDesc].qpd != NULL )
            delete QtImageDescPool[qtImageDesc].qpd;

		if(QtImageDescPool[qtImageDesc].loadBuffer!=NULL)
			delete QtImageDescPool[qtImageDesc].loadBuffer;
		
		memset(&QtImageDescPool[qtImageDesc], '\0', sizeof(QtImageDesc));
		r = -1;
	}
    AWT_QT_UNLOCK;
	
	return r;
}

JNIEXPORT jobject JNICALL
Java_java_awt_QtGraphicsConfiguration_createBufferedImageObject (JNIEnv *env, jclass cls, jobject qtimage, int qtImageDesc)
{
    AWT_QT_LOCK;
	if(QtImageDescPool[qtImageDesc].loadBuffer!=NULL)
		delete QtImageDescPool[qtImageDesc].loadBuffer;
	
	QtImageDescPool[qtImageDesc].loadBuffer = NULL;
    AWT_QT_UNLOCK;
	
	return env->NewObject(QtCachedIDs.java_awt_image_BufferedImage, QtCachedIDs.java_awt_image_BufferedImage_constructor, qtimage);

}
