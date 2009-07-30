/*
 * @(#)GdkGraphics.c	1.29 06/10/10
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

#include <string.h>

#include "sun_awt_gtk_GdkGraphics.h"
#include "GdkGraphics.h"
#include "GComponentPeer.h"
#include "ImageRepresentation.h"

#define max(x, y) (((int)(x) > (int)(y)) ? (x) : (y))
#define min(x, y) (((int)(x) < (int)(y)) ? (x) : (y))

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_initIDs (JNIEnv *env, jclass cls)
{
	GCachedIDs.GdkGraphicsClass = (*env)->NewGlobalRef (env, cls);
	GET_FIELD_ID (GdkGraphics_dataFID, "data", "I");
	GET_FIELD_ID (GdkGraphics_originXFID, "originX", "I");
	GET_FIELD_ID (GdkGraphics_originYFID, "originY", "I");
	GET_FIELD_ID (GdkGraphics_peerFID, "peer", "Lsun/awt/gtk/GComponentPeer;");
	GET_METHOD_ID (GdkGraphics_constructWithPeerMID, "<init>", "(Lsun/awt/gtk/GComponentPeer;)V");
	
	GET_CLASS (sun_awt_NullGraphicsClass, "sun/awt/NullGraphics");
	GET_METHOD_ID (sun_awt_NullGraphics_constructorMID, "<init>", "(Ljava/awt/Component;)V");
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GdkGraphics_createFromComponentNative (JNIEnv *env, jclass cls, jobject componentPeer)
{
	GComponentPeerData *peerData = awt_gtk_getData (env, componentPeer);
	GdkGraphicsData *data;
	GdkWindow *window;
	jobject graphics = NULL;
	
	if (peerData == NULL)
		return NULL;
		
	window = peerData->drawWidget->window;
	
	/* Unlike X/Motif Gtk doesn't create windows until a widget is shown on the screen.
	   This means that we can't create a GdkGraphics for the widget when it is displayable
	   (i.e. its peer is created) but not visible on the screen. However, the Java spec.
		requires that a graphics object is still available. In the case where the component
		is not showing on the screen we therefore return a NullGraphics (a graphics object that
		ignores all drawing requests). */
	   
	
	if (window == NULL)
	{
		jobject target = (*env)->GetObjectField (env, componentPeer, GCachedIDs.GComponentPeer_targetFID);
		graphics = (*env)->NewObject (env, GCachedIDs.sun_awt_NullGraphicsClass, GCachedIDs.sun_awt_NullGraphics_constructorMID, target);
	}
		
	/* Create a new GdkGraphics. */
	
	else
	{
		graphics = (*env)->NewObject (env, cls, GCachedIDs.GdkGraphics_constructWithPeerMID, componentPeer);
		
		if (graphics == NULL)
			return NULL;
			
		/* Create the GdkGraphicsData and set it on this object. */
		
		data = calloc (1, sizeof (GdkGraphicsData));
		(*env)->SetIntField (env, graphics, GCachedIDs.GdkGraphics_dataFID, (jint)data);
		
		if (data == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			return NULL;
		}
		
		/* Create the GC used for colors etc. */
		
		awt_gtk_threadsEnter();
		gdk_window_ref (window);
		data->drawable = window;
		data->isPixmap = FALSE;
		data->gc = gdk_gc_new (data->drawable);
                data->alpha_rule = 0;
                data->extra_alpha = 0;
		awt_gtk_threadsLeave();
		
		if (data->gc == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			return NULL;
		}
		
		data->clipHasBeenSet = FALSE;
		data->xorMode = FALSE;
	}
	
	return graphics;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_createFromGraphics (JNIEnv *env, jobject this, jobject graphics)
{
	GdkGraphicsData *data;
	GdkGraphicsData *graphicsData;
	
	graphicsData = (GdkGraphicsData *)(*env)->GetIntField (env, graphics, GCachedIDs.GdkGraphics_dataFID);
	
	if (graphicsData == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}

	/* Create the GdkGraphicsData and set it on this object. */
	
	data = malloc (sizeof (GdkGraphicsData));
	(*env)->SetIntField (env, this, GCachedIDs.GdkGraphics_dataFID, (jint)data);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	memcpy(data, graphicsData, sizeof (GdkGraphicsData));
	
	awt_gtk_threadsEnter();

	if (graphicsData->isPixmap)
		gdk_pixmap_ref ((GdkPixmap *)graphicsData->drawable);
		
	else
		gdk_window_ref ((GdkWindow *)graphicsData->drawable);
	
	data->gc = gdk_gc_new (data->drawable);
        gdk_gc_copy(data->gc, graphicsData->gc);

	awt_gtk_threadsLeave();
	
	if (data->gc == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}

}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_createFromImage (JNIEnv *env, jobject this, jobject imageRep)
{
	GdkGraphicsData *data;
	ImageRepresentationData *imageRepresentationData;

	/* Create the GdkGraphicsData and set it on this object. */
	
	data = calloc (1, sizeof (GdkGraphicsData));
	(*env)->SetIntField (env, this, GCachedIDs.GdkGraphics_dataFID, (jint)data);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	imageRepresentationData = awt_gtk_getImageRepresentationData (env, imageRep, NULL);
	
	if (imageRepresentationData == NULL)
	{
		awt_gtk_threadsLeave();
		return;
	}
	
	gdk_pixmap_ref (imageRepresentationData->pixmap);
	data->drawable = imageRepresentationData->pixmap;

	data->gc = gdk_gc_new (data->drawable);
        gdk_gc_copy(data->gc, imageRepresentationData->gc);

	data->isPixmap = TRUE;
	
	awt_gtk_threadsLeave();
	
	if (data->gc == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}

	data->clipHasBeenSet = FALSE;
	data->xorMode = FALSE;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_dispose (JNIEnv *env, jobject this)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	
	if (data == NULL)
		return;
		
	awt_gtk_threadsEnter();
	
	if (data->drawable != NULL)
	{
		if (data->isPixmap)
			gdk_pixmap_unref ((GdkPixmap *)data->drawable);
			
		else gdk_window_unref ((GdkWindow *)data->drawable);
	}
	
	if (data->gc != NULL)
		gdk_gc_unref (data->gc);
	
	awt_gtk_threadsLeave();
		
	free (data);
		
	(*env)->SetIntField (env, this, GCachedIDs.GdkGraphics_dataFID, (jint)NULL);
}

