/*
 * @(#)awt_TextField.c	1.87 06/10/10
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

#include <Xm/VirtKeys.h>

#include "awt_p.h"
#include "canvas.h"
#include "java_awt_TextField.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Canvas.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MCanvasPeer.h"
#include "sun_awt_motif_MTextFieldPeer.h"
#include "awt_Font.h"
#include "awt_Text.h"
#include "multi_font.h"
#include <Xm/TextFP.h>		/* Motif TextField private header. */


#define ECHO_BUFFER_LEN 1024

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MTextFieldPeer_actionMID = (*env)->GetMethodID(env, cls, "action", "()V");

	cls = (*env)->FindClass(env, "java/awt/TextField");

	MCachedIDs.java_awt_TextField_echoCharFID = (*env)->GetFieldID(env, cls, "echoChar", "C");
}

static void
TextField_action(Widget w, XtPointer client_data, XmAnyCallbackStruct * s)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MTextFieldPeer_actionMID);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	struct ComponentData *parentComponentData;
	struct TextFieldData *textFieldData;
	jobject         thisGlobalRef;

	AWT_LOCK();

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	parentComponentData =
		(struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);
	textFieldData = (struct TextFieldData *)XtCalloc(1, sizeof(struct TextFieldData));

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) textFieldData);

	if (textFieldData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	thisGlobalRef = textFieldData->comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	textFieldData->comp.widget = XtVaCreateManagedWidget("textfield:",
						     xmTextFieldWidgetClass,
						parentComponentData->widget,
						    XmNrecomputeSize, False,
						   XmNhighlightThickness, 1,
						      XmNshadowThickness, 2,
				     XmNuserData, (XtPointer) thisGlobalRef,
							     NULL);

	/* XtSetMappedWhenManaged(textFieldData->comp.widget, False); */
	XtAddCallback(textFieldData->comp.widget,
		      XmNactivateCallback, (XtCallbackProc) TextField_action, (XtPointer) thisGlobalRef);
	XtAddCallback(textFieldData->comp.widget,
		      XmNvalueChangedCallback, (XtCallbackProc) Text_valueChanged, (XtPointer) thisGlobalRef);
	XtAddEventHandler(textFieldData->comp.widget, StructureNotifyMask, True, Text_mapNotify, thisGlobalRef);

	XtInsertEventHandler(textFieldData->comp.widget,
			     KeyPressMask | KeyReleaseMask, False, Text_handlePaste, (XtPointer) thisGlobalRef, XtListHead);

	AWT_UNLOCK();
}

/*
 * TextField can not use the setFont() method in Component, because TextField
 * uses XFontSet as XmNfontList, and normal Components don't use it.
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_setFont(JNIEnv * env, jobject this, jobject font)
{
	struct TextFieldData *textFieldData;
	XmFontList      xfl;

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	xfl = getFontList(env, font);
	if (xfl != NULL)
		XtVaSetValues(textFieldData->comp.widget, XmNfontList, xfl, NULL);

	AWT_UNLOCK();
}

JNIEXPORT jstring JNICALL
Java_sun_awt_motif_MTextFieldPeer_getText(JNIEnv * env, jobject this)
{
	struct TextFieldData *textFieldData;	/* Bug#4141731 */
	char           *val = NULL;
	struct DPos    *dp;
	jobject         target;
	jobject         font = NULL;
	int             ret;
	jstring         returnVal = NULL;
	jchar           echoChar;

	AWT_LOCK();		/* Bug#4141731 */

	textFieldData = (struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return NULL;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	echoChar = (*env)->GetCharField(env, target, MCachedIDs.java_awt_TextField_echoCharFID);

	if (echoChar != 0) {
		ret = XFindContext(XtDisplay(textFieldData->comp.widget), XtWindow(textFieldData->comp.widget),
			   textFieldData->echoContextID, (XPointer *) & dp);

		if ((ret == 0) && (dp != NULL)) {
			val = (char *) (dp->data);
		} else {
			val = "";
		}
	} else {
	  /*		XtVaGetValues(textFieldData->comp.widget, XmNvalue, &val, NULL); */
	  val=XmTextGetString(textFieldData->comp.widget);
	}

	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env))
		goto finish;

	/*
	 * TODO: Correctly make the Java string.... if (isMultiFont (env,
	 * font)) returnVal = makeJavaStringFromPlatformCString (val, strlen
	 * (val));
	 *
	 * else returnVal = makeJavaString (val, strlen (val));
	 */

	returnVal = (*env)->NewStringUTF(env, val);

