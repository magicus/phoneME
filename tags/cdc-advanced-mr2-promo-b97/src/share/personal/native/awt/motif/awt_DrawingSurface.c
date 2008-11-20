/*
 * @(#)awt_DrawingSurface.c	1.17 06/10/10
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
#include "image.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MDrawingSurfaceInfo.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MDrawingSurfaceInfo_hFID = (*env)->GetFieldID(env, cls, "h", "I");
	MCachedIDs.MDrawingSurfaceInfo_imgrepFID =
		(*env)->GetFieldID(env, cls, "imgrep", "Lsun/awt/image/ImageRepresentation;");
	MCachedIDs.MDrawingSurfaceInfo_peerFID =
		(*env)->GetFieldID(env, cls, "peer", "Lsun/awt/motif/MComponentPeer;");
	MCachedIDs.MDrawingSurfaceInfo_wFID = (*env)->GetFieldID(env, cls, "w", "I");
	MCachedIDs.MDrawingSurfaceInfo_stateFID = (*env)->GetFieldID(env, cls, "state", "I");
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_lock(JNIEnv * env, jobject this)
{
	jobject         peer;
	jint            state;

	state = (*env)->GetIntField(env, this, MCachedIDs.MDrawingSurfaceInfo_stateFID);

	AWT_LOCK();

	peer = (*env)->GetObjectField(env, this, MCachedIDs.MDrawingSurfaceInfo_peerFID);

	if (peer != NULL) {

		jobject         target = (*env)->GetObjectField(env, peer, MCachedIDs.MComponentPeer_targetFID);

		jint            width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
		jint            height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

		if ((*env)->GetIntField(env, this, MCachedIDs.MDrawingSurfaceInfo_wFID) != width ||
		    (*env)->GetIntField(env, this, MCachedIDs.MDrawingSurfaceInfo_hFID) != height) {
			(*env)->SetIntField(env, this, MCachedIDs.MDrawingSurfaceInfo_wFID, width);
			(*env)->SetIntField(env, this, MCachedIDs.MDrawingSurfaceInfo_hFID, height);
			(*env)->SetIntField(env, this, MCachedIDs.MDrawingSurfaceInfo_stateFID, ++state);
		}
	}
	return state;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_unlock(JNIEnv * env, jobject this)
{
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_getDisplay(JNIEnv * env, jobject this)
{
	return (jint) awt_display;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_getDrawable(JNIEnv * env, jobject this)
{
	struct CanvasData *cdata;
	Drawable        drawable = 0;
	jobject         peer, imgrep;

	AWT_LOCK();

	peer = (*env)->GetObjectField(env, this, MCachedIDs.MDrawingSurfaceInfo_peerFID);

	if (peer != NULL) {
		cdata = (struct CanvasData *) (*env)->GetIntField(env, peer, MCachedIDs.MComponentPeer_pDataFID);

		if (cdata != NULL) {
			Widget          win = cdata->comp.widget;

			drawable = XtWindow(win);
		}
	} else {
		imgrep = (*env)->GetObjectField(env, this, MCachedIDs.MDrawingSurfaceInfo_imgrepFID);
		if (imgrep != NULL)
			drawable = image_getIRDrawable(env, imgrep);
	}

	AWT_UNLOCK();

	if (drawable == 0) {
		(*env)->ThrowNew(env,
			     MCachedIDs.java_lang_NullPointerExceptionClass,
			 "unable to determine Drawable for DrawingSurface");
	}
	return (jint) drawable;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_getDepth(JNIEnv * env, jobject this)
{
	return (jint) awt_depth;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_getVisualID(JNIEnv * env, jobject this)
{
	return (jint) XVisualIDFromVisual(awt_visual);
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MDrawingSurfaceInfo_getColormapID(JNIEnv * env, jobject this)
{
	return (jint) awt_cmap;
}
