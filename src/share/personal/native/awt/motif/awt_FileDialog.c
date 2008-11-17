/*
 * @(#)awt_FileDialog.c	1.62 06/10/10
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

#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>
#include "awt_p.h"
#include "awt_Font.h"
#include "java_awt_FileDialog.h"
#include "sun_awt_motif_MFileDialogPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "multi_font.h"

#define MAX_DIR_PATH_LEN    1024

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MFileDialogPeer_handleSelectedMID =
	(*env)->GetMethodID(env, cls, "handleSelected", "(Ljava/lang/String;)V");
	MCachedIDs.MFileDialogPeer_handleCancelMID = (*env)->GetMethodID(env, cls, "handleCancel", "()V");
	MCachedIDs.MFileDialogPeer_handleQuitMID = (*env)->GetMethodID(env, cls, "handleQuit", "()V");

	cls = (*env)->FindClass(env, "java/awt/FileDialog");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_FileDialog_fileFID = (*env)->GetFieldID(env, cls, "file", "Ljava/lang/String;");
	MCachedIDs.java_awt_FileDialog_modeFID = (*env)->GetFieldID(env, cls, "mode", "I");
}

static void
FileDialog_OK(Widget w, XtPointer client_data, XmFileSelectionBoxCallbackStruct * call_data)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;
	char           *file;
	jstring         fileName;
	struct FrameData *frameData;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &file);
	fileName = (*env)->NewStringUTF(env, file);

	(*env)->CallVoidMethod(env, this, MCachedIDs.MFileDialogPeer_handleSelectedMID, fileName);

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	XtUnmanageChild(frameData->winData.comp.widget);
}

static void
FileDialog_CANCEL(Widget w, XtPointer client_data, XmFileSelectionBoxCallbackStruct * call_data)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;
	struct FrameData *frameData;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MFileDialogPeer_handleCancelMID);

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	XtUnmanageChild(frameData->winData.comp.widget);
}


static void
FileDialog_QUIT(Widget w, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MFileDialogPeer_handleQuitMID);
}

void
setFSBDirAndFile(Widget w, char *dir, char *file)
{
	Widget          textField;
	char            dirbuf[MAX_DIR_PATH_LEN];
	XmString        xim;
	int             lastSelect;

	dirbuf[0] = (char) '\0';

	if (dir != 0)
		strcpy(dirbuf, dir);

	/* -----> make sure dir ends in '/' */
	if (dirbuf[0] != (char) '\0') {
		if (dirbuf[strlen(dirbuf) - 1] != (char) '/')
			strcat(dirbuf, "/");
	} else {
		getcwd(dirbuf, MAX_DIR_PATH_LEN - 16);
		strcat(dirbuf, "/");
	}

	strcat(dirbuf, "*");
	xim = XmStringCreateLocalized(dirbuf);
	XtVaSetValues(w, XmNdirMask, xim, NULL);

	if (xim != NULL) {
		XmStringFree(xim);
		xim = NULL;
	}
	textField = XmFileSelectionBoxGetChild(w, XmDIALOG_TEXT);

	if (textField != 0 && file != 0) {
		lastSelect = strlen(file);
		XtVaSetValues(textField, XmNvalue, file, NULL);
		XmTextFieldSetSelection(textField, 0, lastSelect, CurrentTime);
	}
}