finish:

	AWT_UNLOCK();

	if (val != NULL)
		XtFree(val);

	return returnVal;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_psetText(JNIEnv * env, jobject this, jbyteArray text)
{
	struct TextFieldData *textFieldData;	/* Bug#4141731 */
	/*	char           *ctext = NULL; */
	jobject         target;
	jobject         font;
	jchar           echoChar;
	jbyte          *bytereg;
	int             blen;

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env))
		return;

#if 0

	if (text == NULL) {
		ctext = "";
	} else {


		/*
		 * We use makePlatformCString() to convert unicode to EUC
		 * here, although output only components
		 * (Label/Button/Menu..) is not using make/allocCString()
		 * functions anymore. Because Motif TextFiled widget does not
		 * support multi-font compound string.
		 */
		/*
		 * TODO: Correctly convert this and work out when to free
		 * string. if (isMultiFont (env, font)) ctext =
		 * makePlatformCString (text); else ctext = makeCString
		 * (text);
		 */

		ctext = (char *) (*env)->GetStringUTFChars(env, text, NULL);


	}
#endif

	textFieldData = (struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	blen = (*env)->GetArrayLength(env, text);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, text, 0, blen, bytereg);

	echoChar = (*env)->GetCharField(env, target, MCachedIDs.java_awt_TextField_echoCharFID);

	if (echoChar != 0) {
	  /*		XtVaSetValues(textFieldData->comp.widget, XmNvalue, "", NULL); */
	  XmTextSetString(textFieldData->comp.widget, "");
		XmTextFieldInsert(textFieldData->comp.widget, 0, (char *) bytereg);
		XmTextSetCursorPosition(textFieldData->comp.widget, (XmTextPosition) strlen((char *)bytereg));
	} else {
	  XmTextSetString(textFieldData->comp.widget, (char *)bytereg);

	  /*		XtVaSetValues(textFieldData->comp.widget, XmNvalue, (char *) bytereg, NULL); */
	}

	XtFree((char *)bytereg);

	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_insertReplaceText(JNIEnv * env, jobject this, jstring text)
{
	struct TextFieldData *textFieldData;	/* Bug#4141731 */
	char           *ctext = NULL;
	XmTextPosition  start, end;
	jobject         font;
	jobject         target;

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env))
		goto finish;

	if (text == NULL) {
		ctext = "";
	} else {
		/*
		 * We use makePlatformCString() to convert unicode to EUC
		 * here, although output only components
		 * (Label/Button/Menu..) is not using make/allocCString()
		 * functions anymore. Because Motif TextFiled widget does not
		 * support multi-font compound string.
		 */
		/*
		 * TODO: correctly convert this... if (isMultiFont (env,
		 * font)) ctext = makePlatformCString (text); else ctext =
		 * makeCString (text);
		 */

		ctext = (char *) (*env)->GetStringUTFChars(env, text, NULL);
	}

	textFieldData = (struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		goto finish;
	}
	if (!XmTextGetSelectionPosition(textFieldData->comp.widget, &start, &end)) {
		start = end = XmTextGetCursorPosition(textFieldData->comp.widget);
	}
	XmTextReplace(textFieldData->comp.widget, start, end, ctext);

