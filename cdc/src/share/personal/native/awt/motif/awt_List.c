/*
 * @(#)awt_List.c	1.68 06/10/10
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
#include "java_awt_List.h"
#include "java_awt_AWTEvent.h"
#include "sun_awt_motif_MListPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "canvas.h"
#include "jni.h"
#include "awt.h"
#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MListPeer_actionMID = (*env)->GetMethodID(env, cls, "action", "(I)V");
	MCachedIDs.MListPeer_handleListChangedMID = (*env)->GetMethodID(env, cls, "handleListChanged", "(I)V");
}

static void
Slist_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	switch (cbs->reason) {
	case XmCR_DEFAULT_ACTION:
		(*env)->CallVoidMethod(env, this, MCachedIDs.MListPeer_actionMID, (cbs->item_position - 1));
		break;

	case XmCR_BROWSE_SELECT:
		(*env)->CallVoidMethod(env, this, MCachedIDs.MListPeer_handleListChangedMID, (cbs->item_position - 1));
		break;

	case XmCR_MULTIPLE_SELECT:
		(*env)->CallVoidMethod(env, this, MCachedIDs.MListPeer_handleListChangedMID, (cbs->item_position - 1));
		break;

	default:
		break;
	}

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    create Signature:
 * (Lsun/awt/motif/MComponentPeer;)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	int             argc;
	Arg             args[40];
	struct ComponentData *parentComponentData;
	struct ListData *listData;
	Pixel           bg;
	jobject         thisGlobalRef;

	AWT_LOCK();

	if (parent == NULL ||
	    (parentComponentData = (struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID)) == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	listData = (struct ListData *) XtCalloc(1, sizeof(struct ListData));

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) listData);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	thisGlobalRef = listData->comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		XtFree((char *)listData);
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(parentComponentData->widget, XmNbackground, &bg, NULL);

	argc = 0;

	/*
	XtSetArg(args[argc], XmNrecomputeSize, False);
	argc++;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNlistSizePolicy, XmCONSTANT);
	argc++;
	XtSetArg(args[argc], XmNx, 0);
	argc++;
	XtSetArg(args[argc], XmNy, 0);
	argc++;
	*/


	XtSetArg(args[argc], XmNmarginTop, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginBottom, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginLeft, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginRight, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;


	/*
	XtSetArg(args[argc], XmNlistMarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNlistMarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNscrolledWindowMarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNscrolledWindowMarginHeight, 0);
	argc++;
	*/

	XtSetArg(args[argc], XmNuserData, (XtPointer) thisGlobalRef);
	argc++;

	listData->list = XmCreateScrolledList(parentComponentData->widget,
					      "slist:",
					      args,
					      argc);


	listData->comp.widget = XtParent(listData->list);
	/* XtSetMappedWhenManaged(listData->comp.widget, False); */

	XtAddCallback(listData->list,
		      XmNdefaultActionCallback,
		      Slist_callback,
		      (XtPointer) thisGlobalRef);

	/*
	 * XtAddEventHandler(listData->list, FocusChangeMask, True,
	 * awt_canvas_handleEvent, thisGlobalRef);
	 * 
	 * awt_addWidget(env, listData->list, listData->comp.widget,
	 * thisGlobalRef, java_awt_AWTEvent_KEY_EVENT_MASK |
	 * java_awt_AWTEvent_MOUSE_EVENT_MASK |
	 * java_awt_AWTEvent_MOUSE_MOTION_EVENT_MASK);
	 */

	XtManageChild(listData->list);
	XtManageChild(listData->comp.widget);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_pDispose(JNIEnv * env, jobject this)
{
	struct ListData *listData;

	AWT_LOCK();

	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData != NULL)
		awt_delWidget(listData->list);

	AWT_UNLOCK();

	Java_sun_awt_motif_MComponentPeer_pDispose(env, this);
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    setMultipleSelections
 * Signature: (Z)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_setMultipleSelections(JNIEnv * env, jobject this, jboolean multipleSelections)
{
	struct ListData *listData;
	jobject         globalThis;

	AWT_LOCK();

	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(listData->list, XmNuserData, &globalThis, NULL);

	if (multipleSelections == JNI_FALSE) {
		XtVaSetValues(listData->list,
			      XmNselectionPolicy, XmBROWSE_SELECT,
			      NULL);
		XtRemoveCallback(listData->list,
				 XmNmultipleSelectionCallback,
				 Slist_callback,
				 (XtPointer) globalThis);
		XtAddCallback(listData->list,
			      XmNbrowseSelectionCallback,
			      Slist_callback,
			      (XtPointer) globalThis);
	} else {
		XtVaSetValues(listData->list,
			      XmNselectionPolicy, XmMULTIPLE_SELECT,
			      NULL);
		XtRemoveCallback(listData->list,
				 XmNbrowseSelectionCallback,
				 Slist_callback,
				 (XtPointer) globalThis);
		XtAddCallback(listData->list,
			      XmNmultipleSelectionCallback,
			      Slist_callback,
			      (XtPointer) globalThis);
	}
	AWT_UNLOCK();
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    isSelected Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_motif_MListPeer_isSelected(JNIEnv * env, jobject this, jint pos)
{
	struct ListData *listData;
	jboolean        selected;

	AWT_LOCK();
	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return JNI_FALSE;
	}
	pos++;

	selected = (XmListPosSelected(listData->list, pos) == True) ? JNI_TRUE : JNI_FALSE;
	AWT_UNLOCK();
	return selected;
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    addItem Signature:
 * (Ljava/lang/String;I)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_paddItem(JNIEnv * env, jobject this, jbyteArray item, jint index)
{
	XmString        xim;
	struct ListData *listData;
	jobject         target;
	jobject         font;
	/* TODO: check the need of these variables
           jint            width, height;
        */
	jbyte          *bytereg;
	int             blen;

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (item == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return;
	}
	/* motif uses 1-based indeces for the list operations with 0 */
	/* referring to the last item on the list. Thus if index is -1 */
	/* then we'll get the right effect of adding to the end of the */
	/* list. */
	index++;

	blen = (*env)->GetArrayLength(env, item);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, item, 0, blen, bytereg);
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree((char *)bytereg);

	XmListAddItemUnselected(listData->list, xim, index);

	if (xim != NULL) {
		XmStringFree(xim);
	}



#if 0
	width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
	height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

	/*
	 * It is necessary to change the size then change it back, because *
	 * otherwise we can't guarantee that the gadget's representation on *
	 * the screen will be properly re-calculated.
	 */
	XtVaSetValues(listData->comp.widget,
		      XmNwidth, (width > 1) ? width - 1 : 1,
		      XmNheight, (height > 1) ? height - 1 : 1,
		      NULL);
	XtVaSetValues(listData->comp.widget,
		      XmNwidth, (width > 0) ? width : 1,
		      XmNheight, (height > 0) ? height : 1,
		      NULL);

#endif

	AWT_UNLOCK();
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    delItems Signature: (II)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_delItems(JNIEnv * env, jobject this, jint start, jint end)
{
	struct ListData *listData;
	jobject         target;
        /* TODO: check the need of these variables
          jint            width, height;
        */

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	start++;
	end++;

	if (start == end)
		XmListDeletePos(listData->list, start);

	else
		XmListDeleteItemsPos(listData->list, end - start + 1, start);


#if 0
	width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
	height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

	/*
	 * It is necessary to change the size then change it back, because *
	 * otherwise we can't guarantee that the gadget's representation on *
	 * the screen will be properly re-calculated.
	 */
	XtVaSetValues(listData->comp.widget,
		      XmNwidth, (width > 1) ? width - 1 : 1,
		      XmNheight, (height > 1) ? height - 1 : 1,
		      NULL);
	XtVaSetValues(listData->comp.widget,
		      XmNwidth, (width > 0) ? width : 1,
		      XmNheight, (height > 0) ? height : 1,
		      NULL);

#endif

	AWT_UNLOCK();
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    select Signature: (I)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_select(JNIEnv * env, jobject this, jint pos)
{
	struct ListData *listData;

	AWT_LOCK();
	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	pos++;
	XmListSelectPos(listData->list, pos, 0);
	AWT_UNLOCK();
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    deselect Signature: (I)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_deselect(JNIEnv * env, jobject this, jint pos)
{
	struct ListData *listData;

	AWT_LOCK();
	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	pos++;
	XmListDeselectPos(listData->list, pos);
	AWT_UNLOCK();
}

/*
 * Class:     sun_awt_motif_MListPeer Method:    makeVisible Signature: (I)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MListPeer_makeVisible(JNIEnv * env, jobject this, jint pos)
{
	int             top, visible;
	struct ListData *listData;

	AWT_LOCK();
	listData = (struct ListData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (listData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(listData->list,
		      XmNtopItemPosition, &top,
		      XmNvisibleItemCount, &visible,
		      NULL);

	pos++;

	if (pos < top)
		XmListSetPos(listData->list, pos);

	else
		XmListSetBottomPos(listData->list, pos);

	AWT_UNLOCK();
}
