/*
 * @(#)MWGraphicsConfiguration.c	1.12 06/10/10
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
#include "java_awt_MWGraphicsConfiguration.h"
#include "java_awt_image_BufferedImage.h"
#include <stdio.h>
#include <stdlib.h>


JNIEXPORT jint JNICALL
Java_java_awt_MWGraphicsConfiguration_createCompatibleImageType (JNIEnv *env, jobject this , jint width, jint height, jint type)
{
	GraphicsEngine *graphicsEngine = mwawt_hardwareInterface->CreateOffscreenEngine (width, height);

	if (graphicsEngine == NULL)
	{
		(*env)->ThrowNew (env, MWCachedIDs.OutOfMemoryError, NULL);
		return 0;
	}

	return (jint)graphicsEngine;
}




JNIEXPORT jobject JNICALL
Java_java_awt_MWGraphicsConfiguration_createBufferedImageObject (JNIEnv *env, jobject this, jobject mwimage)
{
	return (*env)->NewObject(env, MWCachedIDs.java_awt_image_BufferedImage, MWCachedIDs.java_awt_image_BufferedImage_constructor, mwimage);

}

