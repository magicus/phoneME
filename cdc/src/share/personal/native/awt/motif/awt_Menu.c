/*
 * @(#)awt_Menu.c	1.49 06/10/10
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
#include "color.h"
#include "java_awt_Menu.h"
#include "sun_awt_motif_MMenuPeer.h"
#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "awt_Font.h"
#include "multi_font.h"

/*
 * Creates a menu. Returns JNI_TRUE on success or JNI_FALSE if, and only if,
 * an exception was thrown.
 */

static          jboolean
awt_createMenu(JNIEnv * env, jobject this, Widget menuParent)
{
	int             argc;
	Arg             args[10];
	struct MenuData *mdata;
	Pixel           bg;
	Pixel           fg;
	Widget          tearOffWidget;
	jobject         target = (*env)->GetObjectField(env, this, MCachedIDs.MMenuItemPeer_targetFID);
	XmString        xim;
	XmFontList      xfl;
	jobject         font;
	jstring         label;
	jboolean        tearOff;

	jbyte          *bytereg;
	jbyteArray      bytearray;

	int             blen;

	if (target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return JNI_FALSE;
	}
	mdata = (struct MenuData *)XtCalloc(1, sizeof(struct MenuData));

	if (mdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		return JNI_FALSE;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID, (jint) mdata);

	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_MenuComponent_getFontMID);

	if ((*env)->ExceptionCheck(env))
		return JNI_FALSE;

	xfl = getFontList(env, font);

	XtVaGetValues(menuParent, XmNbackground, &bg, NULL);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;

	tearOff = (*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Menu_tearOffFID);

	if (tearOff == JNI_TRUE) {
		XtSetArg(args[argc], XmNtearOffModel, XmTEAR_OFF_ENABLED);
		argc++;
	}
	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;

	mdata->itemData.comp.widget = XmCreatePulldownMenu(menuParent, "menu:", args, argc);

	if (tearOff == JNI_TRUE) {
		tearOffWidget = XmGetTearOffControl(mdata->itemData.comp.widget);
		fg = AwtColorMatch(0, 0, 0);
		XtVaSetValues(tearOffWidget, XmNbackground, bg, XmNforeground, fg, XmNhighlightColor, fg, NULL);
	}
	argc = 0;
	XtSetArg(args[argc], XmNsubMenuId, mdata->itemData.comp.widget);
	argc++;

	label = (jstring) (*env)->GetObjectField(env, target, MCachedIDs.java_awt_MenuItem_labelFID);
	bytearray = (*env)->CallObjectMethod(env, label, MCachedIDs.java_lang_String_getBytesMID);

	blen = (*env)->GetArrayLength(env, bytearray);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, bytearray, 0, blen, bytereg);
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree((char *)bytereg);

	XtSetArg(args[argc], XmNlabelString, xim);
	argc++;
	if(xfl!=NULL) { XtSetArg(args[argc], XmNfontList, xfl); argc++;}

	XtSetArg(args[argc], XmNbackground, bg);
	argc++;

	mdata->comp.widget = XmCreateCascadeButton(menuParent, "mnubtn:", args, argc);

	if ((*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Menu_isHelpMenuFID) == JNI_TRUE)
		XtVaSetValues(menuParent, XmNmenuHelpWidget, mdata->comp.widget, NULL);

	XtManageChild(mdata->comp.widget);
	XtSetSensitive(mdata->comp.widget,
		       ((*env)->GetBooleanField(env, target, MCachedIDs.java_awt_MenuItem_enabledFID) == JNI_TRUE) ? True : False);

	if (xim != NULL)
		XmStringFree(xim);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, &bg);
	argc++;
	XtGetValues(mdata->comp.widget, args, argc);
	XmChangeColor(mdata->comp.widget, bg);

	return JNI_TRUE;
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuPeer_createMenu(JNIEnv * env, jobject this, jobject parent)
{
	struct ComponentData *componentData;

	AWT_LOCK();

	if (parent == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	componentData =
		(struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MMenuBarPeer_pDataFID);

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_createMenu(env, this, componentData->widget);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuPeer_createSubMenu(JNIEnv * env, jobject this, jobject parent)
{
	struct MenuData *parentMenuData;

	AWT_LOCK();

	if (parent == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	parentMenuData = (struct MenuData *) (*env)->GetIntField(env, parent, MCachedIDs.MMenuBarPeer_pDataFID);

	if (parentMenuData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_createMenu(env, this, parentMenuData->itemData.comp.widget);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MMenuPeer_pDispose(JNIEnv * env, jobject this)
{
	struct MenuData *menuItemData;

	AWT_LOCK();

	menuItemData = (struct MenuData *) (*env)->GetIntField(env, this, MCachedIDs.MMenuItemPeer_pDataFID);

	if (menuItemData == NULL) {
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(menuItemData->comp.widget);
	awt_util_consumeAllXEvents(menuItemData->itemData.comp.widget);
	awt_util_consumeAllXEvents(menuItemData->comp.widget);
	XtDestroyWidget(menuItemData->itemData.comp.widget);
	XtDestroyWidget(menuItemData->comp.widget);
	XtFree((void *) menuItemData);
	AWT_UNLOCK();
}
