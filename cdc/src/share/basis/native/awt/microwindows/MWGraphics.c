/*
 * @(#)MWGraphics.c	1.29 06/10/10
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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "java_awt_MWGraphics.h"
#include "java_awt_AlphaComposite.h"

JNIEXPORT jint JNICALL
Java_java_awt_MWGraphics_pClonePSD (JNIEnv *env, jclass cls, jint _graphicsEngine)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	return (jint)graphicsEngine->Clone(graphicsEngine);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDisposePSD (JNIEnv *env, jclass cls, jint _graphicsEngine)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	if (graphicsEngine != NULL)
		graphicsEngine->Destroy(graphicsEngine);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pSetColor (JNIEnv * env, jclass cls, jint _graphicsEngine, jint rgb)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->SetForegroundColor(graphicsEngine, rgb);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pSetComposite (JNIEnv * env, jclass cls, jint _graphicsEngine, jint rule, jfloat _extraAlpha)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOMPOSITE composite;

	unsigned char extraAlpha = (unsigned char)(_extraAlpha * 255);

	switch (rule)
	{
		case java_awt_AlphaComposite_SRC:
			composite = COMPOSITE_SRC;
		break;

		case java_awt_AlphaComposite_SRC_OVER:
			composite = COMPOSITE_SRC_OVER;
		break;

		case java_awt_AlphaComposite_CLEAR:
			composite = COMPOSITE_CLEAR;
		break;

		default:
			(*env)->ThrowNew (env, MWCachedIDs.AWTError, "Unknown composite rule");
			return;
	}

	graphicsEngine->SetComposite(graphicsEngine, composite, extraAlpha);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pSetPaintMode (JNIEnv * env, jclass cls, jint _graphicsEngine)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->SetComposite(graphicsEngine, COMPOSITE_SRC_OVER, 255);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pSetXORMode (JNIEnv * env, jclass cls, jint _graphicsEngine, jint rgb)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->SetXORColor(graphicsEngine, rgb);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pFillRect (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->FillRect(graphicsEngine, x, y, w, h);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pClearRect (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h, jint rgb)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->ClearRect (graphicsEngine, x, y, w, h, graphicsEngine->colorModel->ConvertColorToPixel(rgb));
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawRect (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->DrawRect (graphicsEngine, x, y, w + 1, h + 1);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pCopyArea (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h,
									jint dx, jint dy)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->CopyArea (graphicsEngine, x, y, w, h, dx, dy);
}


JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawLine (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x1, jint y1, jint x2, jint y2)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	graphicsEngine->DrawLine (graphicsEngine, x1, y1, x2, y2);
}


JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawPolygon (JNIEnv * env, jclass cls, jint _graphicsEngine, jint originX, jint originY,
									   jintArray xp, jintArray yp, jint nPoints, jboolean close)
{
	MWPOINT *points;
	jint *xpoints, *ypoints;
	int i;
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	points = (MWPOINT *) calloc (nPoints, sizeof (MWPOINT));

	xpoints = (*env)->GetIntArrayElements (env, xp, NULL);
	ypoints = (*env)->GetIntArrayElements (env, yp, NULL);

	for (i = 0; i < nPoints; i++)
	{
		points[i].x = xpoints[i] + originX;
		points[i].y = ypoints[i] + originY;
	}

	(*env)->ReleaseIntArrayElements (env, xp, xpoints, JNI_ABORT);
	(*env)->ReleaseIntArrayElements (env, yp, ypoints, JNI_ABORT);

	graphicsEngine->DrawPolygon (graphicsEngine, nPoints, points);

	if (close)
		graphicsEngine->DrawLine (graphicsEngine, points[0].x, points[0].y, points[nPoints - 1].x, points[nPoints - 1].y);

	free (points);
}


JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pFillPolygon (JNIEnv * env, jclass cls, jint _graphicsEngine, jint originX, jint originY,
									   jintArray xp, jintArray yp, jint nPoints)
{
	MWPOINT *points;
	jint *xpoints, *ypoints;
	int i;
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	points = (MWPOINT *) calloc (nPoints, sizeof (MWPOINT));

	xpoints = (*env)->GetIntArrayElements (env, xp, NULL);
	ypoints = (*env)->GetIntArrayElements (env, yp, NULL);

	for (i = 0; i < nPoints; i++)
	{
		points[i].x = xpoints[i] + originX;
		points[i].y = ypoints[i] + originY;
	}

	(*env)->ReleaseIntArrayElements (env, xp, xpoints, JNI_ABORT);
	(*env)->ReleaseIntArrayElements (env, yp, ypoints, JNI_ABORT);

	graphicsEngine->FillPolygon (graphicsEngine, nPoints, points);

	free (points);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawArc (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint width,
								   jint height, jint start, jint end)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOORD rx = width / 2;
	MWCOORD ry = height / 2;

	/*
	** if startAngle is negative rotation to first line is clockwise
	*/
	if (start < 0)
	  start += 360;

	if (end  >0) {
	  /*
	  ** we're going anti-clockwise
	  */
	  graphicsEngine->DrawArc (graphicsEngine, x + rx, y + ry, rx, ry, start*64, (start+end)*64, MWARC);
	}
	else {
	  /*
	  ** we're going clockwise
	  */
	  end = abs(end);
	  graphicsEngine->DrawArc (graphicsEngine, x + rx, y + ry, rx, ry, (start-end)*64, start*64, MWARC);
	}

}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pFillArc (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint width,
								   jint height, jint start, jint end)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOORD rx = width / 2;
	MWCOORD ry = height / 2;

	/*
	** if startAngle is negative rotation to first line is clockwise
	*/
	if (start < 0)
	  start += 360;

	if (end  >0) {
	  /*
	  ** we're going anti-clockwise
	  */
	  graphicsEngine->DrawArc (graphicsEngine, x + rx, y + ry, rx, ry, start*64, (start+end)*64, MWPIE);
	}
	else {
	  /*
	  ** we're going clockwise
	  */
	  end = abs(end);
	  graphicsEngine->DrawArc (graphicsEngine, x + rx, y + ry, rx, ry, (start-end)*64, start*64, MWPIE);
	}

}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawOval (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint width,
									jint height)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOORD rx = width / 2;
	MWCOORD ry = height / 2;

	graphicsEngine->DrawEllipse (graphicsEngine, x + rx, y + ry, rx, ry, FALSE);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pFillOval (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint width,
									jint height)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOORD rx = width / 2;
	MWCOORD ry = height / 2;

	graphicsEngine->DrawEllipse (graphicsEngine, x + rx, y + ry, rx, ry, TRUE);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawRoundRect (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h,
										 jint arcW, jint arcH)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOORD rx;
	MWCOORD ry;

	if (arcW >= w)
	  rx = w / 2;
	else
	  rx = arcW / 2;

	if (arcH >= h)
	  ry = h / 2 ;
	else
	  ry = arcH / 2;

	graphicsEngine->DrawLine (graphicsEngine, x, y + ry, x, y + h - ry);
	graphicsEngine->DrawLine (graphicsEngine, x + w, y + ry, x + w, y + h - ry);
	graphicsEngine->DrawLine (graphicsEngine, x + rx, y, x + w - rx, y);
	graphicsEngine->DrawLine (graphicsEngine, x + rx, y + h, x + w - rx, y + h);
	graphicsEngine->DrawArc (graphicsEngine, x + rx, y + ry, rx, ry, 90 * 64, 180 * 64, MWARC);
	graphicsEngine->DrawArc (graphicsEngine, x + rx, y + h - ry, rx, ry, 180 * 64, 270 * 64, MWARC);
	graphicsEngine->DrawArc (graphicsEngine, x + w - rx, y + ry, rx, ry, 0, 90 * 64, MWARC);
	graphicsEngine->DrawArc (graphicsEngine, x + w - rx, y + h - ry, rx, ry, 270 * 64, 0, MWARC);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pFillRoundRect (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h,
										 jint arcW, jint arcH)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;
	MWCOORD rx;
	MWCOORD ry;

	if (arcW >= w)
	  rx = w / 2;
	else
	  rx = arcW / 2;

	if (arcH >= h)
	  ry = h / 2;
	else
	  ry = arcH / 2;

	/*
	** use radius *2 instead of arc? to avoid overlap for odd numbers
	*/
	graphicsEngine->FillRect (graphicsEngine, x, y + ry, w, h - (ry * 2));
	graphicsEngine->FillRect (graphicsEngine, x + rx, y, w - (rx * 2), ry);
	graphicsEngine->FillRect (graphicsEngine, x + rx, y + h - ry, w - (rx * 2), ry);
	graphicsEngine->DrawArc (graphicsEngine, x + rx-1, y + ry-1, rx-1, ry-1, 90 * 64, 180 * 64, MWPIE);    /* NW */
	graphicsEngine->DrawArc (graphicsEngine, x + rx-1, y + h - ry - 1, rx, ry, 180 * 64, 270 * 64, MWPIE); /* SW */
	graphicsEngine->DrawArc (graphicsEngine, x + w - rx, y + ry-1, rx, ry-1, 0, 90 * 64, MWPIE);           /* NE */
	graphicsEngine->DrawArc (graphicsEngine, x + w - rx, y + h - ry - 1, rx, ry, 270 * 64, 0, MWPIE);      /* SE */
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawString (JNIEnv * env, jclass cls, jint _graphicsEngine, jint mwfont, jstring string, jint x,
									  jint y)
{
	const jchar *chars;
	jboolean isCopy;
	jsize length;
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	if (mwfont == 0)
		return;

	chars = (*env)->GetStringChars (env, string, &isCopy);

	if (chars == NULL)
		return;

	length = (*env)->GetStringLength (env, string);

	if (length == 0)
		return;

	mwawt_hardwareInterface->fontRenderer->DrawText(graphicsEngine, (MWFont *)mwfont, (UNICODE_CHAR *)chars, length, x, y);

	(*env)->ReleaseStringChars (env, string, chars);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pDrawChars (JNIEnv * env, jclass cls, jint _graphicsEngine, jint mwfont, jcharArray charArray,
									 jint offset, jint length, jint x, jint y)
{
	jchar *chars;
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	chars = malloc (length * sizeof (jchar));

	if (chars == NULL)
	{
		(*env)->ThrowNew (env, MWCachedIDs.OutOfMemoryError, NULL);
		return;
	}

	(*env)->GetCharArrayRegion (env, charArray, offset, length, chars);

	if ((*env)->ExceptionCheck (env))
	{
		free (chars);
		return;
	}

	mwawt_hardwareInterface->fontRenderer->DrawText(graphicsEngine, (MWFont *)mwfont, (UNICODE_CHAR *)chars, length, x, y);
	free (chars);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pChangeClipRect (JNIEnv * env, jclass cls, jint _graphicsEngine, jint x, jint y, jint w, jint h)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	/* Width and height can be negative when the setClip mthod is used. Negative values imply
	   an empty rectangle in Java so we set these to a width and height of zero. */

	if (w < 0)
		w = 0;

	if (h < 0)
		h = 0;

	graphicsEngine->SetClipRect(graphicsEngine, x, y, w, h);
}


JNIEXPORT void JNICALL
Java_java_awt_MWGraphics_pRemoveClip (JNIEnv * env, jclass cls, jint _graphicsEngine)
{
	GraphicsEngine *graphicsEngine = (GraphicsEngine *)_graphicsEngine;

	GraphicsEngine_RemoveClip(graphicsEngine);
}



JNIEXPORT jobject JNICALL
Java_java_awt_MWGraphics_getBufferedImagePeer(JNIEnv *env, jclass cls, jobject BufferedImage)
{
	return (*env)->GetObjectField(env, BufferedImage, MWCachedIDs.java_awt_image_BufferedImage_peerFID);
}
