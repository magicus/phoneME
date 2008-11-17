/*
 * @(#)DrawableImageImpl.c	1.11 06/10/10
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

#include "sun_awt_gfX_DrawableImageImpl.h"
#include "Xheaders.h"
#include "common.h"

#include "img_util.h"
#include "img_util_md.h"

JNIACCESS(drawableImageImpl);

JNIEXPORT void JNICALL
Java_sun_awt_gfX_DrawableImageImpl_initJNI(JNIEnv *env,
					   jclass cl)
{
    DEFINECLASS(drawableImageImpl, cl);
    DEFINEFIELD(drawableImageImpl, pData, "I");

    if (MUSTLOAD(sun_awt_image_Image)) {
	FINDCLASS(sun_awt_image_Image, "sun/awt/image/Image");
	DEFINEMETHOD(sun_awt_image_Image, getImageRep,
		     "()Lsun/awt/image/ImageRepresentation;");
    }
}

Pixmap
getDrawablePixmap(JNIEnv *env, jobject obj) {
    IRData *ird;
    jobject imageRep;

    imageRep = CALLMETHOD(Object, obj, sun_awt_image_Image, getImageRep);

    ird = image_getIRData(env, imageRep, NULL, 0);
    return ird->pixmap;    
}

/*
 * Class:     sun_awt_gfX_DrawableImageImpl
 * Method:    init
 * Signature: (Lsun/awt/image/ImageRepresentation;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_DrawableImageImpl_init(JNIEnv *env,
					jobject obj,
					jobject imageRep)
{
}

/*
 * Class:     sun_awt_gfX_DrawableImageImpl
 * Method:    releaseNativeData
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_DrawableImageImpl_releaseNativeData(JNIEnv *env,
						     jobject obj)
{
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_DrawableImageImpl_lock0(JNIEnv *env,
					 jobject obj,
					 jobject lock)
{
    (*env)->MonitorEnter(env, lock);
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_DrawableImageImpl_unlock0(JNIEnv *env,
					   jobject obj,
					   jobject lock)
{
    (*env)->MonitorExit(env, lock);
}