finish:

	if (text != NULL && ctext != NULL)
		(*env)->ReleaseStringUTFChars(env, text, ctext);

	AWT_FLUSH_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_pSetEditable(JNIEnv * env, jobject this, jboolean editable)
{
	struct TextFieldData *textFieldData;	/* Bug#4141731 */

	AWT_LOCK();
	textFieldData = (struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(textFieldData->comp.widget,
		      XmNeditable, (editable ? True : False),
		 XmNcursorPositionVisible, (editable ? True : False), NULL);

	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_preDispose(JNIEnv * env, jobject this)
{
	struct TextFieldData *textFieldData;	/* Bug#4141731 */
	struct DPos    *dp;
	jobject         target;
	XmTextFieldWidget textFieldWidget = NULL;
	int             ret;
	jchar           echoChar;

	AWT_LOCK();

	textFieldData = (struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	echoChar = (*env)->GetCharField(env, target, MCachedIDs.java_awt_TextField_echoCharFID);

	if (echoChar != 0) {
		ret = XFindContext(XtDisplay(textFieldData->comp.widget), XtWindow(textFieldData->comp.widget),
			   textFieldData->echoContextID, (XPointer *) & dp);

		if ((ret == 0) && (dp != NULL)) {
			/*
			 * Remove the X context associated with this
			 * textfield's echo character. BugId #4225734
			 */

			XDeleteContext(XtDisplay(textFieldData->comp.widget), XtWindow(textFieldData->comp.widget),
				       textFieldData->echoContextID);

			textFieldData->echoContextIDInit = FALSE;


			/*
			 * Free up the space allocated for the echo character
			 * data.
			 */

			if (dp->data != NULL)
				XtFree(dp->data);

			XtFree((char *)dp);
			dp = NULL;
		}
	}
	/*
	 * Remove the BrowseScroll timer, because Motif fails to do it in its
	 * Destroy function in Xm/TextF.c. Ask Motif group to fix it.
	 */
	textFieldWidget = (XmTextFieldWidget) textFieldData->comp.widget;

	if (textFieldWidget->text.select_id) {
		XtRemoveTimeOut(textFieldWidget->text.select_id);
		textFieldWidget->text.select_id = (XtIntervalId) NULL;
	}
	AWT_UNLOCK();
}

static void
echoCharCallback(Widget text_w, XtPointer unused, XmTextVerifyCallbackStruct * cbs)
{
	int             len;
	long            c;
	char           *val;
	struct DPos    *dp;
	int             ret;
	jobject         this;
	struct TextFieldData *textFieldData;
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	/*
	 * Get the echoContextID from the globalRef which is stored in the
	 * XmNuserData resource for the widget.
	 */

	XtVaGetValues(text_w, XmNuserData, &this, NULL);
	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	ret = XFindContext(XtDisplay(text_w), XtWindow(text_w), textFieldData->echoContextID, (XPointer *) & dp);

	if ((ret != 0) || (dp == NULL)) {
		/* no context found or DPos is NULL  - shouldn't happen */
		return;
	}
	c = dp->echoC;
	val = (char *) (dp->data);
	len = strlen(val);

	if (cbs->text->ptr == NULL) {
		if (cbs->text->length == 0 && cbs->startPos == 0) {
			val[0] = 0;
			return;
		} else if (cbs->startPos == (len - 1)) {
			/* handle deletion */
			cbs->endPos = strlen(val);
			val[cbs->startPos] = 0;
			return;
		} else {
			/* disable deletes anywhere but at the end */
			cbs->doit = False;
			return;
		}
	}
	if (cbs->startPos != len) {
		/* disable "paste" or inserts into the middle */
		cbs->doit = False;
		return;
	}
	/* append the value typed in */
	if ((cbs->endPos + cbs->text->length) > ECHO_BUFFER_LEN) {
		val = XtRealloc(val, cbs->endPos + cbs->text->length + 10);
	}
	strncat(val, cbs->text->ptr, cbs->text->length);
	val[cbs->endPos + cbs->text->length] = 0;

	/* modify the output to be the echo character */
	for (len = 0; len < cbs->text->length; len++) {
		cbs->text->ptr[len] = (char) (long) c;
	}
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_setEchoChar(JNIEnv * env, jobject this, jchar c)
{
	char           *val;
	char           *cval;
	struct TextFieldData *textFieldData;
	struct DPos    *dp;
	int             i, len;
	int             ret;

	AWT_LOCK();		/* Bug#4141731 */

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	/*
	 * If changing the echo char, we need to remove any existing instance
	 * of the callback which echoes input when in password mode. If omit
	 * this step then an app which sets echo character twice completely
	 * breaks the text field, as well as losing the data
	 */

	XtVaGetValues(textFieldData->comp.widget, XmNvalue, &cval, NULL);

	if (!textFieldData->echoContextIDInit) {
		textFieldData->echoContextID = XUniqueContext();
		textFieldData->echoContextIDInit = TRUE;
	}
	ret = XFindContext(XtDisplay(textFieldData->comp.widget), XtWindow(textFieldData->comp.widget),
			   textFieldData->echoContextID, (XPointer *) & dp);

	if (ret != 0) {
		dp = NULL;
	}
	if (dp) {
		XtRemoveCallback(textFieldData->comp.widget, XmNmodifyVerifyCallback,
				 (XtCallbackProc) echoCharCallback, NULL);
	} else {
		if ((int) strlen(cval) > ECHO_BUFFER_LEN)
			val = (char *) XtMalloc(strlen(cval) + 1);

		else
			val = (char *) XtMalloc(ECHO_BUFFER_LEN + 1);

		if (val == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
			AWT_UNLOCK();
			return;
		}
		if (cval)
			strcpy(val, cval);
		else
			*val = 0;

		dp = (struct DPos *) XtMalloc(sizeof(struct DPos));

		dp->x = -1;
		dp->data = (void *) val;
	}

	dp->echoC = c;
	len = strlen(cval);

	for (i = 0; i < len; i++) {
		cval[i] = (char) (c);
	}

	XtVaSetValues(textFieldData->comp.widget, XmNvalue, cval, NULL);

	ret = XSaveContext(XtDisplay(textFieldData->comp.widget), XtWindow(textFieldData->comp.widget),
			   textFieldData->echoContextID, (XPointer) dp);

	if (ret == 0)
		XtAddCallback(textFieldData->comp.widget, XmNmodifyVerifyCallback, (XtCallbackProc) echoCharCallback,
			      NULL);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_pSelect(JNIEnv * env, jobject this, jint start, jint end)
{
	struct TextFieldData *textFieldData;

	AWT_LOCK();

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XmTextSetSelection(textFieldData->comp.widget, (XmTextPosition) start, (XmTextPosition) end, 0);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextFieldPeer_getSelectionStart(JNIEnv * env, jobject this)
{
	struct TextFieldData *textFieldData;
	XmTextPosition  start, end, pos;

	AWT_LOCK();

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	if (XmTextGetSelectionPosition(textFieldData->comp.widget, &start, &end) && (start != end))
		pos = start;
	else
		pos = XmTextGetCursorPosition(textFieldData->comp.widget);

	AWT_UNLOCK();

	return pos;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextFieldPeer_getSelectionEnd(JNIEnv * env, jobject this)
{
	struct TextFieldData *textFieldData;
	XmTextPosition  start, end, pos;

	AWT_LOCK();

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	if (XmTextGetSelectionPosition(textFieldData->comp.widget, &start, &end) && (start != end))
		pos = end;

	else
		pos = XmTextGetCursorPosition(textFieldData->comp.widget);

	AWT_UNLOCK();

	return pos;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextFieldPeer_getCaretPosition(JNIEnv * env, jobject this)
{
	struct TextFieldData *textFieldData;
	XmTextPosition  pos;

	AWT_LOCK();

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	pos = XmTextGetCursorPosition(textFieldData->comp.widget);
	AWT_UNLOCK();

	return (jint) pos;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextFieldPeer_setCaretPosition(JNIEnv * env, jobject this, jint pos)
{
	struct TextFieldData *textFieldData;

	AWT_LOCK();

	textFieldData =
		(struct TextFieldData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textFieldData == NULL || textFieldData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XmTextSetCursorPosition(textFieldData->comp.widget, (XmTextPosition) pos);

	AWT_FLUSH_UNLOCK();
}
