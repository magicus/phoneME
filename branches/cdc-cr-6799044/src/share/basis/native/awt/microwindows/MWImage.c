/*
 * @(#)MWImage.c	1.26 06/10/10
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
#include <assert.h>

#include "awt.h"
#include "java_awt_MWImage.h"


JNIEXPORT void JNICALL
Java_java_awt_MWImage_initIDs (JNIEnv * env, jclass cls)
{
	GET_FIELD_ID (MWImage_widthFID, "width", "I");
	GET_FIELD_ID (MWImage_heightFID, "height", "I");

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


#define RGBTOBGR(c) ((c & 0x0000FF00) | ((c & 0x00FF0000) >> 16) | ((c & 0x000000FF) << 16))
#define BGRTORGB(c) RGBTOBGR(c)

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pSetColorModelBytePixels (JNIEnv * env,
												jclass cls,
												jint _graphicsEngine,
												jint x,
												jint y,
												jint w,
												jint h,
												jobject colorModel,
												jbyteArray pixels, jint offset, jint scansize)
{
	unsigned long rgb;
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned char pixel;
	unsigned char lastPixel;
	jint index = offset;
	jbyte *pixelArray = NULL;
	GraphicsEngine *graphicsEngine;

    if(_graphicsEngine==0) return;

	pixelArray = (*env)->GetByteArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		return;

	/* Clone graphics so we can switch to SRC mode without affecting the original. */

    graphicsEngine = ((GraphicsEngine *)_graphicsEngine)->Clone((GraphicsEngine *)_graphicsEngine);
    graphicsEngine->SetComposite(graphicsEngine, COMPOSITE_SRC, 255);

	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned char) pixelArray[index + m];

			if (lastPixel != pixel)
			{
				rgb = (unsigned long)
					(*env)->CallIntMethod (env, colorModel, MWCachedIDs.java_awt_image_ColorModel_getRGBMID,
										   (jint) pixel);

				if ((*env)->ExceptionCheck (env))
					goto finish;

				graphicsEngine->SetForegroundColor(graphicsEngine, rgb);

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;
			}

			graphicsEngine->DrawPixel (graphicsEngine, x + m, y + n);
		}

		index += scansize;
	}

  finish:

	(*env)->ReleaseByteArrayElements (env, pixels, pixelArray, JNI_ABORT);
	graphicsEngine->Destroy(graphicsEngine);
}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pSetColorModelIntPixels (JNIEnv * env,
											   jclass cls,
											   jint _graphicsEngine,
											   jint x,
											   jint y,
											   jint w,
											   jint h,
											   jobject colorModel,
											   jintArray pixels, jint offset, jint scansize)
{
	unsigned long rgb;
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned long pixel;
	unsigned long lastPixel;
	jint index = offset;
	jint *pixelArray = NULL;
	GraphicsEngine *graphicsEngine;

    if(_graphicsEngine==0) return;

	pixelArray = (*env)->GetIntArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		return;

	/* Clone graphics so we can switch to SRC mode without affecting the original. */

    graphicsEngine = ((GraphicsEngine *)_graphicsEngine)->Clone((GraphicsEngine *)_graphicsEngine);
    graphicsEngine->SetComposite(graphicsEngine, COMPOSITE_SRC, 255);

	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned long) pixelArray[index + m];

			if (lastPixel != pixel)
			{
				rgb = (unsigned long)
					(*env)->CallIntMethod (env, colorModel, MWCachedIDs.java_awt_image_ColorModel_getRGBMID,
										   pixel);

				if ((*env)->ExceptionCheck (env))
					goto finish;

				graphicsEngine->SetForegroundColor(graphicsEngine, rgb);

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;
			}

			graphicsEngine->DrawPixel (graphicsEngine, x + m, y + n);
		}

		index += scansize;
	}

  finish:

	(*env)->ReleaseIntArrayElements (env, pixels, pixelArray, JNI_ABORT);
	graphicsEngine->Destroy(graphicsEngine);
}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pSetIndexColorModelBytePixels (JNIEnv * env,
													 jclass cls,
													 jint _graphicsEngine,
													 jint x,
													 jint y,
													 jint w,
													 jint h,
													 jobject colorModel,
													 jbyteArray pixels, jint offset, jint scansize)
{
	unsigned long rgb;
	jintArray rgbs;
	jint *rgbArray;				/* Array of RGB values retrieved from IndexColorModel. */
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned char pixel;
	unsigned char lastPixel;
	jint index = offset;
	jbyte *pixelArray = NULL;
	GraphicsEngine *graphicsEngine;

    if(_graphicsEngine==0) return;

	rgbs = (*env)->GetObjectField (env, colorModel, MWCachedIDs.java_awt_image_IndexColorModel_rgbFID);
	rgbArray = (*env)->GetIntArrayElements (env, rgbs, NULL);

	if (rgbArray == NULL)
		return;

	pixelArray = (*env)->GetByteArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		goto finish;

	/* Clone graphics so we can switch to SRC mode without affecting the original. */

    graphicsEngine = ((GraphicsEngine *)_graphicsEngine)->Clone((GraphicsEngine *)_graphicsEngine);
    graphicsEngine->SetComposite(graphicsEngine, COMPOSITE_SRC, 255);

	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned char) pixelArray[index + m];

			/* Get and store red, green, blue values. */

			if (pixel != lastPixel)
			{
				rgb = (unsigned long) rgbArray[pixel];
				graphicsEngine->SetForegroundColor(graphicsEngine, rgb);

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;
			}

			graphicsEngine->DrawPixel (graphicsEngine, x + m, y + n);
		}

		index += scansize;
	}

  finish:

	if (pixelArray != NULL)
		(*env)->ReleaseByteArrayElements (env, pixels, pixelArray, JNI_ABORT);

	(*env)->ReleaseIntArrayElements (env, rgbs, rgbArray, JNI_ABORT);
	graphicsEngine->Destroy(graphicsEngine);
}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pSetIndexColorModelIntPixels (JNIEnv * env,
													jclass cls,
													jint _graphicsEngine,
													jint x,
													jint y,
													jint w,
													jint h,
													jobject colorModel,
													jintArray pixels, jint offset, jint scansize)
{
	unsigned long rgb;
	jintArray rgbs;
	jint *rgbArray;				/* Array of RGB values retrieved from IndexColorModel. */
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned long pixel;
	unsigned long lastPixel;
	jint *pixelArray = NULL;
	jint index = offset;
	GraphicsEngine *graphicsEngine;

        if(_graphicsEngine==0) return;

	rgbs = (*env)->GetObjectField (env, colorModel, MWCachedIDs.java_awt_image_IndexColorModel_rgbFID);
	rgbArray = (*env)->GetIntArrayElements (env, rgbs, NULL);

	if (rgbArray == NULL)
		return;

	pixelArray = (*env)->GetIntArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		goto finish;

	/* Clone graphics so we can switch to SRC mode without affecting the original. */

    graphicsEngine = ((GraphicsEngine *)_graphicsEngine)->Clone((GraphicsEngine *)_graphicsEngine);
    graphicsEngine->SetComposite(graphicsEngine, COMPOSITE_SRC, 255);

	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

	/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned long) pixelArray[index + m];

			/* Get and store red, green, blue values. */

			if (pixel != lastPixel)
			{
				rgb = (unsigned long) rgbArray[pixel];

				graphicsEngine->SetForegroundColor(graphicsEngine, rgb);

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;
			}

			graphicsEngine->DrawPixel (graphicsEngine, x + m, y + n);
		}

		index += scansize;
	}

  finish:

	if (pixelArray != NULL)
		(*env)->ReleaseIntArrayElements (env, pixels, pixelArray, JNI_ABORT);

	(*env)->ReleaseIntArrayElements (env, rgbs, rgbArray, JNI_ABORT);
	graphicsEngine->Destroy(graphicsEngine);
}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pSetDirectColorModelPixels (JNIEnv * env,
												  jclass cls,
												  jint _graphicsEngine,
												  jint x,
												  jint y,
												  jint w,
												  jint h,
												  jobject colorModel,
												  jintArray pixels, jint offset, jint scansize)
{
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned long pixel;
	unsigned long lastPixel;
	jint *pixelArray = NULL;
	unsigned long red_mask, red_offset, red_scale;
	unsigned long green_mask, green_offset, green_scale;
	unsigned long blue_mask, blue_offset, blue_scale;
	unsigned long alpha_mask, alpha_offset, alpha_scale;
	unsigned long alpha, red, green, blue;
	jint index = offset;
	GraphicsEngine *graphicsEngine;

        if(_graphicsEngine==0) return;

	pixelArray = (*env)->GetIntArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		return;

	/* Clone graphics so we can switch to SRC mode without affecting the original. */

    graphicsEngine = ((GraphicsEngine *)_graphicsEngine)->Clone((GraphicsEngine *)_graphicsEngine);
    graphicsEngine->SetComposite(graphicsEngine, COMPOSITE_SRC, 255);

	red_mask =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_red_maskFID);
	red_offset =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_red_offsetFID);
	red_scale =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_red_scaleFID);

	green_mask =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_green_maskFID);
	green_offset =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_green_offsetFID);
	green_scale =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_green_scaleFID);

	blue_mask =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_blue_maskFID);
	blue_offset =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_blue_offsetFID);
	blue_scale =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_blue_scaleFID);

	alpha_mask =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_alpha_maskFID);
	alpha_offset =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_alpha_offsetFID);
	alpha_scale =
		(unsigned long) (*env)->GetIntField (env, colorModel,
											 MWCachedIDs.java_awt_image_DirectColorModel_alpha_scaleFID);

	/* Make sure first time round the loop we actually get the Microwindows pixel value. */

	lastPixel = ~(pixelArray[index]);

	/* For each pixel in the supplied pixel array calculate its rgb value from the colorModel. */

	for (n = 0; n < h; n++)
	{
		for (m = 0; m < w; m++)
		{
			/* Get the pixel at m, n. */

			pixel = (unsigned long) pixelArray[index + m];

			/* If pixel value is not the same as the last one then get red, green, blue
			   and set the foreground for drawing the point in the image. */

			if (pixel != lastPixel)
			{
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

				else
				{
					alpha = ((pixel & alpha_mask) >> alpha_offset);

					if (alpha_scale != 0)
						alpha = alpha * 255 / alpha_scale;
				}

				graphicsEngine->SetForegroundColor(graphicsEngine, MWCOLORARGB(alpha, red, green, blue));

				/* Remember last pixel value for optimisation purposes. */

				lastPixel = pixel;
			}

			/* Use DrawPixel in the GraphicsEngine * structure so as to not change the foreground color and to
	   		   make sure the pixel is actually drawn (as GdPoint looks at the clip mask which should
	   		   not be used when setting pixels). */

			graphicsEngine->DrawPixel (graphicsEngine, x + m, y + n);
		}

		index += scansize;
	}

	if (pixelArray != NULL)
		(*env)->ReleaseIntArrayElements (env, pixels, pixelArray, JNI_ABORT);

	graphicsEngine->Destroy(graphicsEngine);
}

