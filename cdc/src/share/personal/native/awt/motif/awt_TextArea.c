/*
 * @(#)awt_TextArea.c	1.67 06/10/10
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
#include "canvas.h"
#include "java_awt_TextArea.h"
#include "java_awt_Cursor.h"
#include "java_awt_Component.h"
#include "java_awt_Color.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_Font.h"
#include "sun_awt_motif_MTextAreaPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include <X11/cursorfont.h>
#include "awt_Font.h"
#include "awt_Text.h"
#include "multi_font.h"
#include "color.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_initIDs(JNIEnv * env, jclass cls)
{
	cls = (*env)->FindClass(env, "java/awt/TextArea");

	MCachedIDs.java_awt_TextArea_scrollbarVisibilityFID =
		(*env)->GetFieldID(env, cls, "scrollbarVisibility", "I");
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_create(JNIEnv * env, jobject this, jobject parent)
{
	struct TextAreaData *textAreaData;
	Arg             args[30];
	int             argc;
	struct ComponentData *parentComponentData;
	jobject         target;
	Pixel           bg;
	jint            sbVisibility;
	Boolean         wordWrap, hsb, vsb;
	jobject         thisGlobalRef;

	AWT_LOCK();

	if (parent == NULL ||
	    (parentComponentData =
	     (struct ComponentData *) (*env)->GetIntField(env, parent,
			     MCachedIDs.MComponentPeer_pDataFID)) == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	textAreaData = (struct TextAreaData *)XtCalloc(1, sizeof(struct TextAreaData));

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) textAreaData);

	if (textAreaData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(parentComponentData->widget, XmNbackground, &bg, NULL);

	sbVisibility = (*env)->GetIntField(env, target, MCachedIDs.java_awt_TextArea_scrollbarVisibilityFID);

	switch (sbVisibility) {
	case java_awt_TextArea_SCROLLBARS_NONE:
		wordWrap = True;
		hsb = False;
		vsb = False;
		break;
	case java_awt_TextArea_SCROLLBARS_VERTICAL_ONLY:
		wordWrap = True;
		hsb = False;
		vsb = True;
		break;
	case java_awt_TextArea_SCROLLBARS_HORIZONTAL_ONLY:
		wordWrap = False;
		hsb = True;
		vsb = False;
		break;
	case java_awt_TextArea_SCROLLBARS_BOTH:
	default:
		wordWrap = False;
		hsb = True;
		vsb = True;
		break;
	}

	thisGlobalRef = textAreaData->comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		XtFree((char *)textAreaData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	argc = 0;

	/*
	XtSetArg(args[argc], XmNrecomputeSize, False);
	argc++;
	*/

	XtSetArg(args[argc], XmNx, 0);
	argc++;
	XtSetArg(args[argc], XmNy, 0);
	argc++;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNeditMode, XmMULTI_LINE_EDIT);
	argc++;
	XtSetArg(args[argc], XmNwordWrap, wordWrap);
	argc++;

	/*

	XtSetArg(args[argc], XmNscrollHorizontal, hsb);
	argc++;
	XtSetArg(args[argc], XmNscrollVertical, vsb);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 2);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 2);
	argc++;
	*/

      
	XtSetArg(args[argc], XmNscrollBarDisplayPolicy, XmAS_NEEDED);
	argc++;

	XtSetArg(args[argc], XmNuserData, thisGlobalRef);
	argc++;

	textAreaData->txt = XmCreateScrolledText(parentComponentData->widget, "textA:", args, argc);
	textAreaData->comp.widget = XtParent(textAreaData->txt);

	/* XtSetMappedWhenManaged(textAreaData->comp.widget, False); */

	XtManageChild(textAreaData->txt);
	XtManageChild(textAreaData->comp.widget);

	XtAddCallback(textAreaData->txt, XmNvalueChangedCallback, Text_valueChanged, (XtPointer) thisGlobalRef);

	/*	XtAddEventHandler(textAreaData->txt, StructureNotifyMask, True, Text_mapNotify, (XtPointer) thisGlobalRef); */

	XtInsertEventHandler(textAreaData->txt, KeyPressMask | KeyReleaseMask, False, Text_handlePaste, (XtPointer) thisGlobalRef, XtListHead);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_pDispose(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;

	AWT_LOCK();
	textAreaData =
		(struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textAreaData != NULL)
		awt_delWidget(textAreaData->txt);

	AWT_UNLOCK();

	Java_sun_awt_motif_MComponentPeer_pDispose(env, this);
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextAreaPeer_getExtraWidth(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	Dimension       spacing, shadowThickness, textMarginWidth, sbWidth;
	Widget          verticalScrollBar;

	AWT_LOCK();


	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	XtVaGetValues(textAreaData->txt, XmNmarginWidth, &textMarginWidth, NULL);
	XtVaGetValues(textAreaData->comp.widget, XmNspacing, &spacing, XmNverticalScrollBar, &verticalScrollBar,
		      NULL);

	if (verticalScrollBar) {
		/*
		 * Assumption:  shadowThickness same for scrollbars and text
		 * area
		 */
		XtVaGetValues(verticalScrollBar, XmNwidth, &sbWidth, XmNshadowThickness, &shadowThickness, NULL);
	} else {
		sbWidth = 0;
		shadowThickness = 0;
	}

	AWT_UNLOCK();

	return (sbWidth + spacing + 2 * textMarginWidth + 4 * shadowThickness);
}


JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextAreaPeer_getExtraHeight(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	Dimension       spacing, shadowThickness, textMarginHeight, sbHeight;
	Widget          horizontalScrollBar;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	XtVaGetValues(textAreaData->txt, XmNmarginHeight, &textMarginHeight, NULL);
	XtVaGetValues(textAreaData->comp.widget,
		      XmNspacing, &spacing, XmNhorizontalScrollBar, &horizontalScrollBar, NULL);

	if (horizontalScrollBar) {
		/*
		 * Assumption:  shadowThickness same for scrollbars and text
		 * area
		 */
		XtVaGetValues(horizontalScrollBar, XmNheight, &sbHeight, XmNshadowThickness, &shadowThickness, NULL);
	} else {
		sbHeight = 0;
		shadowThickness = 0;
	}

	AWT_UNLOCK();
	return (sbHeight + spacing + 2 * textMarginHeight + 4 * shadowThickness);
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_pSetEditable(JNIEnv * env, jobject this, jboolean editable)
{
	struct TextAreaData *textAreaData;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(textAreaData->txt,
		      XmNeditable, (editable ? True : False),
		 XmNcursorPositionVisible, (editable ? True : False), NULL);

	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_setTextBackground(JNIEnv * env, jobject this, jobject c)
{
	struct TextAreaData *textAreaData;
	Pixel           color;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL || c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	color = awt_getColor(env, c);
	XtVaSetValues(textAreaData->txt, XmNbackground, color, NULL);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_pSelect(JNIEnv * env, jobject this, jint start, jint end)
{
	struct TextAreaData *textAreaData;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XmTextSetSelection(textAreaData->txt, (XmTextPosition) start, (XmTextPosition) end, 0);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextAreaPeer_getSelectionStart(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	XmTextPosition  start, end, pos;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	if (XmTextGetSelectionPosition(textAreaData->txt, &start, &end) && (start != end)) {
		pos = start;
	} else {
		pos = XmTextGetCursorPosition(textAreaData->txt);
	}

	AWT_UNLOCK();

	return (jint) pos;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextAreaPeer_getSelectionEnd(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	XmTextPosition  start, end, pos;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	if (XmTextGetSelectionPosition(textAreaData->txt, &start, &end) && (start != end)) {
		pos = end;
	} else {
		pos = XmTextGetCursorPosition(textAreaData->txt);
	}

	AWT_UNLOCK();

	return (jint) pos;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextAreaPeer_getCaretPosition(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	XmTextPosition  pos;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	pos = XmTextGetCursorPosition(textAreaData->txt);
	AWT_UNLOCK();

	return pos;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_setCaretPosition(JNIEnv * env, jobject this, jint pos)
{
	struct TextAreaData *textAreaData;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XmTextSetCursorPosition(textAreaData->txt, (XmTextPosition) pos);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MTextAreaPeer_endPos(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	XmTextPosition  pos;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	pos = XmTextGetLastPosition(textAreaData->txt);
	AWT_UNLOCK();

	return (jint) pos;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_psetText(JNIEnv * env, jobject this, jbyteArray txt)
{
	struct TextAreaData *textAreaData;
	jobject         font;
	jobject         target;
	jbyte          *bytereg;
	int             blen;

	if (txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	if ((textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return;
	}
	blen = (*env)->GetArrayLength(env, txt);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, txt, 0, blen, bytereg);
	XtVaSetValues(textAreaData->txt, XmNvalue, (char *) bytereg, NULL);
	XtFree((char *)bytereg);

	AWT_UNLOCK();
}

JNIEXPORT jstring JNICALL
Java_sun_awt_motif_MTextAreaPeer_getText(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;
	char           *cTxt;
	jstring         rval;
	jobject         target;
	jobject         font;

	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return 0;
	}
	cTxt = XmTextGetString(textAreaData->txt);

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env))
		return NULL;

	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "font is null");
		AWT_UNLOCK();
		return NULL;
	}
	/*
	 * TODO: How to create Java string from platform string? if
	 * (isMultiFont (env, font)) rval = makeJavaStringFromPlatformCString
	 * (cTxt, strlen (cTxt)); else rval = makeJavaString (cTxt, strlen
	 * (cTxt));
	 */

	rval = (*env)->NewStringUTF(env, cTxt);

	if (cTxt != NULL) {
		XtFree(cTxt);
		cTxt = NULL;
	}
	AWT_UNLOCK();

	return rval;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_insert(JNIEnv * env, jobject this, jstring txt, jint pos)
{
	struct TextAreaData *textAreaData;
	char           *cTxt;
	jobject         font;
	jobject         target;

	if (txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env))
		return;

	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "font is null");
		AWT_UNLOCK();
		return;
	}
	/*
	 * TODO: Work out platform stuff and wehn to free string. if
	 * (isMultiFont (env, font)) cTxt = makePlatformCString (txt); else
	 * cTxt = makeCString (txt);
	 */

	cTxt = (char *) (*env)->GetStringUTFChars(env, txt, NULL);
	XmTextInsert(textAreaData->txt, (XmTextPosition) pos, cTxt);
	(*env)->ReleaseStringUTFChars(env, txt, cTxt);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_replaceRange(JNIEnv * env, jobject this, jstring txt, jint start, jint end)
{
	struct TextAreaData *textAreaData;
	char           *cTxt;
	jobject         font;
	jobject         target;

	if (txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();
	if (
	    (textAreaData =
	     (struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID)) == NULL
	    || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env))
		return;

	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "font is null");
		AWT_UNLOCK();
		return;
	}
	/*
	 * TODO: Work out platform stuff and wehn to free string. if
	 * (isMultiFont (env, font)) cTxt = makePlatformCString (txt); else
	 * cTxt = makeCString (txt);
	 */

	cTxt = (char *) (*env)->GetStringUTFChars(env, txt, NULL);
	XmTextReplace(textAreaData->txt, (XmTextPosition) start, (XmTextPosition) end, cTxt);
	(*env)->ReleaseStringUTFChars(env, txt, cTxt);
	AWT_FLUSH_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_setFont(JNIEnv * env, jobject this, jobject f)
{
	struct TextAreaData *textAreaData;
	XmFontList      fontlist;

	if (f == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	textAreaData =
		(struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textAreaData == NULL || textAreaData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	fontlist = getFontList(env, f);

	if (fontlist != NULL) {
		XtVaSetValues(textAreaData->txt, XmNfontList, fontlist, NULL);
	}
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_pShow(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;

	AWT_LOCK();

	textAreaData =
		(struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textAreaData == NULL || textAreaData->comp.widget == NULL || textAreaData->txt == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_util_show(env, textAreaData->comp.widget);

	if ((*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_cursorSetFID) == 0) {
		(*env)->SetIntField(env, this,
				    MCachedIDs.MComponentPeer_cursorSetFID,
				    awt_util_setCursor(textAreaData->txt, textAreaData->comp.cursor));
	}
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_pMakeCursorVisible(JNIEnv * env, jobject this)
{
	struct TextAreaData *textAreaData;

	AWT_LOCK();

	textAreaData =
		(struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textAreaData == NULL || textAreaData->comp.widget == NULL) {
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this,
			    MCachedIDs.MComponentPeer_cursorSetFID,
	  awt_util_setCursor(textAreaData->txt, textAreaData->comp.cursor));

	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextAreaPeer_setCursor(JNIEnv * env, jobject this, jobject cursor)
{
	Cursor          xcursor;
	struct TextAreaData *textAreaData;
	jint            cursorType;

	AWT_LOCK();

	textAreaData =
		(struct TextAreaData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (textAreaData == NULL || textAreaData->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (cursor == NULL) {
		xcursor = None;
	} else {
		cursorType = (*env)->GetIntField(env, cursor, MCachedIDs.java_awt_Cursor_typeFID);

		switch (cursorType) {
		case java_awt_Cursor_DEFAULT_CURSOR:
			cursorType = XC_left_ptr;
			break;
		case java_awt_Cursor_CROSSHAIR_CURSOR:
			cursorType = XC_crosshair;
			break;
		case java_awt_Cursor_TEXT_CURSOR:
			cursorType = XC_xterm;
			break;
		case java_awt_Cursor_WAIT_CURSOR:
			cursorType = XC_watch;
			break;
		case java_awt_Cursor_SW_RESIZE_CURSOR:
			cursorType = XC_bottom_left_corner;
			break;
		case java_awt_Cursor_NW_RESIZE_CURSOR:
			cursorType = XC_top_left_corner;
			break;
		case java_awt_Cursor_SE_RESIZE_CURSOR:
			cursorType = XC_bottom_right_corner;
			break;
		case java_awt_Cursor_NE_RESIZE_CURSOR:
			cursorType = XC_top_right_corner;
			break;
		case java_awt_Cursor_S_RESIZE_CURSOR:
			cursorType = XC_bottom_side;
			break;
		case java_awt_Cursor_N_RESIZE_CURSOR:
			cursorType = XC_top_side;
			break;
		case java_awt_Cursor_W_RESIZE_CURSOR:
			cursorType = XC_left_side;
			break;
		case java_awt_Cursor_E_RESIZE_CURSOR:
			cursorType = XC_right_side;
			break;
		case java_awt_Cursor_HAND_CURSOR:
			cursorType = XC_hand2;
			break;
		case java_awt_Cursor_MOVE_CURSOR:
			cursorType = XC_fleur;
			break;
		default:

			(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass,
					 "Unknown cursor type");
			AWT_UNLOCK();
			return;
		}

		xcursor = XCreateFontCursor(awt_display, cursorType);
	}

	if (textAreaData->comp.cursor != 0 && textAreaData->comp.cursor != None) {
		XFreeCursor(awt_display, textAreaData->comp.cursor);
	}
	textAreaData->comp.cursor = xcursor;

	(*env)->SetIntField(env, this,
			    MCachedIDs.MComponentPeer_cursorSetFID,
	  awt_util_setCursor(textAreaData->txt, textAreaData->comp.cursor));

	AWT_FLUSH_UNLOCK();
}
