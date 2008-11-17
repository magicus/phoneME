/*
 * @(#)awt_PopupMenu.c	1.42 06/10/10
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
#include <Xm/Separator.h>
#include <Xm/MenuUtilP.h>
#include "color.h"
#include "java_awt_PopupMenu.h"
#include "java_awt_Component.h"
#include "java_awt_AWTEvent.h"
#include "sun_awt_motif_MPopupMenuPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "awt_Font.h"
#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MPopupMenuPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MPopupMenuPeer_destroyNativeWidgetAfterGettingTreeLockMID =
	(*env)->GetMethodID(env, cls, "destroyNativeWidgetAfterGettingTreeLock", "()V");
}

/*
 * BUG-4045826 Item event is not generated if checkbox menuitem in popup menu
 * is clicked Replaced the callback function Frame_callbackCB() with this
 * popdown_event_handler() since Frame_callbackCB() is always invoked before
 * the callback function on the MenuItem.
 */
static void
popdown_event_handler(Widget w, XtPointer client_data, XEvent * event, Boolean * cont)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if (w->core.being_destroyed) {
		return;
	}
	switch (event->xany.type) {
	case UnmapNotify:
		(*env)->CallVoidMethod(env, this, MCachedIDs.MPopupMenuPeer_destroyNativeWidgetAfterGettingTreeLockMID);
		break;
	default:
		break;
	}

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);
}

/*
 * Class:     sun_awt_motif_MPopupMenuPeer Method:    createMenu Signature:
 * (Lsun/awt/motif/MComponentPeer;)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MPopupMenuPeer_createMenu(JNIEnv * env, jobject this, jobject parent)
{
	struct ComponentData *parentComponentData;
	struct MenuData *menuData;
	int             argc;
	Arg             args[10];
	Pixel           bg, fg;

	XmFontList      fontList;
	XmString        xim;

	jobject         target, font;
	jobject         globalThis;
	jboolean        tearOff;
	jstring         label;

	jbyteArray      bytearray;
	jbyte          *bytereg;

	int             blen;

	AWT_LOCK();

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	parentComponentData =
		(struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MMenuItemPeer_targetFID);

	if (parentComponentData == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_MenuComponent_getFontMID);
	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return;
	}
	fontList = getFontList(env, font);

	menuData = (struct MenuData *)XtCalloc(1, sizeof(struct MenuData));

	if (menuData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID, (jint) menuData);

	label = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_MenuItem_labelFID);

	bytearray = (*env)->CallObjectMethod(env, label, MCachedIDs.java_lang_String_getBytesMID);
	blen = (*env)->GetArrayLength(env, bytearray);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, bytearray, 0, blen, bytereg);
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree((char *)bytereg);

	XtVaGetValues(parentComponentData->widget, XmNbackground, &bg, NULL);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;

	tearOff = (*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Menu_tearOffFID);

	if (tearOff == JNI_TRUE) {
		XtSetArg(args[argc], XmNtearOffModel, XmTEAR_OFF_ENABLED);
		argc++;
	}
	globalThis = (*env)->NewGlobalRef(env, this);

	XtSetArg(args[argc], XmNuserData, (XtPointer) globalThis);
	argc++;
	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;
	if (fontList != NULL) {
		XtSetArg(args[argc], XmNfontList, fontList);
		argc++;
	}
	menuData->itemData.comp.widget = XmCreatePopupMenu(parentComponentData->widget, "popup:", args, argc);

	if (XmStringLength(xim) > 0) {
		Widget          labelWidget;

		labelWidget = XtVaCreateManagedWidget("poplbl:",
						      xmLabelWidgetClass,
					     menuData->itemData.comp.widget,
						      XmNlabelString, xim,
						      XmNbackground, bg,
						      NULL);


		if(fontList!=NULL) XtVaSetValues(labelWidget, XmNfontList, fontList, NULL);

		/* Create separator */
		XtVaCreateManagedWidget("popsep:",
					xmSeparatorWidgetClass,
		   menuData->itemData.comp.widget, XmNbackground, bg, NULL);

		XmChangeColor(labelWidget, bg);
	}
	if (tearOff == JNI_TRUE) {
		Widget          tearOffWidget = XmGetTearOffControl(menuData->itemData.comp.widget);

		fg = AwtColorMatch(0, 0, 0);
		XtVaSetValues(tearOffWidget, XmNbackground, bg, XmNforeground, fg, XmNhighlightColor, fg, NULL);
	}
	menuData->comp.widget = menuData->itemData.comp.widget;
	XtAddEventHandler(XtParent(menuData->comp.widget), StructureNotifyMask, FALSE, popdown_event_handler,
			  (XtPointer) globalThis);

	XtSetSensitive(menuData->comp.widget, (*env)->GetBooleanField(env, target, MCachedIDs.java_awt_MenuItem_enabledFID) == JNI_TRUE ? True : False);

	if (xim != NULL) {
		XmStringFree(xim);
	}
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MPopupMenuPeer_pShow(JNIEnv * env, jobject this, jobject origin,
			      jint x, jint y)
{
	struct MenuData *menuData;
	struct ComponentData *originComponentData;
	XButtonEvent    newButtonEvent;
	int             rx, ry;
	Window          root, win;

	AWT_LOCK();

	menuData = (struct MenuData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (menuData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	originComponentData =
		(struct ComponentData *) (*env)->GetIntField(env, origin, MCachedIDs.MComponentPeer_pDataFID);;

	if (!XtIsRealized(originComponentData->widget)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_InternalErrorClass, "widget not visible on screen");
		AWT_UNLOCK();
		return;
	}
	/*
	 * Fix for BugTraq ID 4186663 - Pural PopupMenus appear at the same
	 * time. If ahother popup is currently visible hide it.
	 */
	if (originComponentData->activePopup != NULL &&
	    originComponentData->activePopup != menuData->comp.widget &&
	    XtIsObject(originComponentData->activePopup) && XtIsManaged(originComponentData->activePopup)) {
		XtUnmanageChild(originComponentData->activePopup);
	}

	root = RootWindowOfScreen(XtScreen(originComponentData->widget));
	XTranslateCoordinates(awt_display, XtWindow(originComponentData->widget), root, (int) x, (int) y, &rx, &ry, &win);

	newButtonEvent.type = ButtonPress;
	newButtonEvent.display = awt_display;
	newButtonEvent.window = XtWindow(originComponentData->widget);
	newButtonEvent.x = (int) x;
	newButtonEvent.y = (int) y;
	newButtonEvent.x_root = rx;
	newButtonEvent.y_root = ry;

	XmMenuPosition(menuData->comp.widget, &newButtonEvent);
	XtManageChild(menuData->comp.widget);

	originComponentData->activePopup = menuData->comp.widget;

	AWT_FLUSH_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MPopupMenuPeer_pDispose(JNIEnv * env, jobject this)
{
	struct MenuData *menuData;
	jobject         globalThis;

	AWT_LOCK();

	menuData = (struct MenuData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (menuData == NULL) {
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(menuData->comp.widget, XmNuserData, &globalThis, NULL);

	if (globalThis != NULL)
		(*env)->DeleteGlobalRef(env, globalThis);

	XtUnmanageChild(menuData->comp.widget);
	awt_util_consumeAllXEvents(menuData->comp.widget);
	XtDestroyWidget(menuData->comp.widget);
	XtFree((void *) menuData);

	AWT_UNLOCK();
}