/* Given a java.awt.Color object this function will initialize the supplied GdkColor structure with the
   pixel value from the system color map which is the nearest color to the supplied java color
   object.
   returns TRUE if successful or FALSE if an exception has been thrown. */

gboolean
awt_gtk_getColor (JNIEnv *env, jobject color, GdkColor *gdkcolor)
{
	gboolean result = JNI_TRUE;
	
	jint rgb = (*env)->GetIntField (env, color, GCachedIDs.java_awt_Color_valueFID);
		
	gdkcolor->red = (65535 / 255) * ((0xff0000 & rgb) >> 16);
	gdkcolor->green = (65535 / 255) * ((0x00ff00 & rgb) >> 8);
	gdkcolor->blue = (65535 / 255) * (0x0000ff & rgb);
	
	awt_gtk_threadsEnter();
	result = gdk_colormap_alloc_color (gdk_colormap_get_system(), gdkcolor, FALSE, TRUE);
	awt_gtk_threadsLeave();
	
	if (!result)
		(*env)->ThrowNew (env, GCachedIDs.java_awt_AWTErrorClass, "could not allocate color");
		
	return result;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_setForegroundNative (JNIEnv *env, jobject this, jobject color)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	GdkColor foreground;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	if (!awt_gtk_getColor (env, color, &foreground))
		return;
		
	data->foreground = foreground;
	
	if (data->xorMode)
		foreground.pixel ^= data->xorColor.pixel;

	gdk_gc_set_foreground (data->gc, &foreground);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_setPaintMode (JNIEnv *env, jobject this)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	data->xorMode = FALSE;
	
	awt_gtk_threadsEnter();
	gdk_gc_set_function (data->gc, GDK_COPY);
	gdk_gc_set_foreground (data->gc, &data->foreground);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_setXORMode (JNIEnv *env, jobject this, jobject color)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	GdkColor xorColor;
	
	if (data == NULL || color == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	if (!awt_gtk_getColor (env, color, &xorColor))
		return;
		
	data->xorColor = xorColor;
	data->xorMode = TRUE;
	
	xorColor.pixel ^= data->foreground.pixel;
	
	awt_gtk_threadsEnter();
	gdk_gc_set_function (data->gc, GDK_XOR);
	gdk_gc_set_foreground (data->gc, &xorColor);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_setClipNative (JNIEnv * env, jobject this,
										   jint x, jint y, jint w, jint h)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	
	awt_gtk_threadsEnter();
	
	data->clipBounds.x = x;
	data->clipBounds.y = y;
	data->clipBounds.width = w;
	data->clipBounds.height = h;
	data->clipHasBeenSet = TRUE;

	gdk_gc_set_clip_rectangle (data->gc, &data->clipBounds);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_removeClip (JNIEnv *env, jobject this)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	data->clipHasBeenSet = FALSE;
	gdk_gc_set_clip_rectangle (data->gc, NULL);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_drawLineNative (JNIEnv *env, jobject this, jint x, jint y, jint x2, jint y2)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	gdk_draw_line (data->drawable, data->gc, originX + x, originY + y, originX + x2, originY + y2);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_drawRectNative (JNIEnv *env, jobject this, jboolean fill, jint x, jint y, jint width, jint height)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	gdk_draw_rectangle (data->drawable, data->gc, (fill == JNI_TRUE) ? TRUE : FALSE, originX + x, originY + y, width, height);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_clearRect (JNIEnv *env, jobject this, jint x, jint y, jint width, jint height)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	jobject peer;
	jobject background = NULL;
	GdkColor color;
	GdkGC *newGC;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	

	/* Get the background color for the drawing surface. If this graphics was created from a component
	   then we use the component's background color. If it is from an image then we use the image's
	   background color. */
	   
	peer = (*env)->GetObjectField (env, this, GCachedIDs.GdkGraphics_peerFID);
	
	if (peer != NULL)
	{
		jobject target = (*env)->GetObjectField (env, peer, GCachedIDs.GComponentPeer_targetFID);
		background = (*env)->CallObjectMethod (env, target, GCachedIDs.java_awt_Component_getBackgroundMID);

                if (background != NULL)
                {
                        if (!awt_gtk_getColor (env, background, &color))
                        {
                                awt_gtk_threadsLeave();
                                return;
                        }
                }

                else /* Clear with black if background cannot be determined. */
                {
                        color.red = 0;
                        color.green = 0;
                        color.blue = 0;
                        gdk_colormap_alloc_color (gdk_colormap_get_system(), &color, FALSE, TRUE);
                }

        }	
	else
	{
                GdkGCValues gcValues;

                gdk_gc_get_values(data->gc, &gcValues);

                color = gcValues.background;
	}
	

	/* We create a new graphics context as the API specifies that clearRect does not use the current paint mode. */
	
	newGC = gdk_gc_new (data->drawable);
	
	if (data->clipHasBeenSet)
		gdk_gc_set_clip_rectangle (newGC, &data->clipBounds);
		
	gdk_gc_set_foreground (newGC, &color);
	gdk_draw_rectangle (data->drawable, newGC, TRUE, originX + x, originY + y, width, height);
	gdk_gc_unref (newGC);
	
	awt_gtk_threadsLeave();
}

static void
awt_gtk_drawArc (GdkGraphicsData *data,
 		jint x, jint y, jint width, jint height,
 		jint startAngle, jint endAngle, jboolean filled)
{
	if (width < 0 || height < 0)
		return;

	if (endAngle >= 360 || endAngle <= -360)
	{
		startAngle = 0;
		endAngle = 360 * 64;
	}
	
	else
	{
		startAngle %= 360;
		startAngle *= 64;
		endAngle *= 64;
	}
	
	gdk_draw_arc (data->drawable, data->gc, (filled == JNI_TRUE) ? TRUE : FALSE, x, y, width, height, startAngle, endAngle);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_drawArcNative (JNIEnv *env, jobject this,
									  jint x,
									  jint y,
									  jint width,
									  jint height,
									  jint startAngle,
									  jint endAngle)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	awt_gtk_drawArc (data, originX + x, originY + y, width, height, startAngle, endAngle, JNI_FALSE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_fillArcNative (JNIEnv *env, jobject this,
									  jint x,
									  jint y,
									  jint width,
									  jint height,
									  jint startAngle,
									  jint endAngle)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	awt_gtk_drawArc (data, originX + x, originY + y, width, height, startAngle, endAngle, JNI_TRUE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_drawRoundRectNative (JNIEnv *env, jobject this,
									  jint x,
									  jint y,
									  jint width,
									  jint height,
									  jint arcWidth,
									  jint arcHeight)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	jint start, end, pos;
	jint arcRadiusWidth;
	jint arcRadiusHeight;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	if (width < 0 || height < 0)
		return;
	
	x += originX;
	y += originY;
	
	arcWidth = ABS(arcWidth);
	arcHeight = ABS(arcHeight);
	
	if (arcWidth > width)
		arcWidth = width;
		
	if (arcHeight > height)
		arcHeight = height;
		
	arcRadiusWidth = arcWidth / 2;
	arcRadiusHeight = arcHeight / 2;
	
	awt_gtk_threadsEnter();
	
	awt_gtk_drawArc (data, x, y, arcWidth, arcHeight, 90, 90, JNI_FALSE);
	awt_gtk_drawArc (data, x + width - arcWidth, y, arcWidth, arcHeight, 0, 90, JNI_FALSE);
	awt_gtk_drawArc (data, x, y + height - arcHeight, arcWidth, arcHeight, 180, 90, JNI_FALSE);
	awt_gtk_drawArc (data, x + width - arcWidth, y + height - arcHeight, arcWidth, arcHeight, 270, 90, JNI_FALSE);
	
	start = x + arcRadiusWidth + 1;
	end = x + width - arcRadiusWidth - 1;
	pos = y + height;
	
	gdk_draw_line (data->drawable, data->gc, start, y, end, y);
	gdk_draw_line (data->drawable, data->gc, start, pos, end, pos);
	
	start = y + arcRadiusHeight + 1;
	end = y + height - arcRadiusHeight - 1;
	pos = x + width;
	
	gdk_draw_line (data->drawable, data->gc, x, start, x, end);
	gdk_draw_line (data->drawable, data->gc, pos, start, pos, end);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_fillRoundRectNative (JNIEnv *env, jobject this,
									  jint x,
									  jint y,
									  jint width,
									  jint height,
									  jint arcWidth,
									  jint arcHeight)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	jint arcRadiusWidth;
	jint arcRadiusHeight;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	if (width < 0 || height < 0)
		return;
	
	x += originX;
	y += originY;
	
	arcWidth = ABS(arcWidth);
	arcHeight = ABS(arcHeight);
	
	if (arcWidth > width)
		arcWidth = width;
		
	if (arcHeight > height)
		arcHeight = height;
		
	arcRadiusWidth = arcWidth / 2;
	arcRadiusHeight = arcHeight / 2;
	
	awt_gtk_threadsEnter();
	
	awt_gtk_drawArc (data, x, y, arcRadiusWidth*2, arcRadiusHeight*2, 90, 90, JNI_TRUE);
	awt_gtk_drawArc (data, x + width - (arcRadiusWidth*2), y, arcRadiusWidth*2, arcRadiusHeight*2, 0, 90, JNI_TRUE);
	awt_gtk_drawArc (data, x, y + height - (arcRadiusHeight*2), arcRadiusWidth*2, arcRadiusHeight*2, 180, 90, JNI_TRUE);
	awt_gtk_drawArc (data, x + width - (arcRadiusWidth*2), y + height - (arcRadiusHeight*2), arcRadiusWidth*2, arcRadiusHeight*2, 270, 90, JNI_TRUE);
	
	gdk_draw_rectangle (data->drawable, data->gc, TRUE, x + arcRadiusWidth, y, width - (arcRadiusWidth*2), height);
	gdk_draw_rectangle (data->drawable, data->gc, TRUE, x, y + arcRadiusHeight, arcRadiusWidth, height - (arcRadiusHeight*2));
	gdk_draw_rectangle (data->drawable, data->gc, TRUE, x + width - arcRadiusWidth, y + arcRadiusHeight, arcRadiusWidth, height - (arcRadiusHeight*2));
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_copyArea (JNIEnv *env, jobject this, jint x, jint y, jint width, jint height, jint dx, jint dy)
{
	GdkGraphicsData *data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);

	if ( (width <= 0) || (height <= 0) ) { /* Fix for 4727883 */
	    /* negative values crash X server and are not specified */
            return;
	}

	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	x += originX;
	y += originY;
	
	awt_gtk_threadsEnter();
	gdk_window_copy_area (data->drawable, data->gc, x + dx, y + dy, data->drawable, x, y, width, height);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_drawStringNative (JNIEnv *env, jobject this, jbyteArray text, jint textLength, jint gfont, jint x, jint y)
{
	GdkGraphicsData *data;
	gchar *gtext;
	GdkFont *gdkfont;
	
	data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);

	if (gfont == NULL || data == NULL || text == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
/*
	gdkfont = (GdkFont *)(*env)->GetIntField (env, fontPeer, GCachedIDs.GFontPeer_dataFID);
*/

	gdkfont = (GdkFont *)gfont;

/*	
	if (gdkfont == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
*/		
	x += (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	y += (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	
	gtext = (gchar *)(*env)->GetByteArrayElements (env, text, NULL);
	
	if (gtext == NULL)
		return;
/*		
	textLength = (*env)->GetArrayLength(env, text);
*/
	awt_gtk_threadsEnter();
	gdk_draw_text (data->drawable, gdkfont, data->gc, x, y, gtext, textLength);
	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, text, gtext, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GdkGraphics_drawPolygonNative (JNIEnv *env,
												jobject this,
												jboolean filled,
												jintArray xPointsArray,
												jintArray yPointsArray,
												jint nPoints,
												jboolean close)
{
	GdkGraphicsData *data;
	jint *xPoints;
	jint *yPoints;
	GdkPoint pointsArray[10];
	GdkPoint *points;
	int i;
	jint originX = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originXFID);
	jint originY = (*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_originYFID);
	
	if (nPoints == 0)
		return;
	
	data = (GdkGraphicsData *)(*env)->GetIntField (env, this, GCachedIDs.GdkGraphics_dataFID);
	
	if (data == NULL || xPointsArray == NULL || yPointsArray == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	if (nPoints > (*env)->GetArrayLength (env, xPointsArray) ||
	    nPoints > (*env)->GetArrayLength (env, yPointsArray))
	{
		(*env)->ThrowNew (env, GCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
		return;
	}
	
	xPoints = (*env)->GetIntArrayElements (env, xPointsArray, NULL);
	
	if (xPoints == NULL)
		return;
		
	yPoints = (*env)->GetIntArrayElements (env, yPointsArray, NULL);
	
	if (yPoints == NULL)
		return;
		
	/* Simple optimization to prevent malloc and free in most cases. This can reduce
	   memory fragmentation and provides a small speed improvement. */
		
	if (nPoints <= 10)
		points = pointsArray;
		
	else
	{
		points = malloc (sizeof (GdkPoint) * nPoints);
	
		if (points == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
			return;
		}
	}
	
	for (i = 0; i < nPoints; i++)
	{
		points[i].x = xPoints[i] + originX;
		points[i].y = yPoints[i] + originY;
	}
	
	(*env)->ReleaseIntArrayElements (env, xPointsArray, xPoints, JNI_ABORT);
	(*env)->ReleaseIntArrayElements (env, yPointsArray, yPoints, JNI_ABORT);
	
	awt_gtk_threadsEnter();
	
	if (close == JNI_TRUE || filled)
	  gdk_draw_polygon (data->drawable, data->gc, (gint)filled, points, (gint)nPoints);
	else {
	  for (i = 0; i < nPoints-1; i++) {
	    gdk_draw_line (data->drawable, data->gc, points[i].x, points[i].y, points[i+1].x, points[i+1].y);
	  }
	}
	awt_gtk_threadsLeave();
	
	if (nPoints > 10)
		free (points);
}


