/*
 * @(#)awt_Canvas.c	1.37 06/10/10
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
#include "awt_p.h"
#include "awt_util.h"
#include "java_awt_Canvas.h"
#include "sun_awt_motif_MCanvasPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "color.h"
#include "canvas.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCanvasPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	struct CanvasData *canvasData;
	struct CanvasData *parentCanvasData;
	jobject         thisGlobalRef;

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	parentCanvasData = (struct CanvasData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);

	if (parentCanvasData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}

	canvasData = (struct CanvasData *)XtCalloc(1, sizeof(struct CanvasData));

	if (canvasData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) canvasData);

	thisGlobalRef = canvasData->comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}

	AWT_LOCK();

	canvasData->comp.widget =
		awt_canvas_create(env, (XtPointer) thisGlobalRef,
				  parentCanvasData->comp.widget,
				  XtName(parentCanvasData->comp.widget),
				  0, 0, False, NULL);

	XtVaSetValues(canvasData->comp.widget,
		      XmNinsertPosition, awt_util_insertCallback,
		      NULL);

	canvasData->flags = 0;
	canvasData->shell = parentCanvasData->shell;
	AWT_UNLOCK();
}
