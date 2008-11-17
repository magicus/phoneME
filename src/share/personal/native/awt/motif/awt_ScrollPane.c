/*
 * @(#)awt_ScrollPane.c	1.38 06/10/10
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
#include "java_awt_Insets.h"
#include "java_awt_ScrollPane.h"
#include "java_awt_Adjustable.h"
#include "java_awt_event_AdjustmentEvent.h"
#include "sun_awt_motif_MScrollPanePeer.h"
#include "sun_awt_motif_MComponentPeer.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollPanePeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MScrollPanePeer_dragInProgressFID = (*env)->GetFieldID(env, cls, "dragInProgress", "Z");
	MCachedIDs.MScrollPanePeer_scrolledHorizontalMID =
		(*env)->GetMethodID(env, cls, "scrolledHorizontal", "(II)V");
	MCachedIDs.MScrollPanePeer_scrolledVerticalMID =
		(*env)->GetMethodID(env, cls, "scrolledVertical", "(II)V");

	cls = (*env)->FindClass(env, "java/awt/ScrollPane");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID =
		(*env)->GetFieldID(env, cls, "scrollbarDisplayPolicy", "I");
}

static int
getScrollType(int reason)
{
	switch (reason) {
		case XmCR_DRAG:
		case XmCR_VALUE_CHANGED:
		case XmCR_TO_TOP:
		case XmCR_TO_BOTTOM:
		return java_awt_event_AdjustmentEvent_TRACK;
	case XmCR_PAGE_INCREMENT:
		return java_awt_event_AdjustmentEvent_BLOCK_INCREMENT;
	case XmCR_PAGE_DECREMENT:
		return java_awt_event_AdjustmentEvent_BLOCK_DECREMENT;
	case XmCR_INCREMENT:
		return java_awt_event_AdjustmentEvent_UNIT_INCREMENT;
	case XmCR_DECREMENT:
		return java_awt_event_AdjustmentEvent_UNIT_DECREMENT;
	default:
		return 0;
	}
}


static void
ScrollPane_scrollV(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScrollBarCallbackStruct *scrolldata = (XmScrollBarCallbackStruct *) call_data;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if (scrolldata->reason == XmCR_DRAG) {
		(*env)->SetBooleanField(env, this, MCachedIDs.MScrollPanePeer_dragInProgressFID, JNI_TRUE);
	}
	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollPanePeer_scrolledVerticalMID,
		      scrolldata->value, getScrollType(scrolldata->reason));

	if (scrolldata->reason == XmCR_VALUE_CHANGED) {
		(*env)->SetBooleanField(env, this, MCachedIDs.MScrollPanePeer_dragInProgressFID, JNI_FALSE);
	}
}

static void
ScrollPane_scrollH(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScrollBarCallbackStruct *scrolldata = (XmScrollBarCallbackStruct *) call_data;
	jobject         this = (jobject) client_data;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if (scrolldata->reason == XmCR_DRAG) {
		(*env)->SetBooleanField(env, this, MCachedIDs.MScrollPanePeer_dragInProgressFID, JNI_TRUE);
	}
	(*env)->CallVoidMethod(env, this, MCachedIDs.MScrollPanePeer_scrolledHorizontalMID,
		      scrolldata->value, getScrollType(scrolldata->reason));

	if (scrolldata->reason == XmCR_VALUE_CHANGED) {
		(*env)->SetBooleanField(env, this, MCachedIDs.MScrollPanePeer_dragInProgressFID, JNI_FALSE);
	}
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollPanePeer_create(JNIEnv * env, jobject this, jobject parent)
{
	int             argc;
	Arg             args[40];
	struct ComponentData *parentComponentData;
	struct ComponentData *componentData;
	jobject         target;
	Pixel           bg;
	Widget          vsb, hsb;
	jint            sbDisplay;
	jobject         thisGlobalRef;

	AWT_LOCK();

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	parentComponentData =
		(struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	componentData = (struct ComponentData *)XtCalloc(1, sizeof(struct ComponentData));

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) componentData);

	XtVaGetValues(parentComponentData->widget, XmNbackground, &bg, NULL);
	argc = 0;

	sbDisplay = (*env)->GetIntField(env, target, MCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID);

	thisGlobalRef = componentData->peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		XtFree((char *)componentData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtSetArg(args[argc], XmNuserData, (XtPointer) thisGlobalRef);
	argc++;

	if (sbDisplay == java_awt_ScrollPane_SCROLLBARS_NEVER) {
		componentData->widget = XtCreateWidget("ScrolledWindowClipWindow",
		    xmManagerWidgetClass, parentComponentData->widget, args,
						       argc);

	} else {
		if (sbDisplay == java_awt_ScrollPane_SCROLLBARS_ALWAYS) {
			XtSetArg(args[argc], XmNscrollBarDisplayPolicy, XmSTATIC);
			argc++;
		} else {
			XtSetArg(args[argc], XmNscrollBarDisplayPolicy, XmAS_NEEDED);
			argc++;
		}

		XtSetArg(args[argc], XmNscrollingPolicy, XmAUTOMATIC);
		argc++;
		XtSetArg(args[argc], XmNvisualPolicy, XmCONSTANT);
		argc++;
		XtSetArg(args[argc], XmNspacing, 0);
		argc++;

		componentData->widget = XmCreateScrolledWindow(parentComponentData->widget, "scroller", args, argc);

		XtVaGetValues(componentData->widget, XmNverticalScrollBar, &vsb, XmNhorizontalScrollBar, &hsb, NULL);

		if (vsb != NULL) {
			XtAddCallback(vsb, XmNincrementCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNdecrementCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNpageIncrementCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNpageDecrementCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNtoTopCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNtoBottomCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNvalueChangedCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);
			XtAddCallback(vsb, XmNdragCallback, ScrollPane_scrollV, (XtPointer) thisGlobalRef);

			XtVaSetValues(vsb, XmNhighlightThickness, 0, NULL);

		}
		if (hsb != NULL) {
			XtAddCallback(hsb, XmNincrementCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNdecrementCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNpageIncrementCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNpageDecrementCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNtoTopCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNtoBottomCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNvalueChangedCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);
			XtAddCallback(hsb, XmNdragCallback, ScrollPane_scrollH, (XtPointer) thisGlobalRef);

			XtVaSetValues(hsb, XmNhighlightThickness, 0, NULL);
		}
	}

	/* XtSetMappedWhenManaged(componentData->widget, False); */
	XtManageChild(componentData->widget);

	AWT_UNLOCK();
}


JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MScrollPanePeer_pGetScrollbarSpace(JNIEnv * env, jobject this, jint orient)
{
	struct ComponentData *componentData;
	Widget          scrollbar;
	Dimension       thickness = 0;
	Dimension       space = 0;
	Dimension       highlight = 0;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL || componentData->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	if (orient == java_awt_Adjustable_VERTICAL) {

		XtVaGetValues(componentData->widget, XmNverticalScrollBar, &scrollbar, XmNspacing, &space, NULL);
		XtVaGetValues(scrollbar, XmNwidth, &thickness, XmNhighlightThickness, &highlight, NULL);
	} else {

		XtVaGetValues(componentData->widget, XmNhorizontalScrollBar, &scrollbar, XmNspacing, &space, NULL);
		XtVaGetValues(scrollbar, XmNheight, &thickness, XmNhighlightThickness, &highlight, NULL);
	}

	AWT_UNLOCK();
	return (long) (thickness + space + 2 * highlight);
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MScrollPanePeer_pGetBlockIncrement(JNIEnv * env, jobject this, jint orient)
{
	struct ComponentData *componentData;
	Widget          scrollbar;
	int             pageIncr = 0;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL || componentData->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	if (orient == java_awt_Adjustable_VERTICAL) {
		XtVaGetValues(componentData->widget, XmNverticalScrollBar, &scrollbar, NULL);
		XtVaGetValues(scrollbar, XmNpageIncrement, &pageIncr, NULL);
	} else {
		XtVaGetValues(componentData->widget, XmNhorizontalScrollBar, &scrollbar, NULL);
		XtVaGetValues(scrollbar, XmNpageIncrement, &pageIncr, NULL);
	}

	AWT_UNLOCK();
	return (long) (pageIncr);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_motif_MScrollPanePeer_pInsets(JNIEnv * env, jobject this,
		 jint width, jint height, jint childWidth, jint childHeight)
{
	struct ComponentData *componentData;
	jobject         target;
	jobject         insets;
	Widget          hsb, vsb;
	Dimension       hsbThickness, hsbHighlight, hsbSpace = 0, vsbThickness,
	                vsbHighlight, vsbSpace = 0, space, border, shadow,
	                hMargin, vMargin;
	unsigned char   placement;
	Boolean         hsbVisible, vsbVisible;
	long            sbDisplay;
	long            top, left, right, bottom;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL || componentData == NULL || componentData->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	sbDisplay = (*env)->GetIntField(env, target, MCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID);

	/*
	 * TODO: investigate caching these values rather than querying for
	 * them each time.
	 */

	if (sbDisplay == java_awt_ScrollPane_SCROLLBARS_NEVER) {
		XtVaGetValues(componentData->widget, XmNshadowThickness, &shadow, NULL);
		space = border = hMargin = vMargin = 0;
	} else {
		XtVaGetValues(componentData->widget,
			      XmNverticalScrollBar, &vsb,
			      XmNhorizontalScrollBar, &hsb,
			      XmNscrollBarPlacement, &placement,
			      XmNspacing, &space,
			      XmNshadowThickness, &shadow,
			      XmNscrolledWindowMarginHeight, &vMargin,
			      XmNscrolledWindowMarginWidth, &hMargin, XmNborderWidth, &border, NULL);

		XtVaGetValues(vsb, XmNwidth, &vsbThickness, XmNhighlightThickness, &vsbHighlight, NULL);

		XtVaGetValues(hsb, XmNheight, &hsbThickness, XmNhighlightThickness, &hsbHighlight, NULL);

		hsbSpace = hsbThickness + space + hsbHighlight;
		vsbSpace = vsbThickness + space + vsbHighlight;

		/*
		 * XtVaGetValues(clip, XmNwidth, &clipw, XmNheight, &cliph,
		 * XmNx, &clipx, XmNy, &clipy, NULL); printf("insets:
		 * spacing=%d shadow=%d swMarginH=%d swMarginW=%d border=%d ;
		 * \ vsb=%d vsbHL=%d ; hsb=%d hsbHL=%d ; %dx%d ->clip=%d,%d
		 * %dx%d\n", space, shadow, vMargin, hMargin, border,
		 * vsbThickness, vsbHighlight, hsbThickness, hsbHighlight, w,
		 * h, clipx, clipy, clipw, cliph);
		 */


	}

	/*
	 * We unfortunately have to use the size parameters to determine
	 * whether or not "as needed" scrollbars are currently present or not
	 * because we can't necessarily rely on getting valid geometry values
	 * straight from the Motif widgets until they are mapped. :(
	 */
	switch (sbDisplay) {
	case java_awt_ScrollPane_SCROLLBARS_NEVER:
		vsbVisible = hsbVisible = FALSE;
		break;
	case java_awt_ScrollPane_SCROLLBARS_ALWAYS:
		vsbVisible = hsbVisible = TRUE;
		break;
	case java_awt_ScrollPane_SCROLLBARS_AS_NEEDED:
	default:
		vsbVisible = hsbVisible = FALSE;
		if (childWidth > width - 2 * shadow) {
			hsbVisible = TRUE;
		}
		if (childHeight > height - 2 * shadow) {
			vsbVisible = TRUE;
		}
		if (!hsbVisible && vsbVisible && childWidth > width - 2 * shadow - vsbSpace) {
			hsbVisible = TRUE;
		} else if (!vsbVisible && hsbVisible && childHeight > height - 2 * shadow - hsbSpace) {
			vsbVisible = TRUE;
		}
	}

	top = bottom = shadow + vMargin;
	left = right = shadow + hMargin;

	if (sbDisplay != java_awt_ScrollPane_SCROLLBARS_NEVER) {
		switch (placement) {
		case XmBOTTOM_RIGHT:
			bottom += (hsbVisible ? hsbSpace : (vsbVisible ? vsbHighlight : 0));
			right += (vsbVisible ? vsbSpace : (hsbVisible ? hsbHighlight : 0));
			top += (vsbVisible ? vsbHighlight : 0);
			left += (hsbVisible ? hsbHighlight : 0);
			break;
		case XmBOTTOM_LEFT:
			bottom += (hsbVisible ? hsbSpace : (vsbVisible ? vsbHighlight : 0));
			left += (vsbVisible ? hsbSpace : (hsbVisible ? hsbHighlight : 0));
			top += (vsbVisible ? vsbHighlight : 0);
			right += (hsbVisible ? hsbHighlight : 0);
			break;
		case XmTOP_RIGHT:
			top += (hsbVisible ? hsbSpace : (vsbVisible ? vsbHighlight : 0));
			right += (vsbVisible ? vsbSpace : (hsbVisible ? hsbHighlight : 0));
			bottom += (vsbVisible ? vsbHighlight : 0);
			left += (hsbVisible ? hsbHighlight : 0);
			break;
		case XmTOP_LEFT:
			top += (hsbVisible ? hsbSpace : (vsbVisible ? vsbHighlight : 0));
			left += (vsbVisible ? vsbSpace : (hsbVisible ? hsbHighlight : 0));
			bottom += (vsbVisible ? vsbHighlight : 0);
			right += (hsbVisible ? hsbHighlight : 0);
		}
	}
	/*
	 * Deadlock prevention: don't hold the toolkit lock while invoking
	 * constructor.
	 */
	AWT_UNLOCK();
	insets =
		(*env)->NewObject(env, MCachedIDs.java_awt_InsetsClass, MCachedIDs.java_awt_Insets_constructorMID,
				  top, left, bottom, right);
	if (insets == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "insets constructor failed");
	}
	return insets;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollPanePeer_pSetScrollChild(JNIEnv * env, jobject this, jobject child)
{
	struct ComponentData *childComponentData;
	struct ComponentData *componentData;
	jobject         target;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	childComponentData =
		(struct ComponentData *) (*env)->GetIntField(env, child, MCachedIDs.MComponentPeer_pDataFID);

	if (child == NULL ||
	    target == NULL ||
	    componentData == NULL ||
	    componentData->widget == NULL || childComponentData == NULL || childComponentData->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if ((*env)->GetIntField(env, target, MCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID) ==
	    java_awt_ScrollPane_SCROLLBARS_NEVER) {


	} else {

		XmScrolledWindowSetAreas(componentData->widget, NULL, NULL, childComponentData->widget);
		/*
		 * XtInsertEventHandler(childComponentData->widget,
		 * StructureNotifyMask, FALSE, child_event_handler,
		 * componentData->widget, XtListHead);
		 */
	}

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollPanePeer_pSetIncrement(JNIEnv * env, jobject this,
				      jint orient, jint incrType, jint incr)
{
	struct ComponentData *componentData;
	Widget          scrollbar = NULL;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL || componentData->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (!XtIsSubclass(componentData->widget, xmScrolledWindowWidgetClass)) {
		AWT_UNLOCK();
		return;
	}
	if (orient == java_awt_Adjustable_VERTICAL) {
		XtVaGetValues(componentData->widget, XmNverticalScrollBar, &scrollbar, NULL);
	} else {
		XtVaGetValues(componentData->widget, XmNhorizontalScrollBar, &scrollbar, NULL);
	}

	if (scrollbar != NULL) {
		if (incrType == sun_awt_motif_MScrollPanePeer_UNIT_INCREMENT) {
			XtVaSetValues(scrollbar, XmNincrement, (XtArgVal) incr, NULL);
		} else {	/* BLOCK_INCREMENT */
			XtVaSetValues(scrollbar, XmNpageIncrement, (XtArgVal) incr, NULL);
		}
	}
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MScrollPanePeer_setScrollPosition(JNIEnv * env, jobject this, jint x, jint y)
{
	struct ComponentData *componentData;
	jobject         target;
	Widget          hsb, vsb;
	int             size, incr, pIncr;

	AWT_LOCK();

	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL || componentData == NULL || componentData->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if ((*env)->GetIntField(env, target, MCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID) ==
	    java_awt_ScrollPane_SCROLLBARS_NEVER) {
		WidgetList      children;
		int             numChildren;

		XtVaGetValues(componentData->widget, XmNchildren, &children, XmNnumChildren, &numChildren, NULL);

		if (numChildren < 1) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
			AWT_UNLOCK();
			return;
		}
		XtMoveWidget(children[0], (int) -x, (int) -y);
	} else {
		XtVaGetValues(componentData->widget, XmNhorizontalScrollBar, &hsb, XmNverticalScrollBar, &vsb, NULL);

		if (vsb) {
			XtVaGetValues(vsb, XmNincrement, &incr, XmNpageIncrement, &pIncr, XmNsliderSize, &size, NULL);
			XmScrollBarSetValues(vsb, (int) y, size, incr, pIncr, TRUE);
		}
		if (hsb) {
			XtVaGetValues(hsb, XmNincrement, &incr, XmNpageIncrement, &pIncr, XmNsliderSize, &size, NULL);
			XmScrollBarSetValues(hsb, (int) x, size, incr, pIncr, TRUE);
		}
	}

	AWT_FLUSH_UNLOCK();
}
