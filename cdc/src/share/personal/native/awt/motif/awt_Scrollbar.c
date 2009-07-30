/*
 * @(#)awt_Scrollbar.c	1.51 06/10/10
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
#include "java_awt_Scrollbar.h"
#include "sun_awt_motif_MScrollbarPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include <Xm/ScrollBarP.h>	/* ScrollBar private header. */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollbarPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MScrollbarPeer_dragAbsoluteMID = (*env)->GetMethodID(env, cls, "dragAbsolute", "(I)V");
	MCachedIDs.MScrollbarPeer_lineDownMID = (*env)->GetMethodID(env, cls, "lineDown", "(I)V");
	MCachedIDs.MScrollbarPeer_lineUpMID = (*env)->GetMethodID(env, cls, "lineUp", "(I)V");
	MCachedIDs.MScrollbarPeer_pageDownMID = (*env)->GetMethodID(env, cls, "pageDown", "(I)V");
	MCachedIDs.MScrollbarPeer_pageUpMID = (*env)->GetMethodID(env, cls, "pageUp", "(I)V");

	cls = (*env)->FindClass(env, "java/awt/Scrollbar");

	MCachedIDs.java_awt_Scrollbar_maximumFID = (*env)->GetFieldID(env, cls, "maximum", "I");
	MCachedIDs.java_awt_Scrollbar_minimumFID = (*env)->GetFieldID(env, cls, "minimum", "I");
	MCachedIDs.java_awt_Scrollbar_orientationFID = (*env)->GetFieldID(env, cls, "orientation", "I");
	MCachedIDs.java_awt_Scrollbar_pageIncrementFID = (*env)->GetFieldID(env, cls, "pageIncrement", "I");
	MCachedIDs.java_awt_Scrollbar_valueFID = (*env)->GetFieldID(env, cls, "value", "I");
	MCachedIDs.java_awt_Scrollbar_visibleAmountFID = (*env)->GetFieldID(env, cls, "visibleAmount", "I");
}

/*
 * Remove the ScrollBar widget's continuous scrolling timeout handler on a
 * ButtonRelease to prevent the continuous scrolling that would occur if a
 * timeout expired after the ButtonRelease. This must prevent a bug in Motif.
 */
static void
ButtonReleaseHandler(Widget w, XtPointer data, XEvent * event, Boolean * cont)
{
	/* Remove the timeout handler. */
#define END_TIMER         (1<<2)
	XmScrollBarWidget sbw = (XmScrollBarWidget) w;

	if (sbw->scrollBar.timer != 0) {
		XtRemoveTimeOut(sbw->scrollBar.timer);
		sbw->scrollBar.timer = (XtIntervalId) NULL;
		sbw->scrollBar.flags |= END_TIMER;
	}
}

static void
Scrollbar_lineUp(Widget w, XtPointer client_data, XtPointer call_data)
{
	int             new_value = ((XmScrollBarCallbackStruct *) call_data)->value;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollbarPeer_lineUpMID, new_value);
}

static void
Scrollbar_lineDown(Widget w, XtPointer client_data, XtPointer call_data)
{
	int             new_value = ((XmScrollBarCallbackStruct *) call_data)->value;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollbarPeer_lineDownMID, new_value);
}

static void
Scrollbar_pageUp(Widget w, XtPointer client_data, XtPointer call_data)
{
	int             new_value = ((XmScrollBarCallbackStruct *) call_data)->value;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollbarPeer_pageUpMID, new_value);
}

static void
Scrollbar_pageDown(Widget w, XtPointer client_data, XtPointer call_data)
{
	int             new_value = ((XmScrollBarCallbackStruct *) call_data)->value;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollbarPeer_pageDownMID, new_value);
}

