/*
 * @(#)GdkImageRepresentation.c	1.34 06/10/10
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

#include <gtk/gtk.h>
#include <math.h>

#include "jni.h"
#include "sun_awt_gtk_GdkImageRepresentation.h"
#include "java_awt_image_ImageObserver.h"
#include "ImageRepresentation.h"
#include "GdkGraphics.h"

#define MIN(x,y) ((x < y) ? x : y)

/* Macro for DirectColorModel variable assignment 			*/
/* Presupposes the declaration of the corresponding variables 		*/
#define RGB_SETUPFORDIRECTCM(ee_, cm_) \
{											\
    red_mask = (*ee_)->GetIntField (ee_, cm_, 						\
			GCachedIDs.java_awt_image_DirectColorModel_red_maskFID);	\
    red_offset = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_red_offsetFID);	\
    red_scale = (*ee_)->GetIntField (ee_, cm_, 						\
			GCachedIDs.java_awt_image_DirectColorModel_red_scaleFID);	\
    green_mask = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_green_maskFID);	\
    green_offset = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_green_offsetFID);	\
    green_scale = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_green_scaleFID);	\
    blue_mask = (*ee_)->GetIntField (ee_, cm_, 						\
			GCachedIDs.java_awt_image_DirectColorModel_blue_maskFID);	\
    blue_offset = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_blue_offsetFID);	\
    blue_scale = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_blue_scaleFID);	\
    alpha_mask = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_alpha_maskFID);	\
    alpha_offset = (*ee_)->GetIntField (ee_, cm_, 					\
			GCachedIDs.java_awt_image_DirectColorModel_alpha_offsetFID);	\
    alpha_scale = (*ee_)->GetIntField (ee_, cm_,	 				\
			GCachedIDs.java_awt_image_DirectColorModel_alpha_scaleFID);	\
}


#define GDK_DITHER_TYPE GDK_RGB_DITHER_NORMAL

/* These are used by the direct colour model conversions. */
static jint red_mask, red_offset, red_scale, red_mask_width;
static jint green_mask, green_offset, green_scale, green_mask_width;
static jint blue_mask, blue_offset, blue_scale, blue_mask_width;
static jint alpha_mask, alpha_offset, alpha_scale;

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_initIDs (JNIEnv * env, jclass cls)
{
	cls = (*env)->FindClass (env, "sun/awt/image/ImageRepresentation");

	if (cls == NULL)
		return;

	GET_FIELD_ID (sun_awt_image_ImageRepresentation_pDataFID, "pData", "I");
	GET_FIELD_ID (sun_awt_image_ImageRepresentation_availinfoFID, "availinfo", "I");
	GET_FIELD_ID (sun_awt_image_ImageRepresentation_widthFID, "width", "I");
	GET_FIELD_ID (sun_awt_image_ImageRepresentation_heightFID, "height", "I");
	GET_FIELD_ID (sun_awt_image_ImageRepresentation_hintsFID, "hints", "I");
	GET_FIELD_ID (sun_awt_image_ImageRepresentation_newbitsFID, "newbits", "Ljava/awt/Rectangle;");
	GET_FIELD_ID (sun_awt_image_ImageRepresentation_offscreenFID, "offscreen", "Z");

	cls = (*env)->FindClass (env, "java/awt/image/ColorModel");

	if (cls == NULL)
		return;

	GET_METHOD_ID (java_awt_image_ColorModel_getRGBMID, "getRGB", "(I)I");

	/* This needs to be called before using any of the gdk rgb functions. */

	awt_gtk_threadsEnter ();
	gdk_rgb_init ();

#ifdef CVM_DEBUG
        gdk_rgb_set_verbose(TRUE);
#endif

	awt_gtk_threadsLeave ();
}

/* Gets the data associated with this ImageRepresentaion and ensures the pixmap is created.
   If the pixmap has not yet been created the size of the image
   is queried and the pixmap is created using this size. If the size of the image is not yet known then
   an exception is thrown. When the pixmap is created it will be filled with the background color supplied
   or, if background is NULL, then it will be filled with black.
   Returns the data with a valid pixmap or NULL if, and only if, an exception has been thrown. */

