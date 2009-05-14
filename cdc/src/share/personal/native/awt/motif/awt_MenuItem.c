/*
 * @(#)awt_MenuItem.c	1.58 06/10/10
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
#include "java_awt_MenuItem.h"
#include "sun_awt_motif_MMenuItemPeer.h"
#include "sun_awt_motif_MCheckboxMenuItemPeer.h"
#include "java_awt_Menu.h"
#include "sun_awt_motif_MMenuPeer.h"
#include "awt_Font.h"
#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuItemPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MMenuItemPeer_isCheckboxFID = (*env)->GetFieldID(env, cls, "isCheckbox", "Z");
	MCachedIDs.MMenuItemPeer_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.MMenuItemPeer_targetFID = (*env)->GetFieldID(env, cls, "target", "Ljava/awt/MenuItem;");
	MCachedIDs.MMenuItemPeer_actionMID = (*env)->GetMethodID(env, cls, "action", "(JI)V");

	cls = (*env)->FindClass(env, "sun/awt/motif/MCheckboxMenuItemPeer");

	if (cls == NULL)
		return;

	MCachedIDs.MCheckboxMenuItemPeer_actionMID = (*env)->GetMethodID(env, cls, "action", "(JIZ)V");

	cls = (*env)->FindClass(env, "java/awt/MenuComponent");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_MenuComponent_getFontMID =
		(*env)->GetMethodID(env, cls, "getFont", "()Ljava/awt/Font;");

	cls = (*env)->FindClass(env, "java/awt/MenuItem");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_MenuItem_labelFID = (*env)->GetFieldID(env, cls, "label", "Ljava/lang/String;");
	MCachedIDs.java_awt_MenuItem_enabledFID = (*env)->GetFieldID(env, cls, "enabled", "Z");
	MCachedIDs.java_awt_MenuItem_shortcutFID =
		(*env)->GetFieldID(env, cls, "shortcut", "Ljava/awt/MenuShortcut;");

	cls = (*env)->FindClass(env, "java/awt/Menu");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Menu_isHelpMenuFID = (*env)->GetFieldID(env, cls, "isHelpMenu", "Z");
	MCachedIDs.java_awt_Menu_tearOffFID = (*env)->GetFieldID(env, cls, "tearOff", "Z");
}

static void
MenuItem_selected(Widget w, XtPointer client_data, XmAnyCallbackStruct * s)
{
	extern int      getModifiers(int state);
	jint            modifiers = (jint) getModifiers(s->event->xbutton.state);
	jlong           when = (jlong) s->event->xbutton.time;
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if ((*env)->GetBooleanField(env, this, MCachedIDs.MMenuItemPeer_isCheckboxFID) == JNI_TRUE) {
		Boolean         state;
		struct MenuItemData *mdata =

		(struct MenuItemData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

		if (mdata != NULL) {
			XtVaGetValues(mdata->comp.widget, XmNset, &state, NULL);

			(*env)->CallVoidMethod(env, this, MCachedIDs.MCheckboxMenuItemPeer_actionMID, when,
				   modifiers, state ? JNI_TRUE : JNI_FALSE);
		}
	} else
		(*env)->CallVoidMethod(env, this, MCachedIDs.MMenuItemPeer_actionMID, when, modifiers);

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuItemPeer_createMenuItem(JNIEnv * env, jobject this, jobject parent)
{
	int             argc = 0;
	Arg             args[10];
	struct MenuData *menuData = NULL;
	struct MenuItemData *menuItemData = NULL;
	struct FontData *fontData = NULL;
	Pixel           bg;
	jobject         target = NULL;
	XmString        xim = NULL;
	XmString        ximsc = NULL;
	jobject         menuShortcut = NULL;
	jboolean        enabled;
	jobject         globalThis;
	jstring         label;
	jbyteArray      bytearray;
	jbyte          *bytereg;
	jboolean        isCheckBox;

	int             blen;

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, "parent is null");
		return;
	}
	AWT_LOCK();
	target = (*env)->GetObjectField(env, this, MCachedIDs.MMenuItemPeer_targetFID);
	menuData = (struct MenuData *) (*env)->GetIntField(env, parent, MCachedIDs.MMenuItemPeer_pDataFID);
	if (menuData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	menuItemData = (struct MenuItemData *)XtCalloc(1, sizeof(struct MenuItemData));
	if (menuItemData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID, (jint) menuItemData);

	label = (jstring) (*env)->GetObjectField(env, target, MCachedIDs.java_awt_MenuItem_labelFID);
	bytearray = (jbyteArray) (*env)->CallObjectMethod(env, label, MCachedIDs.java_lang_String_getBytesMID);
	blen = (*env)->GetArrayLength(env, bytearray);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, bytearray, 0, blen, bytereg);
	if (bytereg[0] == '-') {
		menuItemData->comp.widget = XmCreateSeparator(menuData->itemData.comp.widget, "mnusep:", args, argc);
		XtFree((char *)bytereg);
		return;
	}
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree((char *)bytereg);

	XtSetArg(args[argc], XmNlabelString, xim);
	argc++;

	globalThis = (*env)->NewGlobalRef(env, this);
	XtSetArg(args[argc], XmNuserData, (XtPointer) globalThis);
	argc++;

	menuShortcut = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_MenuItem_shortcutFID);
	if (menuShortcut != NULL) {
		jstring         shortcutText;

		shortcutText = (jstring)
			(*env)->CallObjectMethod(env, menuShortcut, MCachedIDs.java_lang_Object_toStringMID);

		/*
		 * If we can't get the shortcut text then don't set the
		 * shortcut text on the widget.
		 */

		if ((*env)->ExceptionCheck(env))
			(*env)->ExceptionDescribe(env);

		else {
			bytearray = (*env)->CallObjectMethod(env, shortcutText, MCachedIDs.java_lang_String_getBytesMID);
			blen = (*env)->GetArrayLength(env, bytearray);
			bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
			(*env)->GetByteArrayRegion(env, bytearray, 0, blen, bytereg);
			ximsc = XmStringCreateLocalized((char *) bytereg);
			XtFree((char *)bytereg);

			XtSetArg(args[argc], XmNacceleratorText, ximsc);
			argc++;
		}
	}
	isCheckBox = (*env)->GetBooleanField(env, this, MCachedIDs.MMenuItemPeer_isCheckboxFID);
	if (isCheckBox == JNI_TRUE) {

		Dimension       indSize = awt_adjustIndicatorSizeForMenu(awt_computeIndicatorSize(fontData));
		if (indSize != MOTIF_XmINVALID_DIMENSION) {
			XtSetArg(args[argc], XmNindicatorSize, indSize);
			argc++;
		}
		XtSetArg(args[argc], XmNset, False);
		argc++;
		XtSetArg(args[argc], XmNvisibleWhenOff, True);
		argc++;
		menuItemData->comp.widget =
			XmCreateToggleButton(menuData->itemData.comp.widget, "mnutog:", args, argc);
	} else {
		menuItemData->comp.widget =
			XmCreatePushButton(menuData->itemData.comp.widget, "mnupsh:", args, argc);
	}

	XtAddCallback(menuItemData->comp.widget,
		      (isCheckBox == JNI_TRUE) ?
		      XmNvalueChangedCallback : XmNactivateCallback,
		(XtCallbackProc) MenuItem_selected, (XtPointer) globalThis);

	enabled = (*env)->GetBooleanField(env, target, MCachedIDs.java_awt_MenuItem_enabledFID);
	XtSetSensitive(menuItemData->comp.widget, (enabled == JNI_TRUE) ? True : False);

	XtManageChild(menuItemData->comp.widget);

	if (xim != NULL)
		XmStringFree(xim);
	if (ximsc != NULL)
		XmStringFree(ximsc);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, &bg);
	argc++;
	XtGetValues(menuData->itemData.comp.widget, args, argc);
	XmChangeColor(menuItemData->comp.widget, bg);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuItemPeer_pEnable(JNIEnv * env, jobject this)
{
	struct MenuItemData *mdata;

	AWT_LOCK();

	mdata = (struct MenuItemData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (mdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtSetSensitive(mdata->comp.widget, True);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuItemPeer_pDisable(JNIEnv * env, jobject this)
{
	struct MenuItemData *mdata;

	AWT_LOCK();

	mdata = (struct MenuItemData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (mdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtSetSensitive(mdata->comp.widget, False);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuItemPeer_pDispose(JNIEnv * env, jobject this)
{
	struct MenuItemData *menuItemData;

	AWT_LOCK();

	menuItemData = (struct MenuItemData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (menuItemData != NULL) {
		jobject         globalThis;

		XtVaGetValues(menuItemData->comp.widget, XmNuserData, &globalThis, NULL);
		(*env)->DeleteGlobalRef(env, globalThis);
		XtUnmanageChild(menuItemData->comp.widget);
		awt_util_consumeAllXEvents(menuItemData->comp.widget);

                awt_removeEventHandlerForWidget(menuItemData->comp.widget,
                                                menuItemData->comp.peerGlobalRef);
		XtDestroyWidget(menuItemData->comp.widget);

		XtFree((void *) menuItemData);
		(*env)->SetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID, (jint) NULL);
	}
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuItemPeer_pSetLabel(JNIEnv * env, jobject this, jbyteArray label)
{
	XmString        xim = NULL;
	struct ComponentData *menuItemData;
	jbyte          *bytereg;
	jobject         target;
	jobject         font;
	XmFontList      xfl;

	AWT_LOCK();

	menuItemData =
		(struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (menuItemData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	target = (*env)->GetObjectField(env, this, MCachedIDs.MMenuItemPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_MenuComponent_getFontMID);

	xfl = getFontList(env, font);

	bytereg = (*env)->GetByteArrayElements(env, label, NULL);
	xim = XmStringCreateLocalized((char *) bytereg);
	(*env)->ReleaseByteArrayElements(env, label, bytereg, JNI_ABORT);

	XtUnmanageChild(menuItemData->widget);
	XtVaSetValues(menuItemData->widget, XmNlabelString, xim, NULL);
	if(xfl!=NULL) XtVaSetValues(menuItemData->widget, XmNfontList, xfl, NULL);

	XtManageChild(menuItemData->widget);

	if (xim != NULL) {
		XmStringFree(xim);
	}
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCheckboxMenuItemPeer_pSetState(JNIEnv * env, jobject this, jboolean state)
{
	struct MenuItemData *menuItemData;

	AWT_LOCK();

	menuItemData = (struct MenuItemData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (menuItemData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(menuItemData->comp.widget, XmNset, (state == JNI_TRUE) ? True : False, NULL);
	AWT_UNLOCK();
}

/*
 * TODO: This doesn't appear in MCheckboxMenuItemPeer! Why not?
 * 
 * JNIEXPORT jboolean JNICALL Java_sun_awt_motif_MCheckboxMenuItemPeer_getState
 * (JNIEnv *env, jobject this) { struct MenuItemData *mdata; Boolean state;
 * 
 * AWT_LOCK ();
 * 
 * mdata = (struct MenuItemData *)(*env)->GetIntField (env, this,
 * MCachedIDs.MComponentPeer_pDataFID);
 * 
 * if (mdata == NULL) { (*env)->ThrowNew (env,
 * MCachedIDs.java_lang_NullPointerExceptionClass, NULL); AWT_UNLOCK ();
 * return JNI_FALSE; }
 * 
 * XtVaGetValues (mdata->comp.widget, XmNset, &state, NULL); AWT_UNLOCK ();
 * 
 * return (state) ? JNI_TRUE : JNI_FALSE; }
 */