JNIEXPORT jint JNICALL
Java_java_awt_MWImage_pGetRGB(JNIEnv *env, jclass cls, jint _graphicsEngine, jint x, jint y)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
    return graphicsEngine->colorModel->ConvertPixelToColor(graphicsEngine->ReadPixel(graphicsEngine, x, y));
}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pGetRGBArray(JNIEnv *env, jclass cls, jint _graphicsEngine,
									jint startX, jint startY, jint width, jint height, jintArray pixelArray,
									int offset, int scansize)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	ColorModel *cm = graphicsEngine->colorModel;
	int x, y;
    jint *pixelArrayElements;
    
    pixelArrayElements = (*env)->GetIntArrayElements(env, pixelArray, NULL);
    
    if (pixelArrayElements == NULL)
    	return;
    	
    for (y = startY; y < startY + height; y++)
    {
    	for (x = startX; x < startX + width; x++)
    	{
    		pixelArrayElements[offset + (y-startY)*scansize + (x-startX)] = cm->ConvertPixelToColor(graphicsEngine->ReadPixel(graphicsEngine, x, y));
    	}
    }
    
    (*env)->ReleaseIntArrayElements(env, pixelArray, pixelArrayElements, 0);
}


JNIEXPORT void JNICALL
Java_java_awt_MWImage_pSetRGB(JNIEnv *env, jclass cls, jint _graphicsEngine, jint x, jint y, jint rgb) {
        GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

        graphicsEngine->SetForegroundColor(graphicsEngine, rgb);
        graphicsEngine->DrawPixel(graphicsEngine, x, y);
}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pDrawImage (JNIEnv * env, jclass cls, jint _graphicsEngine, jint _imageGraphicsEngine, jint x, jint y, jobject bg)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	GraphicsEngine *imageGraphicsEngine = (GraphicsEngine *)_imageGraphicsEngine;

	if (bg != NULL)
	{
		/* Clone graphicsEngine so we don't affect foreground color. */

		GraphicsEngine *cloned = graphicsEngine->Clone(graphicsEngine);
		jint rgb;

		if (cloned == NULL)
		{
			(*env)->ThrowNew(env, MWCachedIDs.OutOfMemoryError, NULL);
			return;
		}

		rgb = (*env)->GetIntField(env, bg, MWCachedIDs.Color_valueFID);

		cloned->SetForegroundColor(cloned, rgb);
		cloned->FillRect(cloned, x, y, imageGraphicsEngine->width, imageGraphicsEngine->height);
		cloned->Destroy(cloned);
	}

	graphicsEngine->Blit (graphicsEngine, x, y, imageGraphicsEngine->width, imageGraphicsEngine->height, imageGraphicsEngine, 0, 0);

}

