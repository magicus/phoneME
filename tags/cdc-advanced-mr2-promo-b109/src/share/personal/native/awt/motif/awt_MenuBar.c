/*
 * @(#)awt_MenuBar.c	1.42 06/10/10
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
#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "java_awt_Menu.h"
#include "java_awt_Frame.h"
#include "sun_awt_motif_MFramePeer.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuBarPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MMenuBarPeer_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuBarPeer_create(JNIEnv * env, jobject this, jobject frame)
{
	Arg             args[20];
	int             argc;
	struct ComponentData *componentData;
	struct FrameData *frameData;
	Pixel           bg;

	if (frame == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	frameData = (struct FrameData *) (*env)->GetIntField(env, frame, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	componentData = (struct ComponentData *)XtCalloc(1, sizeof(struct ComponentData));

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return;
	}
	AWT_LOCK();

	(*env)->SetIntField(env, this, MCachedIDs.MMenuBarPeer_pDataFID, (jint) componentData);

	XtVaGetValues(frameData->winData.comp.widget, XmNbackground, &bg, NULL);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;

	componentData->widget = XmCreateMenuBar(frameData->mainWindow, "menubar:", args, argc);

	/* XtSetMappedWhenManaged(componentData->widget, False); */
	XtManageChild(componentData->widget);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuBarPeer_pDispose(JNIEnv * env, jobject this)
{
	struct ComponentData *componentData;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuBarPeer_pDataFID);

	if (componentData == NULL) {
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(componentData->widget);
	awt_util_consumeAllXEvents(componentData->widget);
        awt_removeEventHandlerForWidget(componentData->widget,
                                        componentData->peerGlobalRef);
	XtDestroyWidget(componentData->widget);
	XtFree((void *) componentData);
	(*env)->SetIntField(env, this, MCachedIDs.MMenuBarPeer_pDataFID, (jint) NULL);
	AWT_UNLOCK();
}
