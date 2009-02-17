/*
 * @(#)awt_Window.c	1.54 06/10/10
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
#include <X11/Shell.h>
#include <X11/Xmu/Editres.h>
#include <Xm/RowColumn.h>
#include <Xm/AtomMgr.h>
#include <Xm/MenuShell.h>
#include <Xm/MwmUtil.h>
#include "java_awt_Window.h"
#include "java_awt_Insets.h"
#include "sun_awt_motif_MWindowPeer.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "canvas.h"
#include "color.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MWindowPeer_insetsFID = (*env)->GetFieldID(env, cls, "insets", "Ljava/awt/Insets;");
	MCachedIDs.MWindowPeer_handleDeiconifyMID = (*env)->GetMethodID(env, cls, "handleDeiconify", "()V");
	MCachedIDs.MWindowPeer_handleIconifyMID = (*env)->GetMethodID(env, cls, "handleIconify", "()V");

	if (MCachedIDs.java_awt_Window_warningStringFID == NULL) {
		cls = (*env)->FindClass(env, "java/awt/Window");

		if (cls == NULL)
			return;

		MCachedIDs.java_awt_Window_warningStringFID =
			(*env)->GetFieldID(env, cls, "warningString", "Ljava/lang/String;");
	}
}

static void
Window_event_handler(Widget w, XtPointer data, XEvent * event, Boolean * cont)
{
	struct FrameData *frameData;
	JNIEnv         *env;
	jobject         this = (jobject) data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	/*
	 * Any event handlers which take peer instance pointers as
	 * client_data should check to ensure the widget has not been marked
	 * as destroyed as a result of a dispose() call on the peer (which
	 * can result in the peer instance pointer already haven been gc'd by
	 * the time this event is processed)
	 */
	if (w->core.being_destroyed)
		return;

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL)
		return;

	switch (event->type) {
	case MapNotify:

		if (frameData->mappedOnce == 0) {
			frameData->mappedOnce = 1;
			(*env)->CallVoidMethod(env, this, MCachedIDs.MPanelPeer_makeCursorsVisibleMID);
		} else
			(*env)->CallVoidMethod(env, this, MCachedIDs.MWindowPeer_handleDeiconifyMID);

		break;

	case UnmapNotify:
		(*env)->CallVoidMethod(env, this, MCachedIDs.MWindowPeer_handleIconifyMID);
		break;

	default:
		break;
	}

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);
}

