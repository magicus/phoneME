/*
 * @(#)MWDefaultGraphicsConfiguration.c	1.10 06/10/10
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
#include "java_awt_MWDefaultGraphicsConfiguration.h"
#include "java_awt_image_BufferedImage.h"


JNIEXPORT jobject JNICALL
Java_java_awt_MWDefaultGraphicsConfiguration_createColorModel (JNIEnv *env, jobject this)
{
	jobject colorModel;
	ColorModel *cm = mwawt_hardwareInterface->OpenScreen()->colorModel;

	if (cm->type == COLORMODEL_PALETTE)
	{
		jbyteArray redArray, greenArray, blueArray;
		jsize i;
		jbyte value;

		redArray = (*env)->NewByteArray(env, (jsize)cm->numColors);

		if (redArray == NULL)
			return NULL;

		greenArray = (*env)->NewByteArray(env, (jsize)cm->numColors);

		if (greenArray == NULL)
			return NULL;

		blueArray = (*env)->NewByteArray(env, (jsize)cm->numColors);

		if (blueArray == NULL)
			return NULL;

		for (i = 0; i < cm->numColors; i++)
		{
			MWCOLORVAL color = cm->colors[i];

			value = (jbyte)REDVALUE(color);
			(*env)->SetByteArrayRegion(env, redArray, i, 1, &value);

			value = (jbyte)GREENVALUE(color);
			(*env)->SetByteArrayRegion(env, greenArray, i, 1, &value);

			value = (jbyte)BLUEVALUE(color);
			(*env)->SetByteArrayRegion(env, blueArray, i, 1, &value);
		}

		colorModel = (*env)->NewObject(env, MWCachedIDs.IndexColorModel,
										MWCachedIDs.IndexColorModel_constructor,
										cm->bitsPerPixel,
										(jint)cm->numColors,
										redArray,
										greenArray,
										blueArray);
	}

	else
	{
		colorModel = (*env)->NewObject(env, MWCachedIDs.DirectColorModel,
										MWCachedIDs.DirectColorModel_constructor,
										cm->bitsPerPixel,
										(jint)cm->redMask,
										(jint)cm->greenMask,
										(jint)cm->blueMask);
	}

	return colorModel;
}

JNIEXPORT jint JNICALL
Java_java_awt_MWDefaultGraphicsConfiguration_getMWCompatibleImageType(JNIEnv *env, jobject this)
{
	return java_awt_image_BufferedImage_TYPE_INT_ARGB;
}