static void
Scrollbar_dragAbsolute(Widget w, XtPointer client_data, XtPointer call_data)
{
	int             new_value = ((XmScrollBarCallbackStruct *) call_data)->value;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollbarPeer_dragAbsoluteMID, new_value);
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollbarPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	int             argc;
	Arg             args[40];
	struct ComponentData *parentComponentData;
	struct ComponentData *componentData;
	Dimension       d;
	jobject         target;
	Pixel           bg;
	jint            visibleAmount;
	jobject         thisGlobalRef;

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "target is null");
		AWT_UNLOCK();
		return;
	}
	parentComponentData = (struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);
	componentData = (struct ComponentData *)XtCalloc(1, sizeof(struct ComponentData));

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) componentData);

	XtVaGetValues(parentComponentData->widget, XmNbackground, &bg, NULL);

	switch ((*env)->GetIntField(env, target, MCachedIDs.java_awt_Scrollbar_orientationFID)) {
	case java_awt_Scrollbar_HORIZONTAL:
		XtVaGetValues(parentComponentData->widget, XmNwidth, &d, NULL);

		argc = 0;
		XtSetArg(args[argc], XmNorientation, XmHORIZONTAL);
		argc++;
		break;
	case java_awt_Scrollbar_VERTICAL:
		XtVaGetValues(parentComponentData->widget, XmNheight, &d, NULL);

		argc = 0;
		XtSetArg(args[argc], XmNorientation, XmVERTICAL);
		argc++;
		break;
	default:
		XtFree((char *)componentData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, "bad scrollbar orientation");
		AWT_UNLOCK();
		return;
	}

	thisGlobalRef = componentData->peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		XtFree((char *)componentData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtSetArg(args[argc], XmNrecomputeSize, False);
	argc++;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNx, 0);
	argc++;
	XtSetArg(args[argc], XmNy, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginTop, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginBottom, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginLeft, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginRight, 0);
	argc++;
	XtSetArg(args[argc], XmNuserData, (XtPointer) thisGlobalRef);
	argc++;

	visibleAmount = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Scrollbar_visibleAmountFID);

	if (visibleAmount > 0) {
		XtSetArg(args[argc], XmNpageIncrement, (int) (*env)->GetIntField(env, target, MCachedIDs.java_awt_Scrollbar_pageIncrementFID));
		argc++;
		XtSetArg(args[argc], XmNsliderSize, (int) visibleAmount);
		argc++;
		XtSetArg(args[argc], XmNvalue, (int) (*env)->GetIntField(env, target, MCachedIDs.java_awt_Scrollbar_valueFID));
		argc++;
		XtSetArg(args[argc], XmNminimum, (int) (*env)->GetIntField(env, target, MCachedIDs.java_awt_Scrollbar_minimumFID));
		argc++;
		XtSetArg(args[argc], XmNmaximum, (int) (*env)->GetIntField(env, target, MCachedIDs.java_awt_Scrollbar_maximumFID));
		argc++;
	}
	componentData->widget = XmCreateScrollBar(parentComponentData->widget, "scrollbar", args, argc);

	/* XtSetMappedWhenManaged(componentData->widget, False); */
	XtManageChild(componentData->widget);
	XtAddCallback(componentData->widget, XmNdecrementCallback, Scrollbar_lineUp, (XtPointer) thisGlobalRef);
	XtAddCallback(componentData->widget, XmNincrementCallback, Scrollbar_lineDown, (XtPointer) thisGlobalRef);
	XtAddCallback(componentData->widget, XmNpageDecrementCallback, Scrollbar_pageUp, (XtPointer) thisGlobalRef);
	XtAddCallback(componentData->widget, XmNpageIncrementCallback, Scrollbar_pageDown, (XtPointer) thisGlobalRef);
	XtAddCallback(componentData->widget, XmNdragCallback, Scrollbar_dragAbsolute, (XtPointer) thisGlobalRef);
	XtAddCallback(componentData->widget, XmNtoTopCallback, Scrollbar_dragAbsolute, (XtPointer) thisGlobalRef);
	XtAddCallback(componentData->widget, XmNtoBottomCallback, Scrollbar_dragAbsolute, (XtPointer) thisGlobalRef);

	XtAddEventHandler(componentData->widget, ButtonReleaseMask, False, ButtonReleaseHandler, NULL);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollbarPeer_pSetValues(JNIEnv * env, jobject this,
		       jint value, jint visible, jint minimum, jint maximum)
{
	struct ComponentData *componentData;

	AWT_LOCK();

	componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	/* pass in visible for sliderSize since Motif will calculate the */
	/* slider's size for us. */
	XtVaSetValues(componentData->widget,
		      XmNminimum, minimum, XmNmaximum, maximum, XmNvalue, value, XmNsliderSize, visible, NULL);
	AWT_FLUSH_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollbarPeer_setLineIncrement(JNIEnv * env, jobject this, jint value)
{
	struct ComponentData *componentData;

	AWT_LOCK();

	componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(componentData->widget, XmNincrement, value, NULL);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollbarPeer_setPageIncrement(JNIEnv * env, jobject this, jint value)
{
	struct ComponentData *componentData;

	AWT_LOCK();

	componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(componentData->widget, XmNpageIncrement, value, NULL);
	AWT_FLUSH_UNLOCK();
}