ImageRepresentationData *
awt_gtk_getImageRepresentationData (JNIEnv * env, jobject this, jobject background)
{
	ImageRepresentationData *data;

	data =
		(ImageRepresentationData *) (*env)->GetIntField (env, this,
														 GCachedIDs.
														 sun_awt_image_ImageRepresentation_pDataFID);

	/* If the data hasn' yet been created then create a new one and set it on the object. */

	if (data == NULL)
	{
		if ((*env)->MonitorEnter (env, this) < 0)
			return NULL;

		data =
			(ImageRepresentationData *) (*env)->GetIntField (env, this,
															 GCachedIDs.
															 sun_awt_image_ImageRepresentation_pDataFID);

		if (data == NULL)
		{
			jint availInfo;
			GdkColor gdkbackground;

			data = (ImageRepresentationData *) calloc (1, sizeof (ImageRepresentationData));

			if (data == NULL)
			{
				(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
				(*env)->MonitorExit (env, this);
				return NULL;
			}

			data->mask = NULL;
			data->rgbBuffer = NULL;
			data->numLines = 0;

			/* Check width and height of image is known so we can create the pixmap with these dimensions. */

			availInfo =
				(*env)->GetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

			if ((availInfo & IMAGE_SIZE_INFO) != IMAGE_SIZE_INFO)
			{
				(*env)->ThrowNew (env, GCachedIDs.java_awt_AWTErrorClass,
								  "Cannot create pixmap because image size not known");
				(*env)->MonitorExit (env, this);
				free (data);
				return NULL;
			}

			/* Get the width and height. */

			data->width =
				(*env)->GetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_widthFID);
			data->height =
				(*env)->GetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_heightFID);

			if (data->width > 0 && data->height > 0)
			{
				awt_gtk_threadsEnter ();

				/* Create the pixmap with the required width, height and depth. */

				data->pixmap =
					gdk_pixmap_new (NULL, data->width, data->height, gdk_visual_get_system ()->depth);

				if (data->pixmap == NULL)
				{
					(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
					(*env)->MonitorExit (env, this);
					awt_gtk_threadsLeave ();
					free (data);
					return NULL;
				}

				data->gc = gdk_gc_new (data->pixmap);

				if (data->gc == NULL)
				{
					(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
					(*env)->MonitorExit (env, this);
					awt_gtk_threadsLeave ();
					free (data);
					return NULL;
				}

				/* Fill image with background color initially. */

				if (background != NULL)
					awt_gtk_getColor (env, background, &gdkbackground);

				else
				{
					gdkbackground.red = 0;
					gdkbackground.green = 0;
					gdkbackground.blue = 0;
					gdk_colormap_alloc_color (gdk_colormap_get_system (), &gdkbackground, FALSE, TRUE);
				}

                                gdk_gc_set_background (data->gc, &gdkbackground);
				gdk_gc_set_foreground (data->gc, &gdkbackground);
				gdk_draw_rectangle (data->pixmap, data->gc, TRUE, 0, 0, data->width, data->height);

				awt_gtk_threadsLeave ();
			}

			else				/* Inavlid width and height for image. */
			{
				(*env)->ThrowNew (env, GCachedIDs.java_awt_AWTErrorClass,
								  "Cannot create pixmap as width and height must be > 0");
				(*env)->MonitorExit (env, this);
				free (data);
				return NULL;
			}

			(*env)->SetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_pDataFID,
								 (jint) data);
		}

		if ((*env)->MonitorExit (env, this) < 0)
			return NULL;
	}

	return data;
}

/* This convenience method performs argument checking for the setBytePixels and setIntPixels
   methods and creates a buffer, if necessary, to store the pixel values in before drawing them
   to the pixmap. This code is defined here so as to share as much code as possible between the two
   methods. */