static void
changeBackground(Widget w, void *bg)
{
	XmChangeColor(w, (Pixel) bg);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_create(JNIEnv * env, jobject this, jobject parentComponentPeer)
{
	struct FrameData *frameData;
	struct CanvasData *parentCanvasData;
	int             argc;
	Arg             args[10];
	Widget          child;
	Pixel           bg;
	jobject         target;
	jobject         font;
	jobject         file;
	jobject         thisGlobalRef;

	if (parentComponentPeer == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	parentCanvasData =
		(struct CanvasData *) (*env)->GetIntField(env, parentComponentPeer,
					MCachedIDs.MComponentPeer_pDataFID);
	frameData = (struct FrameData *)XtCalloc(sizeof(struct FrameData), 1);

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) frameData);

	if (frameData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(parentCanvasData->comp.widget, XmNbackground, &bg, NULL);

	argc = 0;
	XtSetArg(args[argc], XmNmustMatch, False);
	argc++;
	XtSetArg(args[argc], XmNautoUnmanage, False);
	argc++;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;
	XtSetArg(args[argc], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	argc++;
	XtSetArg(args[argc], XmNresizePolicy, XmRESIZE_ANY);
	argc++;


	frameData->winData.comp.widget = XmCreateFileSelectionDialog(parentCanvasData->shell, "fildlg:", args, argc);
	frameData->winData.shell = XtParent(frameData->winData.comp.widget);

	awt_util_mapChildren(frameData->winData.shell, changeBackground, 0, (void *) bg);

	child = XmFileSelectionBoxGetChild(frameData->winData.comp.widget, XmDIALOG_HELP_BUTTON);

	if (child != 0)
		XtUnmanageChild(child);

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);

	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return;
	}

	thisGlobalRef = frameData->winData.comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		free(frameData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}

	XtAddCallback(frameData->winData.comp.widget,
		      XmNokCallback, (XtCallbackProc) FileDialog_OK, (XtPointer) thisGlobalRef);
	XtAddCallback(frameData->winData.comp.widget,
		      XmNcancelCallback, (XtCallbackProc) FileDialog_CANCEL, (XtPointer) thisGlobalRef);
	XtAddCallback(frameData->winData.comp.widget,
		      XmNunmapCallback, (XtCallbackProc) FileDialog_QUIT, (XtPointer)thisGlobalRef);


	file = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_FileDialog_fileFID);

	if (file == NULL)
		setFSBDirAndFile(frameData->winData.comp.widget, ".", "");

	else {
		char           *cfile = (char *) (*env)->GetStringUTFChars(env, file, NULL);

		setFSBDirAndFile(frameData->winData.comp.widget, ".", cfile);

		(*env)->ReleaseStringUTFChars(env, file, cfile);
	}

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_pShow(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtManageChild(frameData->winData.comp.widget);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_pHide(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (XtIsManaged(frameData->winData.comp.widget))
		XtUnmanageChild(frameData->winData.comp.widget);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_setFileEntry(JNIEnv * env, jobject this, jstring dir, jstring file)
{
	char           *cdir;
	char           *cfile;
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || dir == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	cdir = (dir == NULL) ? "" : (char *) (*env)->GetStringUTFChars(env, dir, NULL);
	cfile = (file == NULL) ? "" : (char *) (*env)->GetStringUTFChars(env, file, NULL);

	setFSBDirAndFile(frameData->winData.comp.widget, cdir, cfile);

	if (dir != NULL)
		(*env)->ReleaseStringUTFChars(env, dir, cdir);

	if (file != NULL)
		(*env)->ReleaseStringUTFChars(env, file, cfile);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_pDispose(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;
	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.comp.widget == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(frameData->winData.shell);

        /* need to call this to avoid potential core dumps */
        awt_removeEventHandlerForWidget(frameData->winData.shell,
                                 frameData->winData.comp.peerGlobalRef);

	awt_util_consumeAllXEvents(frameData->winData.shell);
	XtDestroyWidget(frameData->winData.shell);

	XtFree((char *)frameData);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_pReshape(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	XtVaSetValues(frameData->winData.shell, XmNx, (XtArgVal) x, XmNy, (XtArgVal) y, NULL);


	if(w&&h) XtVaSetValues(frameData->winData.shell, XmNwidth, w, XmNheight, h);

	AWT_UNLOCK();
}

/*
 * FileDialog can not use the setFont() method in Component, because
 * FileDialog uses XFontSet as XmNfontList, and normal Components don't use
 * it.
 */

static void
changeFont(Widget w, void *fontList)
{
	XtVaSetValues(w, XmNfontList, fontList, NULL);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFileDialogPeer_setFont(JNIEnv * env, jobject this, jobject font)
{
	struct ComponentData *componentData;
	XmFontList      fontlist;

	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	fontlist = getFontList(env, font);

	if (fontlist != NULL) {
		/*
		 * setting the fontlist in the FileSelectionBox is not good
		 * enough -- you have to set the resource for all the
		 * descendants individually
		 */
		awt_util_mapChildren(componentData->widget, changeFont, 1, (void *) fontlist);
	}

	AWT_UNLOCK();
}