JNIEXPORT void JNICALL
Java_java_awt_MWImage_pDrawImageScaled(JNIEnv * env, jclass cls, jint _graphicsEngine, jint dx1, jint dy1, jint dx2, jint dy2, jint _imageGraphicsEngine, jint sx1, jint sy1, jint sx2, jint sy2, jobject bg)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	GraphicsEngine *imageGraphicsEngine = (GraphicsEngine *)_imageGraphicsEngine;

	if (bg != NULL)
	{
		/* Clone graphicsEngine so we don't affect foreground color. */

		GraphicsEngine *cloned = graphicsEngine->Clone(graphicsEngine);
		jint rgb;
		jint x, y, width ,height;

		if (cloned == NULL)
		{
			(*env)->ThrowNew(env, MWCachedIDs.OutOfMemoryError, NULL);
			return;
		}

		if (dx2 < dx1)
		{
			x = dx2;
			width = dx1 - dx2 + 1;
		}

		else
		{
			x = dx1;
			width = dx2 - dx1 + 1;
		}

		if (dy2 < dy1)
		{
			y = dy2;
			height = dy1 - dy2 + 1;
		}

		else
		{
			y = dy1;
			height = dy2 - dy1 + 1;
		}

		rgb = (*env)->GetIntField(env, bg, MWCachedIDs.Color_valueFID);

		/* Clone graphicsEngine so we don't affect foreground color. */

		cloned->SetForegroundColor(cloned, rgb);
		cloned->FillRect(cloned, x, y, width, height);
		cloned->Destroy(cloned);
	}

	graphicsEngine->StretchBlit(graphicsEngine, dx1, dy1, dx2, dy2, imageGraphicsEngine, sx1, sy1, sx2, sy2);
}




