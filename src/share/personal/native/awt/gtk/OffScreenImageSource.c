/*
 * @(#)OffScreenImageSource.c	1.10 06/10/10
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
#include "ImageRepresentation.h"
#include "java_awt_image_ImageConsumer.h"

JNIEXPORT void JNICALL
Java_sun_awt_image_OffScreenImageSource_sendPixels(JNIEnv * env, jobject this)
{
	jobject imageRep, theConsumer, colorModel;
	jint availInfo;
	ImageRepresentationData *imageRepData;
	jint x, y;
	jobject pixelArray;
	jint width, height, depth;
	
	imageRep = (*env)->GetObjectField(env, this, GCachedIDs.sun_awt_image_OffScreenImageSource_baseIRFID);
	
	if (imageRep == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	availInfo = (*env)->GetIntField (env, imageRep, GCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);
	
	if ((availInfo & IMAGE_SIZE_INFO) != IMAGE_SIZE_INFO)
		return;
	
	imageRepData = awt_gtk_getImageRepresentationData (env, imageRep, NULL);
	
	theConsumer = (*env)->GetObjectField (env, this, GCachedIDs.sun_awt_image_OffScreenImageSource_theConsumerFID);
	
	if (theConsumer == NULL)
		return;
		
	colorModel = awt_gtk_getColorModel (env);
	
	if ((*env)->ExceptionCheck (env))
		return;
		
	(*env)->CallVoidMethod (env, theConsumer, GCachedIDs.java_awt_image_ImageConsumer_setColorModelMID, colorModel);
	
	if ((*env)->ExceptionCheck (env))
		return;
		
	(*env)->CallVoidMethod (env, theConsumer, GCachedIDs.java_awt_image_ImageConsumer_setHintsMID,
				 java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |
			     java_awt_image_ImageConsumer_COMPLETESCANLINES |
			     java_awt_image_ImageConsumer_SINGLEPASS);

	if ((*env)->ExceptionCheck (env))
		return;
		
	width = imageRepData->width;
	height = imageRepData->height;
	depth = gdk_visual_get_system()->depth;
		
	/* Get a line at a time from the pixmap and send it to the consumer. */
	
	if (depth <= 8)
	{
		jbyte *bytePixels;
		pixelArray = (*env)->NewByteArray (env, width);
				
		if (pixelArray == NULL)
			return;
			
		bytePixels = (*env)->GetByteArrayElements (env, pixelArray, NULL);
			
		if (bytePixels == NULL)
			return;
	
		for (y = 0; y < height; y++)
		{
			GdkImage *image;
			
			awt_gtk_threadsEnter();
			
			image = gdk_image_get (imageRepData->pixmap, 0, y, width, 1);
			g_assert(image->depth == depth);
			
			for (x = 0; x < width; x++)
			{
				guint32 pixel = gdk_image_get_pixel (image, x, 0);
				g_assert (pixel <= 255);
				
				bytePixels[x] = (jbyte)pixel;
			}
			
			gdk_image_destroy (image);
			
			awt_gtk_threadsLeave();
			
			(*env)->ReleaseByteArrayElements (env, pixelArray, bytePixels, JNI_COMMIT);
			
			(*env)->CallVoidMethod (env, theConsumer, GCachedIDs.java_awt_image_ImageConsumer_setPixelsMID,
									0, y, width, 1, colorModel, pixelArray, 0, width);
									
			if ((*env)->ExceptionCheck (env))
				return;
		}
		
		(*env)->ReleaseByteArrayElements (env, pixelArray, bytePixels, JNI_ABORT);
	}
	
	else
	{
		jint *intPixels;
		pixelArray = (*env)->NewIntArray (env, width);
				
		if (pixelArray == NULL)
			return;
			
		intPixels = (*env)->GetIntArrayElements (env, pixelArray, NULL);
			
		if (intPixels == NULL)
			return;
	
		for (y = 0; y < height; y++)
		{
			GdkImage *image;
			
			awt_gtk_threadsEnter();
			
			image = gdk_image_get (imageRepData->pixmap, 0, y, width, 1);
			g_assert(image->depth == depth);
			
			for (x = 0; x < width; x++)
			{
				guint32 pixel = gdk_image_get_pixel (image, x, 0);
				
				intPixels[x] = (jint)pixel;
			}
			
			gdk_image_destroy (image);
			
			awt_gtk_threadsLeave();
			
			(*env)->ReleaseIntArrayElements (env, pixelArray, intPixels, JNI_COMMIT);
			
			(*env)->CallVoidMethod (env, theConsumer, GCachedIDs.java_awt_image_ImageConsumer_setPixels2MID,
									0, y, width, 1, colorModel, pixelArray, 0, width);
									
			if ((*env)->ExceptionCheck (env))
				return;
		}
		
		(*env)->ReleaseIntArrayElements (env, pixelArray, intPixels, JNI_ABORT);
	}
}
