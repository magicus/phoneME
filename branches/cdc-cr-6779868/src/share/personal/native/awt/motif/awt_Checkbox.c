/*
 * @(#)awt_Checkbox.c	1.47 06/10/10
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
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MCheckboxPeer.h"
#include "java_awt_Checkbox.h"
#include "java_awt_CheckboxGroup.h"

#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCheckboxPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MCheckboxPeer_actionMID = (*env)->GetMethodID(env, cls, "action", "(Z)V");

	cls = (*env)->FindClass(env, "java/awt/Checkbox");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Checkbox_labelFID = (*env)->GetFieldID(env, cls, "label", "Ljava/lang/String;");
}

static void
Toggle_callback(Widget w, XtPointer client_data, XmAnyCallbackStruct * call_data)
{
	JNIEnv         *env;
	jboolean        state;

	XtVaGetValues(w, XmNset, &state, NULL);

	(*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL);

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MCheckboxPeer_actionMID, state);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCheckboxPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	jobject         target, font, label, thisGlobalRef;

	struct ComponentData *componentData;
	struct ComponentData *parentComponentData;
	Arg             args[10];
	int             argc;

	XmString        xim;
	XmFontList      xfl;
	jbyteArray      bytearray;
	jbyte          *bytereg;
	int             blen;

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	parentComponentData = (struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);

	if (parentComponentData == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	componentData = (struct ComponentData *) XtCalloc(1, sizeof(struct ComponentData));
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) componentData);

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	thisGlobalRef = componentData->peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		XtFree((XtPointer) componentData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	argc = 0;
	XtSetArg(args[argc], XmNrecomputeSize, False);
	argc++;
	XtSetArg(args[argc], XmNvisibleWhenOff, True);
	argc++;
	XtSetArg(args[argc], XmNtraversalOn, True);
	argc++;
	XtSetArg(args[argc], XmNspacing, 0);
	argc++;
	XtSetArg(args[argc], XmNuserData, (XtPointer) thisGlobalRef);
	argc++;


	label = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_Checkbox_labelFID);

	if ((label != NULL) && ((*env)->GetStringLength(env, label) > 0)) {

		font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

		if ((*env)->ExceptionCheck(env)) {
			AWT_UNLOCK();
			return;
		}
		xfl = getFontList(env, font);

		bytearray = (*env)->CallObjectMethod(env, label, MCachedIDs.java_lang_String_getBytesMID);
		blen = (*env)->GetArrayLength(env, bytearray);
		bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
		(*env)->GetByteArrayRegion(env, bytearray, 0, blen, bytereg);
		xim = XmStringCreateLocalized((char *) bytereg);
		XtFree((char *)bytereg);

		XtSetArg(args[argc], XmNlabelString, xim);
		argc++;
		if (xfl != NULL) {
			XtSetArg(args[argc], XmNfontList, xfl);
			argc++;
		}
	} else
		xim = NULL;

	componentData->widget = XmCreateToggleButton(parentComponentData->widget, "chktog:", args, argc);

	if (xim != NULL)
		XmStringFree(xim);

	XtAddCallback(componentData->widget,
		      XmNvalueChangedCallback, (XtCallbackProc) Toggle_callback, (XtPointer) thisGlobalRef);

	XtManageChild(componentData->widget);

	AWT_FLUSH_UNLOCK();

}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCheckboxPeer_setCheckboxGroup(JNIEnv * env, jobject this, jobject group)
{
	struct ComponentData *bdata;

	AWT_LOCK();

	bdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (group == NULL) {
		XtVaSetValues(bdata->widget, XmNindicatorType, XmN_OF_MANY, NULL);
	} else {
		XtVaSetValues(bdata->widget, XmNindicatorType, XmONE_OF_MANY, NULL);
	}
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCheckboxPeer_setState(JNIEnv * env, jobject this, jboolean state)
{
	struct ComponentData *bdata;

	AWT_LOCK();

	bdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(bdata->widget, XmNset, (Boolean) state, NULL);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MCheckboxPeer_psetLabel(JNIEnv * env, jobject this, jbyteArray label)
{
	struct ComponentData *wdata;
	XmString        xim;
	XmFontList      xfl;
	jbyte          *bytereg;
	int             blen;

	jobject         target, font;

	AWT_LOCK();

	wdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	printf("Setting the label\n");

	if (wdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if (font == NULL) {
		AWT_UNLOCK();
		return;
	}
	xfl = getFontList(env, font);

	blen = (*env)->GetArrayLength(env, label);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, label, 0, blen, bytereg);
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree((char *)bytereg);

	if (xfl != NULL)
		XtVaSetValues(wdata->widget, XmNfontList, xfl, NULL);

	if (xim != NULL) {
		XtVaSetValues(wdata->widget, XmNlabelString, xim, NULL);

		XmStringFree(xim);
	}
	AWT_UNLOCK();
}