static void
changeInsets(JNIEnv * env, jobject this, struct FrameData * frameData)
{
	jobject         insets = (*env)->GetObjectField(env, this, MCachedIDs.MWindowPeer_insetsFID);

	if (insets == NULL)
		return;

	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_topFID, frameData->top);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_leftFID, frameData->left);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_bottomFID, frameData->bottom);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_rightFID, frameData->right);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	Dimension       w, h;
	Position        x, y;
	Arg             args[20];
	int             argc;
	struct FrameData *frameData;
	struct FrameData *parentFrameData;
	jobject         target;
	jstring         warningString;
	jobject         thisGlobalRef;
	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL || parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	frameData = (struct FrameData *)XtCalloc(1, sizeof(struct FrameData));

	if (frameData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) frameData);
	parentFrameData =
		(struct FrameData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);

	x = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID);
	y = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID);
	w = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
	h = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);
	if(w < 1) w=1;         if(h < 1) h=1;

	argc = 0;
	XtSetArg(args[argc], XmNtransientFor, parentFrameData->winData.shell);
	argc++;
	XtSetArg(args[argc], XmNsaveUnder, False);
	argc++;
	XtSetArg(args[argc], XmNx, x);
	argc++;
	XtSetArg(args[argc], XmNy, y);
	argc++;
	XtSetArg(args[argc], XmNwidth, w);
	argc++;
	XtSetArg(args[argc], XmNheight, h);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;
	XtSetArg(args[argc], XmNcolormap, awt_cmap);
	argc++;
	XtSetArg(args[argc], XmNdepth, awt_depth);
	argc++;

	frameData->winData.shell = XtCreatePopupShell("Window",
		     xmMenuShellWidgetClass, parentFrameData->winData.shell,
						      args, argc);

	thisGlobalRef = (*env)->NewGlobalRef(env, this);

	XtAddEventHandler(frameData->winData.shell, StructureNotifyMask, FALSE, Window_event_handler, thisGlobalRef);

       	XtAddEventHandler(frameData->winData.shell, (EventMask)0, True, (XtEventHandler) _XEditResCheckMessages, NULL);


	argc = 0;
	XtSetArg(args[argc], XmNwidth, w);
	argc++;
	XtSetArg(args[argc], XmNheight, h);
	argc++;
	XtSetArg(args[argc], XmNmainWindowMarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmainWindowMarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNspacing, 0);
	argc++;

	frameData->mainWindow = XmCreateRowColumn(frameData->winData.shell, "window:", args, argc);
	frameData->top = frameData->left = frameData->bottom = frameData->right = 0;

	warningString = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_Window_warningStringFID);

	if (warningString != NULL) {
		Dimension       hoffset;
		char           *cwarningString = (char *) (*env)->GetStringUTFChars(env, warningString, NULL);

		if (cwarningString == NULL)
			return;

		frameData->warningWindow = awt_util_createWarningWindow(frameData->mainWindow, cwarningString);
		(*env)->ReleaseStringUTFChars(env, warningString, cwarningString);
		XtVaGetValues(frameData->warningWindow, XmNheight, &hoffset, NULL);
		frameData->top += hoffset;
		changeInsets(env, this, frameData);
	} else
		frameData->warningWindow = NULL;

	frameData->winData.comp.widget = awt_canvas_create(env, (XtPointer) thisGlobalRef,
						      frameData->mainWindow,
							   XtName(frameData->mainWindow),
							   w,
							   h,
		      False, (frameData->warningWindow ? frameData : NULL));


	/*	awt_util_show(env, frameData->winData.comp.widget); */
	XtManageChild(frameData->mainWindow);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_pShow(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;
	/* TODO: check the use of these variables 
           Dimension       width, height;
        */
	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || frameData->winData.shell == NULL
	    || frameData->mainWindow == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	/*	XtVaSetValues(frameData->winData.comp.widget, XmNx, -(frameData->left), XmNy, -(frameData->top), NULL); 

	XtVaGetValues(frameData->mainWindow, XmNwidth, &width, XmNheight, &height, NULL);
	*/

	XtPopup(frameData->winData.shell, XtGrabNone);
	XRaiseWindow(awt_display, XtWindow(frameData->winData.shell));

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_pHide(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtPopdown(frameData->winData.shell);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_pDispose(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->mainWindow == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(frameData->mainWindow);
	awt_util_consumeAllXEvents(frameData->mainWindow);
	awt_util_consumeAllXEvents(frameData->winData.shell);
	XtDestroyWidget(frameData->mainWindow);
	XtDestroyWidget(frameData->winData.shell);

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_pReshape(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct FrameData *frameData;
	Dimension       hoffset;
	short           aw, ah;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}


	if (frameData->warningWindow != 0)
		XtVaGetValues(frameData->warningWindow, XmNheight, &hoffset, NULL);

	else
		hoffset = 0;

	aw = w - (frameData->left + frameData->right);

	if (aw < 1)
		aw = 1;

	ah = h + hoffset - (frameData->top + frameData->bottom);

	if (ah < 1)
		ah = 1;


	XtVaSetValues(frameData->winData.shell,
		      XtNx, (XtArgVal) x,
		      XtNy, (XtArgVal) y, XtNwidth, (XtArgVal) aw, XtNheight, (XtArgVal) ah, NULL);

	XtVaSetValues(frameData->winData.comp.widget,
		      XtNx, (XtArgVal) x - frameData->left,
		      XtNy, (XtArgVal) y - frameData->top,
		      XtNwidth, (XtArgVal) w + frameData->left + frameData->right,
		      XtNheight, (XtArgVal) h + frameData->top + frameData->bottom, NULL);


	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MWindowPeer_toBack(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (XtWindow(frameData->winData.shell) != 0)
		XLowerWindow(awt_display, XtWindow(frameData->winData.shell));

	AWT_UNLOCK();
}