static ImageRepresentationData *
getImageRepresentationDataForSetPixels (JNIEnv * env,
										jobject this,
										jint x,
										jint y,
										jint w,
										jint h,
										jobject colorModel, jobject pixels, jint offset, jint scansize)
{
	ImageRepresentationData *data;
	int numLines;
	jint rgbBufferSize;

	/* Perform some argument checking. */

	if (this == NULL || colorModel == NULL || pixels == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return NULL;
	}

	if (x < 0 || y < 0 || w < 0 || h < 0 || scansize < 0 || offset < 0)
	{
		(*env)->ThrowNew (env, GCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
		return NULL;
	}

	if (w == 0 || h == 0)
		return NULL;

	if (((*env)->GetArrayLength (env, pixels) < (unsigned long) (offset + w)) ||
		((scansize != 0) && ((((*env)->GetArrayLength (env, pixels) - w - offset) / scansize) < (h - 1))))
	{
		(*env)->ThrowNew (env, GCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
		return NULL;
	}

	/* The rgb data has now been created for the supplied pixels. First create a pixbuf
	   representing these pixels and then render them into the offscreen pixmap. */

	data = awt_gtk_getImageRepresentationData (env, this, NULL);

	if (data == NULL)
		return NULL;

	/* Remember how many lines we have processed so far. */

	numLines = y + h;

	if (numLines > data->numLines)
		data->numLines = numLines;

	/* Allocate the RGB buffer used to represent these pixels. These are stored as 3 byte values for each
	   pixel. If one has already been allocated of the appropriate size then we don't need to do anything.
	   Otherwise we allocate it and remember its size so it can hopefully be used for the next call to  a
	   setPixels method. We allocate a buffer large enough to store one row at a time. The setPixels methods
	   should only use one line at a time to reduce memory rewuirements even if provided with more. */

	rgbBufferSize = w * 1 * 3;

	if (data->rgbBuffer == NULL || data->rgbBufferSize < rgbBufferSize)
	{
		if (data->rgbBuffer != NULL)
			free (data->rgbBuffer);

		data->rgbBuffer = (guchar *) malloc (rgbBufferSize);

		if (data->rgbBuffer == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			return NULL;
		}

		data->rgbBufferSize = rgbBufferSize;
	}

	return data;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_offscreenInit (JNIEnv * env, jobject this, jobject background)
{
	ImageRepresentationData *data;

        if (background == NULL) {
            jclass colorClass = (*env)->FindClass(env, "java/awt/Color");
            jfieldID black    = (*env)->GetStaticFieldID(env, colorClass, "black", "Ljava/awt/Color;");
            background = (*env)->GetStaticObjectField(env, colorClass, black);
        }

	(*env)->SetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID,
						 (jint) IMAGE_OFFSCRNINFO);

	data = awt_gtk_getImageRepresentationData (env, this, background);

	if (data == NULL || data->pixmap == NULL)
		return;

	/* As we don't receive pixels for offscreen images we set the numLines to indicate that all lines
	   have been received. */

	data->numLines = data->height;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_disposeImage (JNIEnv * env, jobject this)
{
	ImageRepresentationData *data =
		(ImageRepresentationData *) (*env)->GetIntField (env, this,
														 GCachedIDs.
														 sun_awt_image_ImageRepresentation_pDataFID);

	if (data != NULL)
	{
		awt_gtk_threadsEnter ();

		if (data->pixmap != NULL)
		{
			gdk_pixmap_unref (data->pixmap);
			data->pixmap = NULL;
		}

		if (data->gc != NULL)
		{
			gdk_gc_unref (data->gc);
			data->gc = NULL;
		}

		if (data->mask != NULL)
		{
			gdk_pixmap_unref ((GdkPixmap *) data->mask);
			data->mask = NULL;
		}

		if (data->maskGC != NULL)
		{
			gdk_gc_unref (data->maskGC);
			data->maskGC = NULL;
		}

		if (data->rgbBuffer != NULL)
		{
			free (data->rgbBuffer);
			data->rgbBuffer = NULL;
		}

		(*env)->SetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_pDataFID, (jint) NULL);
		free (data);

		awt_gtk_threadsLeave ();
	}
}

/* Marks the suplied pixel location as being transparent by setting the mask value
   to 0 at the supplied position. If a mask has not yet been created then it is
   created and filled with 1s first. */

static jboolean
setTransparentPixel (JNIEnv * env, ImageRepresentationData * data, jint x, jint y, gboolean isTransparent)
{
	/* The GC used for drawing to the mask. */

	GdkColor opaque, transparent;

	opaque.pixel = 1;
	transparent.pixel = 0;

	awt_gtk_threadsEnter ();

	if (isTransparent && data->mask == NULL)
	{
		data->mask = (GdkBitmap *) gdk_pixmap_new (NULL, data->width, data->height, 1);

		if (data->mask == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			awt_gtk_threadsLeave ();
			return JNI_FALSE;
		}

		data->maskGC = gdk_gc_new (data->mask);

		if (data->maskGC == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			awt_gtk_threadsLeave ();
			return JNI_FALSE;
		}

		/* Fill mask with 1s */

		gdk_gc_set_foreground (data->maskGC, &opaque);
		gdk_draw_rectangle (data->mask, data->maskGC, TRUE, 0, 0, data->width, data->height);
	}

	if (data->mask != NULL)
	{
		gdk_gc_set_foreground (data->maskGC, isTransparent ? &transparent : &opaque);
		gdk_draw_point (data->mask, data->maskGC, x, y);
	}

	awt_gtk_threadsLeave ();

	return JNI_TRUE;
}

/* As the image is read in and passed through the java image filters this method gets called
   to store the pixels in the native image representation. The GDK pixbuf library is used to
   store the pixels in a pixmap (a server side off screen image). The pixbuf library does the
   necessary scaling, alpha transparency and dithering. Typically, this method is called for each
   line of the image so the whole image is not in memory at once. */

JNIEXPORT jboolean JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_setBytePixels (JNIEnv * env,
													  jobject this,
													  jint x,
													  jint y,
													  jint w,
													  jint h,
													  jobject colorModel,
													  jbyteArray pixels, jint offset, jint scansize)
{
	ImageRepresentationData *data;
	guchar *rgbBuffer = NULL;	/* Stores red, green, blue bytes for each pixel */
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned char pixel;
	jbyte *pixelArray = NULL;
	int index = 0;				/* Index position to insert next rgb values in the rgb_buf */
	unsigned long rgb;
	jint *rgbArray = NULL;
	jintArray rgbs;

	data =
		getImageRepresentationDataForSetPixels (env, this, x, y, w, h, colorModel, pixels, offset, scansize);

	if (data == NULL || data->pixmap == NULL)
		return JNI_FALSE;

	pixelArray = (*env)->GetByteArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		return JNI_FALSE;

	rgbBuffer = data->rgbBuffer;

	/* Optimization to prevent callbacks into java for index color model. */

	if ((*env)->IsInstanceOf (env, colorModel, GCachedIDs.java_awt_image_IndexColorModelClass))
	{
		rgbs = (*env)->GetObjectField (env, colorModel, GCachedIDs.java_awt_image_IndexColorModel_rgbFID);

		if (rgbs == NULL)
			goto finish;

		rgbArray = (*env)->GetIntArrayElements (env, rgbs, NULL);

		if (rgbArray == NULL)
			goto finish;

		/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

		for (n = 0; n < h; n++)
		{
			index = 0;

			for (m = 0; m < w; m++)
			{
				/* Get the pixel at m, n. */

				pixel = (unsigned char) pixelArray[n * scansize + m + offset];
				rgb = (unsigned long) rgbArray[pixel];

				/* Get and store red, green, blue values. */

				rgbBuffer[index++] = (guchar) ((rgb & 0xff0000) >> 16);
				rgbBuffer[index++] = (guchar) ((rgb & 0xff00) >> 8);
				rgbBuffer[index++] = (guchar) (rgb & 0xff);

				/* Check if pixel should be transparent. */

				if (setTransparentPixel (env, data, x + m, y + n, (rgb & 0xff000000) == 0) == JNI_FALSE)
					goto finish;
			}

			awt_gtk_threadsEnter ();

			/* Render the buffer to the offscreen pixmap. */

			if (data->pixmap != NULL)
				gdk_draw_rgb_image (data->pixmap, data->gc, x, y + n, w, 1, GDK_DITHER_TYPE, rgbBuffer,
									0);

			awt_gtk_threadsLeave ();
		}
	}

	/* No optimization possible - call back into Java. */

	else
	{
		for (n = 0; n < h; n++)
		{
			index = 0;

			for (m = 0; m < w; m++)
			{
				/* Get the pixel at m, n. */

				pixel = ((unsigned char) pixelArray[n * scansize + m + offset]);
				rgb =
					(unsigned long) (*env)->CallIntMethod (env, colorModel,
														   GCachedIDs.java_awt_image_ColorModel_getRGBMID,
														   pixel);

				if ((*env)->ExceptionCheck (env))
					goto finish;

				/* Get and store red, green, blue values. */

				rgbBuffer[index++] = (guchar) ((rgb & 0xff0000) >> 16);
				rgbBuffer[index++] = (guchar) ((rgb & 0xff00) >> 8);
				rgbBuffer[index++] = (guchar) (rgb & 0xff);

				/* Check if pixel should be transparent. */

				if (setTransparentPixel (env, data, x + m, y + n, (rgb & 0xff000000) == 0) ==
					JNI_FALSE)
					goto finish;
			}

			awt_gtk_threadsEnter ();

			/* Render the buffer to the offscreen pixmap. */

			if (data->pixmap != NULL)
				gdk_draw_rgb_image (data->pixmap, data->gc, x, y + n, w, 1, GDK_DITHER_TYPE, rgbBuffer,
									0);

			awt_gtk_threadsLeave ();
		}
	}

  finish:						/* Perform tidying up code and return. */

	if (pixelArray != NULL)
		(*env)->ReleaseByteArrayElements (env, pixels, pixelArray, JNI_ABORT);

	if (rgbArray != NULL)
		(*env)->ReleaseIntArrayElements (env, rgbs, rgbArray, JNI_ABORT);

	return JNI_TRUE;
}

/* As the image is read in and passed through the java image filters this method gets called
   to store the pixels in the native image representation. The GDK pixbuf library is used to
   store the pixels in a pixmap (a server side off screen image). The pixbuf library does the
   necessary scaling, alpha transparency and dithering. Typically, this method is called for each
   line of the image so the whole image is not in memory at once. */

JNIEXPORT jboolean JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_setIntPixels (JNIEnv * env,
													 jobject this,
													 jint x,
													 jint y,
													 jint w,
													 jint h,
													 jobject colorModel,
													 jintArray pixels, jint offset, jint scansize)
{
	ImageRepresentationData *data;
	guchar *rgbBuffer = NULL;	/* Stores red, green, blue bytes for each pixel */
	jint m, n;					/* x, y position in pixel array - same variable names as Javadoc comment for setIntPixels. */
	unsigned long pixel;
	unsigned long rgb;
	jint *pixelArray = NULL;
	jint index = 0;				/* Index position to insert next rgb values in the rgb_buf */
	jintArray rgbs;
	jint *rgbArray = NULL;

	data =
		getImageRepresentationDataForSetPixels (env, this, x, y, w, h, colorModel, pixels, offset, scansize);

	if (data == NULL || data->pixmap == NULL)
		return JNI_FALSE;

	pixelArray = (*env)->GetIntArrayElements (env, pixels, NULL);

	if (pixelArray == NULL)
		goto finish;

	rgbBuffer = data->rgbBuffer;

	/* Optimization to prevent callbacks into java for direct color model. */

	if ((*env)->IsInstanceOf (env, colorModel, GCachedIDs.java_awt_image_DirectColorModelClass))
	{
		jint alpha, red, green, blue;

                RGB_SETUPFORDIRECTCM(env, colorModel);

		/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

		for (n = 0; n < h; n++)
		{
			index = 0;

			for (m = 0; m < w; m++)
			{
				/* Get the pixel at m, n. */

				pixel = (unsigned long) pixelArray[n * scansize + m + offset];

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

				/* Get and store red, green, blue values. */

				rgbBuffer[index++] = (guchar) red;
				rgbBuffer[index++] = (guchar) green;
				rgbBuffer[index++] = (guchar) blue;

				/* Check if pixel should be transparent. */

				if (setTransparentPixel (env, data, x + m, y + n, alpha == 0) == JNI_FALSE)
					goto finish;
			}

			awt_gtk_threadsEnter ();

			/* Render the buffer to the offscreen pixmap. */

			if (data->pixmap != NULL)
				gdk_draw_rgb_image (data->pixmap, data->gc, x, y + n, w, 1, GDK_DITHER_TYPE, rgbBuffer,
									0);

			awt_gtk_threadsLeave ();
		}
	}

	/* Optimization to prevent callbacks into java for index color model. */

	else if ((*env)->IsInstanceOf (env, colorModel, GCachedIDs.java_awt_image_IndexColorModelClass))
	{
		rgbs = (*env)->GetObjectField (env, colorModel, GCachedIDs.java_awt_image_IndexColorModel_rgbFID);

		if (rgbs == NULL)
			goto finish;

		rgbArray = (*env)->GetIntArrayElements (env, rgbs, NULL);

		if (rgbArray == NULL)
			goto finish;

		/* For each pixel in the supplied pixel array look up its rgb value from the colorModel. */

		for (n = 0; n < h; n++)
		{
			index = 0;

			for (m = 0; m < w; m++)
			{
				/* Get the pixel at m, n. */

				pixel = (unsigned long) pixelArray[n * scansize + m + offset];
				rgb = (unsigned long) rgbArray[pixel];

				/* Get and store red, green, blue values. */

				rgbBuffer[index++] = (guchar) ((rgb & 0xff0000) >> 16);
				rgbBuffer[index++] = (guchar) ((rgb & 0xff00) >> 8);
				rgbBuffer[index++] = (guchar) (rgb & 0xff);

				/* Check if pixel should be transparent. */

				if (setTransparentPixel (env, data, x + m, y + n, (rgb & 0xff000000) == 0) ==
					JNI_FALSE)
					goto finish;
			}

			awt_gtk_threadsEnter ();

			/* Render the buffer to the offscreen pixmap. */

			if (data->pixmap != NULL)
				gdk_draw_rgb_image (data->pixmap, data->gc, x, y + n, w, 1, GDK_DITHER_TYPE, rgbBuffer,
									0);

			awt_gtk_threadsLeave ();
		}
	}

	/* No optimization possible - call back into Java. */

	else
	{
		for (n = 0; n < h; n++)
		{
			index = 0;

			for (m = 0; m < w; m++)
			{
				/* Get the pixel at m, n. */

				pixel = (unsigned long) pixelArray[n * scansize + m + offset];
				rgb =
					(unsigned long) (*env)->CallIntMethod (env, colorModel,
														   GCachedIDs.java_awt_image_ColorModel_getRGBMID,
														   pixel);

				if ((*env)->ExceptionCheck (env))
					goto finish;

				/* Get and store red, green, blue values. */

				rgbBuffer[index++] = (guchar) ((rgb & 0xff0000) >> 16);
				rgbBuffer[index++] = (guchar) ((rgb & 0xff00) >> 8);
				rgbBuffer[index++] = (guchar) (rgb & 0xff);

				/* Check if pixel should be transparent. */

				if (setTransparentPixel (env, data, x + m, y + n, (rgb & 0xff000000) == 0) ==
					JNI_FALSE)
					goto finish;
			}

			awt_gtk_threadsEnter ();

			/* Render the buffer to the offscreen pixmap. */

			if (data->pixmap != NULL)
				gdk_draw_rgb_image (data->pixmap, data->gc, x, y + n, w, 1, GDK_DITHER_TYPE, rgbBuffer,
									0);

			awt_gtk_threadsLeave ();
		}
	}

  finish:						/* Perform tidying up code and return. */

	if (pixelArray != NULL)
		(*env)->ReleaseIntArrayElements (env, pixels, pixelArray, JNI_ABORT);

	if (rgbArray != NULL)
		(*env)->ReleaseIntArrayElements (env, rgbs, rgbArray, JNI_ABORT);

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_finish (JNIEnv * env, jobject this, jboolean force)
{
	ImageRepresentationData *data =
		(ImageRepresentationData *) (*env)->GetIntField (env, this,
														 GCachedIDs.
														 sun_awt_image_ImageRepresentation_pDataFID);

	if (data != NULL && data->rgbBuffer != NULL)
	{
		free (data->rgbBuffer);
		data->rgbBuffer = NULL;
	}

	return JNI_FALSE;
}

/* Gets the native data for the image representation and the graphics object for drawing the image. Also checks to make sure
   the data is suitable for drawing images and throws exceptions if there is a problem. This function is defined so as to
   share as much code as possible between the two image drawing methods of ImageRepresentation (imageDraw and imageStretch). */

static gboolean
getDataForImageDrawing (JNIEnv * env,
						jobject this,
						jobject g, ImageRepresentationData ** data, GdkGraphicsData ** graphicsData)
{
	jint availInfo;

	if (g == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return FALSE;
	}

	if (!(*env)->IsInstanceOf (env, g, GCachedIDs.GdkGraphicsClass))
	{
		(*env)->ThrowNew (env, GCachedIDs.IllegalArgumentExceptionClass, NULL);
		return FALSE;
	}

	/* Check width and height of image is known. */

	availInfo = (*env)->GetIntField (env, this, GCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID);

	if ((availInfo & IMAGE_SIZE_INFO) != IMAGE_SIZE_INFO)
		return FALSE;


	*graphicsData = (GdkGraphicsData *) (*env)->GetIntField (env, g, GCachedIDs.GdkGraphics_dataFID);

	if (*graphicsData == NULL || (*graphicsData)->drawable == NULL || (*graphicsData)->gc == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return FALSE;
	}

	*data = awt_gtk_getImageRepresentationData (env, this, NULL);

	if (*data == NULL || (*data)->pixmap == NULL)
		return FALSE;

	return TRUE;
}

static void
drawUnscaledImage (ImageRepresentationData * data,
				   GdkGraphicsData * graphicsData,
				   gint xsrc, gint ysrc, gint xdest, gint ydest, gint width, gint height)
{
	GdkGC *gc;

	awt_gtk_threadsEnter ();

	/* If we have a mask then we need to create a new GC with the bitmap set as the clip. */

	if (data->mask != NULL)
	{
		gc = gdk_gc_new (graphicsData->drawable);
		gdk_gc_copy (gc, graphicsData->gc);
		gdk_gc_set_clip_origin (gc, xdest - xsrc, ydest - ysrc);
		gdk_gc_set_clip_mask (gc, data->mask);
	}

	else
		gc = graphicsData->gc;

	gdk_draw_pixmap (graphicsData->drawable, gc, data->pixmap, xsrc, ysrc, xdest, ydest, width, height);

	if (data->mask != NULL)
		gdk_gc_unref (gc);

	awt_gtk_threadsLeave ();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_imageDraw (JNIEnv * env, jobject this,
												  jobject g, jint x, jint y, jobject c)
{
	GdkGraphicsData *graphicsData;
	ImageRepresentationData *data;
	gint xsrc, ysrc, xdest, ydest, width, height;

	if (!getDataForImageDrawing (env, this, g, &data, &graphicsData))
		return;

	x += (*env)->GetIntField (env, g, GCachedIDs.GdkGraphics_originXFID);
	y += (*env)->GetIntField (env, g, GCachedIDs.GdkGraphics_originYFID);

	/* If a clip rectangle has been set then calculate the intersection of the clip rectangle with
	   the image and only draw this portion of the image. */

	if (graphicsData->clipHasBeenSet)
	{
		GdkRectangle imageRect, intersection;

		imageRect.x = (gint16) x;
		imageRect.y = (gint16) y;
		imageRect.width = (guint16) data->width;
		imageRect.height = (guint16) data->height;

		if (gdk_rectangle_intersect (&imageRect, &graphicsData->clipBounds, &intersection))
		{
			xsrc = intersection.x - x;
			ysrc = intersection.y - y;
			xdest = intersection.x;
			ydest = intersection.y;
			width = intersection.width;
			height = intersection.height;
		}

		else
			return;				/* No intersection - nothing to draw. */
	}

	else						/* Draw whole image */
	{
		xsrc = 0;
		ysrc = 0;
		xdest = x;
		ydest = y;
		width = data->width;
		height = data->height;
	}

	/* Fill with background color. */

	if (c != NULL)
	{
		GdkGC *gc;
		GdkColor gdkcolor;

		awt_gtk_threadsEnter ();

		awt_gtk_getColor (env, c, &gdkcolor);
		gc = gdk_gc_new (graphicsData->drawable);
		gdk_gc_set_foreground (gc, &gdkcolor);
		gdk_draw_rectangle (graphicsData->drawable, gc, TRUE, xdest, ydest, width, height);
		gdk_gc_unref (gc);

		awt_gtk_threadsLeave ();
	}

	drawUnscaledImage (data, graphicsData, xsrc, ysrc, xdest, ydest, width, height);
}

/* Scales the sourceImage to the destImage. The widths and heights of the source and destinations, as provided,
   are used to determine the scaling factors. */

static void
scaleImage (GdkImage * sourceImage, GdkImage * destImage, jint sourceWidth, jint sourceHeight, jint destWidth,
			jint destHeight)
{
	int x, y;
	int sourceX, sourceY, oldSourceX = -1;
	int maxSourceX = sourceImage->width - 1, maxSourceY = sourceImage->height - 1;
	int maxDestX = destImage->width - 1, maxDestY = destImage->height - 1;
	int offsetX, offsetY;
	guint32 pixel;

	/* If we are flipping the image then offset from the right and bottom side of the source image. */

	offsetX = ((sourceWidth < 0 && destWidth > 0) || (sourceWidth > 0 && destWidth < 0)) ? maxSourceX : 0;
	offsetY = ((sourceHeight < 0 && destHeight > 0) || (sourceHeight > 0 && destHeight < 0)) ? maxSourceY : 0;

	for (y = 0; y <= maxDestY; y++)
	{
		sourceY = offsetY + (y * sourceHeight) / destHeight;

		if (sourceY > maxSourceY)
			sourceY = maxSourceY;

		for (x = 0; x <= maxDestX; x++)
		{
			/* For this pixel in the destination image determine the corresponding pixel
			   from the source image. */

			sourceX = offsetX + (x * sourceWidth) / destWidth;

			if (sourceX > maxSourceX)
				sourceX = maxSourceX;

			if (sourceX != oldSourceX)
				pixel = gdk_image_get_pixel (sourceImage, sourceX, sourceY);

			gdk_image_put_pixel (destImage, x, y, pixel);

			oldSourceX = sourceX;
		}
	}
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_imageStretch (JNIEnv * env,
													 jobject this,
													 jobject g,
													 jint dx1,
													 jint dy1,
													 jint dx2,
													 jint dy2,
													 jint sx1, jint sy1, jint sx2, jint sy2, jobject c)
{
	GdkGraphicsData *graphicsData;
	ImageRepresentationData *data;
	jint originX, originY;
	jint sourceWidth, sourceHeight, destWidth, destHeight;
	GdkImage *sourceImage = NULL;
	GdkImage *destImage = NULL;
	GdkBitmap *destMaskBitmap = NULL;
	GdkImage *sourceMaskImage;
	GdkImage *destMaskImage = NULL;
	GdkGC *destMaskGC;
	GdkGC *gc = NULL;
	GdkRectangle imageRect, sourceRect, destRect, intersection;

	if (!getDataForImageDrawing (env, this, g, &data, &graphicsData))
		return;

	/* Translate destination area. */

	originX = (*env)->GetIntField (env, g, GCachedIDs.GdkGraphics_originXFID);
	originY = (*env)->GetIntField (env, g, GCachedIDs.GdkGraphics_originYFID);
	dx1 += originX;
	dx2 += originX;
	dy1 += originY;
	dy2 += originY;

	/* Calculate scale factors (negative scales imply a flip). */

	sourceWidth = sx2 - sx1;

	if (sourceWidth >= 0)
		sourceWidth++;

	else
		sourceWidth--;

	destWidth = dx2 - dx1;

	if (destWidth >= 0)
		destWidth++;

	else
		destWidth--;

	sourceHeight = sy2 - sy1;

	if (sourceHeight >= 0)
		sourceHeight++;

	else
		sourceHeight--;

	destHeight = dy2 - dy1;

	if (destHeight >= 0)
		destHeight++;

	else
		destHeight--;

	destRect.x = MIN (dx1, dx2);
	destRect.y = MIN (dy1, dy2);
	destRect.width = abs (destWidth);
	destRect.height = abs (destHeight);

	sourceRect.x = MIN (sx1, sx2);
	sourceRect.y = MIN (sy1, sy2);
	sourceRect.width = abs (sourceWidth);
	sourceRect.height = abs (sourceHeight);

	/* Calculate intersection of destination rectangle with the clip bounds on the graphics
	   if one has been set. */

	if (graphicsData->clipHasBeenSet)
	{
		if (gdk_rectangle_intersect (&destRect, &graphicsData->clipBounds, &intersection))
		{
			gint x1, y1, x2, y2;
			GdkRectangle tempRect;

			destRect = intersection;

			/* Calculate what portion of the source image this represents. The first source coordinate
			   corresponds to the first dest coordiante and the second source coordinate corresponds
			   to the second dest coordinate always (flipping the image if necessary). */

			x1 = sx1 + (gint) (((destRect.x - dx1) * sourceWidth) / destWidth);
			y1 = sy1 + (gint) (((destRect.y - dy1) * sourceHeight) / destHeight);
			x2 = sx1 + (gint) (((destRect.x + destRect.width - 1 - dx1) * sourceWidth) / destWidth);
			y2 = sy1 + (gint) (((destRect.y + destRect.height - 1 - dy1) * sourceHeight) / destHeight);

			tempRect.x = MIN (x1, x2);
			tempRect.y = MIN (y1, y2);
			tempRect.width = abs (x2 - x1) + 1;
			tempRect.height = abs (y2 - y1) + 1;

			if (gdk_rectangle_intersect (&sourceRect, &tempRect, &intersection))
				sourceRect = intersection;

			else
				return;			/* No intersection - nothing to draw. */
		}

		else
			return;				/* No intersection - nothing to draw. */
	}

	/* Make sure source area intersects with the image. */

	imageRect.x = 0;
	imageRect.y = 0;
	imageRect.width = data->width;
	imageRect.height = data->height;

	if (gdk_rectangle_intersect (&sourceRect, &imageRect, &intersection))
		sourceRect = intersection;

	else
		return;					/* No intersection - nothing to draw. */

	if (sourceRect.width == 0 || sourceRect.height == 0 || destRect.width == 0 || destRect.height == 0)
		return;

	/* Fill with background color. */

	if (c != NULL)
	{
		GdkGC *gc;
		GdkColor gdkcolor;

		awt_gtk_threadsEnter ();

		awt_gtk_getColor (env, c, &gdkcolor);
		gc = gdk_gc_new (graphicsData->drawable);
		gdk_gc_set_foreground (gc, &gdkcolor);
		gdk_draw_rectangle (graphicsData->drawable, gc, TRUE, destRect.x, destRect.y, destRect.width,
							destRect.height);
		gdk_gc_unref (gc);

		awt_gtk_threadsLeave ();
	}

	/* Optimization to draw image unscaled if we can. */

	if (sourceWidth == destWidth && sourceHeight == destHeight)
	{
		drawUnscaledImage (data,
						   graphicsData,
						   sourceRect.x,
						   sourceRect.y,
						   destRect.x,
						   destRect.y,
						   MIN (sourceRect.width, destRect.width), MIN (sourceRect.height, destRect.height));
		return;
	}

	awt_gtk_threadsEnter ();

	/* Get portion of source image from server for scaling and create a destination image large
	   enough to hold the result. */

	sourceImage =
		gdk_image_get (data->pixmap, sourceRect.x, sourceRect.y, sourceRect.width, sourceRect.height);
	destImage = gdk_image_new (GDK_IMAGE_FASTEST, gdk_visual_get_system (), destRect.width, destRect.height);

	if (sourceImage == NULL || destImage == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);

		if (sourceImage != NULL)
			gdk_image_destroy (sourceImage);

		goto finish;
	}

	scaleImage (sourceImage, destImage, sourceWidth, sourceHeight, destWidth, destHeight);
	gdk_image_destroy (sourceImage);

	/* If there is a mask associated with this image then we need to scale the mask as well. */

	if (data->mask != NULL)
	{
		destMaskBitmap = (GdkBitmap *) gdk_pixmap_new (NULL, destRect.width, destRect.height, 1);

		if (destMaskBitmap == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			goto finish;
		}

		destMaskGC = gdk_gc_new (destMaskBitmap);
		destMaskImage = gdk_image_get (destMaskBitmap, 0, 0, destRect.width, destRect.height);
		gc = gdk_gc_new (graphicsData->drawable);
		gdk_gc_copy (gc, graphicsData->gc);
		sourceMaskImage =
			gdk_image_get (data->mask, sourceRect.x, sourceRect.y, sourceRect.width, sourceRect.height);

		if (destMaskGC == NULL || destMaskImage == NULL || gc == NULL || sourceMaskImage == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			goto finish;
		}

		scaleImage (sourceMaskImage, destMaskImage, sourceWidth, sourceHeight, destWidth, destHeight);
		gdk_image_destroy (sourceMaskImage);
		gdk_draw_image (destMaskBitmap, destMaskGC, destMaskImage, 0, 0, 0, 0, destRect.width,
						destRect.height);
		gdk_gc_unref (destMaskGC);
		gdk_gc_set_clip_mask (gc, destMaskBitmap);
		gdk_gc_set_clip_origin (gc, destRect.x, destRect.y);
	}

	else
		gc = graphicsData->gc;

	gdk_draw_image (graphicsData->drawable, gc, destImage, 0, 0, destRect.x, destRect.y, destRect.width,
					destRect.height);

  finish:

	if (destImage != NULL)
		gdk_image_destroy (destImage);

	if (destMaskImage != NULL)
		gdk_image_destroy (destMaskImage);

	if (data->mask != NULL && gc != NULL)
		gdk_gc_unref (gc);

	awt_gtk_threadsLeave ();
}


static int maskWidth(unsigned int mask)
{
    int i = 0;

    while((mask&0x01) == 0x00)
        mask >>= 1;

    while((mask&0x01) == 0x01)
    {
        mask >>= 1;
        i++;
    }

    return i;
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_getRGB (JNIEnv *env, jobject this, 
						jobject colorModel, jint x, jint y)
{
	int width, height;
	jint pixel;
	jint alpha, red, green, blue;
	GdkColor c;
	GdkImage *image;
	GdkVisual *visual;
	GdkColormap *colormap;
	ImageRepresentationData *irdata = 
			awt_gtk_getImageRepresentationData (env, this, NULL);
	width = irdata->width;
	height = irdata->height;

	if(x<0 || y<0 || x>=width || y>=height) {
	    (*env)->ThrowNew (env, GCachedIDs.ArrayIndexOutOfBoundsExceptionClass, 
				"Value out of bounds");
	    return 0;
	}

	if(irdata ==  NULL) {
	    (*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
	    return 0;
	}

	awt_gtk_threadsEnter();

	colormap = gdk_rgb_get_cmap();
  	visual = gdk_rgb_get_visual();

	if((image = gdk_image_get(irdata->pixmap, x, y, 1, 1)) ==  NULL) {
            awt_gtk_threadsLeave();
	    (*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
	    return 0;
	}

	pixel = gdk_image_get_pixel(image, 0, 0);
	gdk_image_destroy(image);

        alpha = -1;
	if(irdata->mask != NULL) {
            if ((image = gdk_image_get(irdata->mask, x, y, 1, 1)) ==  NULL) {
                awt_gtk_threadsLeave();
                (*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
                return 0;
            }

	    alpha = gdk_image_get_pixel(image, 0, 0);
	    gdk_image_destroy(image);
	}

	//Is the system using an indexed or direct colormodel?
	if (colormap->colors != NULL) {	
	    //Indexed.
	    c = colormap->colors[pixel];
	    red = c.red >> 8;
	    green = c.green >> 8;
	    blue  = c.blue >> 8;
            alpha = (alpha > -1 && colormap->colors[alpha].pixel == 0) ? 0 : 255;
	}
	else {
	    //Direct.
            red_mask_width = 8 - maskWidth(visual->red_mask);
            green_mask_width = 8 - maskWidth(visual->green_mask);
            blue_mask_width = 8 - maskWidth(visual->blue_mask);

	    red = (pixel & visual->red_mask) >> visual->red_shift;
	    green = (pixel & visual->green_mask) >> visual->green_shift;	
	    blue = (pixel & visual->blue_mask) >> visual->blue_shift;

            red <<= red_mask_width;
            green <<= green_mask_width;
            blue <<= blue_mask_width;


	    alpha = (alpha == 0 ? 0 : 255);
	}

	//Assume that the Java defaultRGB colormodel is a DirectColorModel

        RGB_SETUPFORDIRECTCM(env, colorModel);

	if(red_scale != 0) {
	    red = red * red_scale / 255;	    
	}
	if(green_scale != 0) {
	    green = green * green_scale / 255;	    
	}
	if(blue_scale != 0) {
	    blue = blue * blue_scale / 255;	    
	}
	red = (red & 0xff) << red_offset;
	green = (green & 0xff) << green_offset;
	blue = (blue & 0xff) << blue_offset;
	alpha = (alpha & 0xff) << alpha_offset;
	pixel = (red | green | blue | alpha);

	awt_gtk_threadsLeave();

	return pixel;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkImageRepresentation_getRGBs (JNIEnv *env, jobject this, jobject colorModel, 
						 jint startX, jint startY, jint w, jint h, 
						 jintArray rgbArray, jint offset, jint scansize)
{
	int x, y;
	int width, height;
	jint pixel = 0;
	jint alpha, red, green, blue;
	GdkImage *image, *alpha_image;
	GdkVisual *visual;
	GdkColormap *colormap;
	GdkColor c;
    	jint *pixelArrayElements;
	ImageRepresentationData *irdata = awt_gtk_getImageRepresentationData(env, this, NULL);
	width = irdata->width;
	height = irdata->height;

	if(startX<0 || startY<0 || (startX+w)>irdata->width || (startY+h)>irdata->height) {
	    (*env)->ThrowNew (env, GCachedIDs.ArrayIndexOutOfBoundsExceptionClass, "Pixel out of range");
	    return;
	}

	if(irdata == NULL) {
	    (*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
	    return;
	}

	awt_gtk_threadsEnter();

	colormap = gdk_rgb_get_cmap();
  	visual = gdk_rgb_get_visual();

	if((image = gdk_image_get(irdata->pixmap, 0, 0, width, height)) ==  NULL) {
            awt_gtk_threadsLeave();
	    (*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
            return;
        }

	alpha_image = NULL;
	if(irdata->mask != NULL) 
            if((alpha_image = gdk_image_get(irdata->mask, 0, 0, width, height)) ==  NULL) {
        	gdk_image_destroy(image);
                awt_gtk_threadsLeave();
                (*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
                return;
            }

    	if ((pixelArrayElements = (*env)->GetIntArrayElements(env, rgbArray, NULL)) == NULL)
        {
            gdk_image_destroy(image);
            gdk_image_destroy(alpha_image);
            awt_gtk_threadsLeave();
	    (*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
    	    return;
        }

        // 4717821 - check if Direct color model
        // Do not set the mask_widht if Indexed color model
        if (colormap->colors == NULL) {
            red_mask_width = 8 - maskWidth(visual->red_mask);
            green_mask_width = 8 - maskWidth(visual->green_mask);
            blue_mask_width = 8 - maskWidth(visual->blue_mask);
        }

  	for (y = startY; y < startY + h; y++) {
    	    for (x = startX; x < startX + w; x++) {
	        pixel = gdk_image_get_pixel(image, x, y);
		alpha = alpha_image != NULL ? gdk_image_get_pixel(alpha_image, x, y) : -1;

	        //Is the system using an indexed or direct colormodel?
		if(colormap->colors != NULL) {
		    //Indexed.
	       	    c = colormap->colors[pixel];
	            red = c.red >> 8;
	            green = c.green >> 8;
	            blue  = c.blue >> 8;
                    alpha = ((alpha > -1) && (colormap->colors[alpha].pixel == 0)) ? 0 : 255;
		}
		else {
		    //Direct.
	    	    red = (pixel & visual->red_mask) >> visual->red_shift;	
	            green = (pixel & visual->green_mask) >> visual->green_shift;	
	            blue = (pixel & visual->blue_mask) >> visual->blue_shift;	

                    red <<= red_mask_width;
                    green <<= green_mask_width;
                    blue <<= blue_mask_width;

                    alpha = (alpha == 0) ? 0 : 255;               
		}
		
                //Assume that the Java defaultRGB colormodel is a DirectColorModel

        	RGB_SETUPFORDIRECTCM(env, colorModel);

                if(red_scale != 0) {
                    red = red * red_scale / 255;	    
                }
                if(green_scale != 0) {
                    green = green * green_scale / 255;	    
                }
                if(blue_scale != 0) {
                    blue = blue * blue_scale / 255;	    
                }
                red = (red & 0xff) << red_offset;
                green = (green & 0xff) << green_offset;
                blue = (blue & 0xff) << blue_offset;
                alpha = (alpha & 0xff) << alpha_offset;
                pixel = (red | green | blue | alpha);

                pixelArrayElements[offset + (y-startY)*scansize + (x-startX)] = pixel;
    	    }
	}

        if(alpha_image != NULL)
                gdk_image_destroy(alpha_image);

	gdk_image_destroy(image);

	awt_gtk_threadsLeave();

        (*env)->ReleaseIntArrayElements(env, rgbArray, pixelArrayElements, 0);

}
