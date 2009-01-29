/*
 * @(#)awt_Selection.c	1.42 06/10/10
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
#include "java_awt_datatransfer_Transferable.h"
#include "java_awt_datatransfer_DataFlavor.h"
#include "sun_awt_motif_X11Selection.h"
#include "sun_awt_motif_X11Clipboard.h"
#include <X11/Intrinsic.h>


static Atom     TARGETS;

static jobject  selections[10];
static int      selectionCount = 0;

enum {
	STRING = 0,
	TEXT = 1,
	COMPOUND_TEXT = 2
};

static Atom     targetList[24];

static Widget   selection_w = NULL;

Boolean         selectionProcessed = FALSE;
Boolean         gotData = FALSE;


jobject
getX11Selection(JNIEnv * env, Atom atom)
{
	int             i;

	for (i = 0; i < selectionCount; i++) {
		if (((Atom) (*env)->GetIntField(env, selections[i], MCachedIDs.X11Selection_atomFID)) == atom) {
			return selections[i];
		}
	}

	return NULL;
}

Boolean
awt_isSelectionOwner(JNIEnv * env, char *sel_str)
{
	Atom            selection;
	jobject         x11sel;

	selection = XInternAtom(awt_display, sel_str, False);

	x11sel = getX11Selection(env, selection);

	if (x11sel != NULL) {
		if ((*env)->GetObjectField(env, x11sel, MCachedIDs.X11Selection_holderFID) != NULL) {
			return TRUE;
		}
	}
	return FALSE;
}

static void     losingSelectionOwnership(Widget w, Atom * selection);

void
awt_notifySelectionLost(JNIEnv * env, char *sel_str)
{
	Atom            selection;

	selection = XInternAtom(awt_display, sel_str, False);
	losingSelectionOwnership(NULL, &selection);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Selection_init(JNIEnv * env, jclass cls)
{
	MCachedIDs.X11Selection_atomFID = (*env)->GetFieldID(env, cls, "atom", "I");
	MCachedIDs.X11Selection_contentsFID =
		(*env)->GetFieldID(env, cls, "contents", "Ljava/awt/datatransfer/Transferable;");
	MCachedIDs.X11Selection_dataFID = (*env)->GetFieldID(env, cls, "data", "Ljava/lang/Object;");
	MCachedIDs.X11Selection_holderFID =
		(*env)->GetFieldID(env, cls, "holder", "Lsun/awt/motif/X11SelectionHolder;");
	MCachedIDs.X11Selection_targetArrayFID = (*env)->GetFieldID(env, cls, "targetArray", "[I");

	MCachedIDs.X11Selection_lostSelectionOwnershipMID = 
	  (*env)->GetMethodID(env, cls, "lostSelectionOwnership", "()V");
	MCachedIDs.X11Selection_getStringDataFromOwnerMID = 
	  (*env)->GetMethodID(env, cls, "getStringDataFromOwner", "(Ljava/awt/datatransfer/DataFlavor;)Ljava/lang/String;");
       

	cls = (*env)->FindClass(env, "java/awt/datatransfer/DataFlavor");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_datatransfer_DataFlavor_atomFID = (*env)->GetFieldID(env, cls, "atom", "I");

	cls = (*env)->FindClass(env, "java/awt/datatransfer/Transferable");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_datatransfer_Transferable_getTransferDataFlavorsMID =
		(*env)->GetMethodID(env, cls, "getTransferDataFlavors", "()[Ljava/awt/datatransfer/DataFlavor;");

	AWT_LOCK();

	selection_w = XtAppCreateShell("AWTSelection", "XApplication",
			    topLevelShellWidgetClass, awt_display, NULL, 0);

	XtSetMappedWhenManaged(selection_w, False);
	XtRealizeWidget(selection_w);

	TARGETS = XInternAtom(awt_display, "TARGETS", False);

	targetList[STRING] = XInternAtom(awt_display, "STRING", False);
	targetList[TEXT] = XInternAtom(awt_display, "TEXT", False);
	targetList[COMPOUND_TEXT] = XInternAtom(awt_display, "COMPOUND_TEXT", False);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Selection_create(JNIEnv * env, jobject this, jstring name)
{
	Atom            selection;
	char           *sel_str;

	AWT_LOCK();

	if (name == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	sel_str = (char *) (*env)->GetStringUTFChars(env, name, NULL);

	if (sel_str == NULL) {
		AWT_UNLOCK();
		return;
	}
	selection = XInternAtom(awt_display, sel_str, False);
	(*env)->ReleaseStringUTFChars(env, name, sel_str);
	(*env)->SetIntField(env, this, MCachedIDs.X11Selection_atomFID, (jint) selection);
	selections[selectionCount++] = (*env)->NewGlobalRef(env, this);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Selection_pDispose(JNIEnv * env, jobject this)
{
	int             i;

	AWT_LOCK();

	for (i = 0; i < selectionCount; i++) {
		if ((*env)->IsSameObject(env, selections[i], this) == JNI_TRUE) {
			(*env)->DeleteGlobalRef(env, selections[i]);
			break;
		}
	}

	for (i++; i < selectionCount; i++) {
		selections[i - 1] = selections[i];
	}

	selectionCount--;

	AWT_UNLOCK();
}

static int
getTargetsForFlavors(JNIEnv * env, jobjectArray flavors, Atom ** targets_ret)
{
	int             num_flavors = (*env)->GetArrayLength(env, flavors);
	Atom            buf[100];
	int             count = 0;
	int             i, j;

	for (i = 0; i < num_flavors; i++) {
		Boolean         exists = False;
		Atom            target;
		jobject         flavor = (*env)->GetObjectArrayElement(env, flavors, i);

		target = (Atom) (*env)->GetIntField(env, flavor, MCachedIDs.java_awt_datatransfer_DataFlavor_atomFID);

		for (j = 0; j < count && !exists; j++) {
			if (buf[j] == target) {
				exists = True;
			}
		}

		if (target != 0 && !exists) {
			buf[count++] = target;
			/*
			 * CONPOUND_TEXT target is also for the stringFlavor
			 */
			if (target == targetList[STRING]) {
				buf[count++] = targetList[COMPOUND_TEXT];
			}
		}
	}

	if (count > 0) {
		*targets_ret = (Atom *) XtMalloc(count * sizeof(Atom));

		for (i = 0; i < count; i++) {
			(*targets_ret)[i] = buf[i];
		}
	}
	return count;
}

static          Boolean
provideSelectionData(Widget w, Atom * selection, Atom * target, Atom * type,
		     XtPointer * value, unsigned long *length, int *format)
{
	JNIEnv         *env;
	jobject         this;
	jobject         contents = 0;
	jobject         flavor = NULL;
	jobjectArray    flavors;
	Atom           *targets;
	int             i, j, num_flavors, count;


	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return FALSE;

	this = getX11Selection(env, *selection);

	if (this == NULL) {
		return FALSE;
	}
	contents = (*env)->GetObjectField(env, this, MCachedIDs.X11Selection_contentsFID);

	if (contents == NULL) {
		return FALSE;
	}
	AWT_UNLOCK();
	flavors =
		(jobjectArray) (*env)->CallObjectMethod(env, contents,
							MCachedIDs.
	      java_awt_datatransfer_Transferable_getTransferDataFlavorsMID);
	AWT_LOCK();

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);

	num_flavors = (*env)->GetArrayLength(env, flavors);
	count = getTargetsForFlavors(env, flavors, &targets);

	if (*target == TARGETS) {
		/*
		 * printf("ConvertRequest: TARGETS\n");
		 */
		if (count == 0) {
			return FALSE;
		}
		*type = XA_ATOM;
		*format = 32;
		*value = (XtPointer) targets;
		*length = count;
		return TRUE;
	}
	for (i = 0; i < count; i++) {
		if (*target == targets[i]) {
			for (j = 0; j < num_flavors; j++) {
				jobject         f = (*env)->GetObjectArrayElement(env, flavors, j);
				Atom            atom =

				(Atom) (*env)->GetIntField(env, f, MCachedIDs.java_awt_datatransfer_DataFlavor_atomFID);

				if (atom == *target) {
					flavor = f;
					break;
				}
				if (atom == targetList[STRING] && *target == targetList[COMPOUND_TEXT]) {
					flavor = f;
					break;
				}
			}

			if (flavor == NULL)
				return FALSE;

			if (*target == targetList[STRING] || *target == targetList[COMPOUND_TEXT]) {
				jstring         strData;
				char           *str;
				char           *data;
				int             len;

				AWT_UNLOCK();
				strData =
					(jstring) (*env)->CallObjectMethod(env, this,
									   MCachedIDs.X11Selection_getStringDataFromOwnerMID,
								    flavor);
				AWT_LOCK();

				if ((*env)->ExceptionCheck(env))
					(*env)->ExceptionDescribe(env);

				if (strData == NULL)
					return FALSE;

				str = (char *) (*env)->GetStringUTFChars(env, strData, NULL);

				if (*target == targetList[STRING]) {
					len = strlen(str);
					data = (char *) XtMalloc(len * sizeof(char));

					strcpy(data, str);
					(*env)->ReleaseStringUTFChars(env, strData, str);

					*type = XA_STRING;
					*format = 8;
					*value = (XtPointer) data;
					*length = len;

					return TRUE;
				} else {
					XTextProperty   text_prop_return;

					if (XmbTextListToTextProperty(awt_display, &str, 1, XCompoundTextStyle,
					    &text_prop_return) == Success) {

						*type = text_prop_return.encoding;
						*format = 8;
						*value = (XtPointer) (text_prop_return.value);
						*length = strlen((char *) text_prop_return.value);

						(*env)->ReleaseStringUTFChars(env, strData, str);

						return TRUE;
					}
					(*env)->ReleaseStringUTFChars(env, strData, str);
				}
			} else if (*target == targetList[TEXT]) {
			} else {
			}
		}
	}

	return FALSE;
}

static char    *data;

static void
transferDone(Widget w, Atom * selection, Atom * target)
{
	XtFree(data);
	data = NULL;
}

static void
losingSelectionOwnership(Widget w, Atom * selection)
{
	JNIEnv         *env;
	jobject         this;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	this = getX11Selection(env, *selection);

	(*env)->CallVoidMethod(env, this, MCachedIDs.X11Selection_lostSelectionOwnershipMID);
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_motif_X11Selection_pGetSelectionOwnership(JNIEnv * env, jobject this)
{
	Atom            selection;
	Boolean         gotit;

	AWT_LOCK();

	selection = (Atom) (*env)->GetIntField(env, this, MCachedIDs.X11Selection_atomFID);

	gotit = XtOwnSelection(selection_w, selection, XtLastTimestampProcessed(awt_display),
	      provideSelectionData, losingSelectionOwnership, transferDone);

	AWT_UNLOCK();

	return (gotit ? JNI_TRUE : JNI_FALSE);
}


static void
getSelectionTargets(Widget w, XtPointer client_data, Atom * selection, Atom * type,
		    XtPointer value, unsigned long *length, int *format)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;
	jintArray       targetArray;
	int             count = *length;
	jint           *atoms;
	int             i;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if (*type == TARGETS) {
		Atom           *targets = (Atom *) value;

		if (count > 0) {
			targetArray = (*env)->NewIntArray(env, count);

			if (targetArray == NULL)
				return;

			atoms = (*env)->GetIntArrayElements(env, targetArray, NULL);

			for (i = 0; i < count; i++) {
				*atoms++ = (jint) targets[i];
			}

			(*env)->ReleaseIntArrayElements(env, targetArray, atoms, 0);
			(*env)->SetObjectField(env, this, MCachedIDs.X11Selection_targetArrayFID, targetArray);
		}
	}
	selectionProcessed = TRUE;
}

static void
getSelectionData(Widget w, XtPointer client_data, Atom * selection, Atom * type,
		 XtPointer value, unsigned long *length, int *format)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;
	int             count = *length;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if (*type == targetList[STRING]) {
		jstring         jstr;
		char           *str = (char *) value;

		jstr = (*env)->NewStringUTF(env, str);
		(*env)->SetObjectField(env, this, MCachedIDs.X11Selection_dataFID, jstr);
		gotData = TRUE;
	} else if (*type == targetList[COMPOUND_TEXT]) {
		jstring         jstr;
		XTextProperty   text_prop;
		char          **list_return;
		int             count_return;

		text_prop.value = (unsigned char *) value;
		text_prop.encoding = *type;
		text_prop.format = 8;
		text_prop.nitems = count;
		XmbTextPropertyToTextList(awt_display, &text_prop, &list_return, &count_return);

		if (list_return != NULL && count_return > 0) {
			jstr = (*env)->NewStringUTF(env, list_return[0]);
			XFreeStringList(list_return);
			list_return = NULL;
			(*env)->SetObjectField(env, this, MCachedIDs.X11Selection_dataFID, jstr);
			gotData = TRUE;
		}
	}
	selectionProcessed = TRUE;
}

/* TODO: verify the need of this function */
#if 0 
static int
WaitForSelectionEvent(void *data)
{
	return selectionProcessed;
}
#endif 

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Selection_pGetTransferTargets(JNIEnv * env, jobject this)
{
	Atom            selection;
	/* TODO: check the need of this unused variable
          int             modalNum;
        */

	AWT_LOCK();

	selection = (Atom) (*env)->GetIntField(env, this, MCachedIDs.X11Selection_atomFID);
	selectionProcessed = FALSE;
	XtGetSelectionValue(selection_w, selection, TARGETS, getSelectionTargets,
		   (XtPointer) this, XtLastTimestampProcessed(awt_display));

	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Selection_pGetTransferData(JNIEnv * env, jobject this, jintArray targets)
{
	Atom            selection;
	jint           *atoms;
	int             count;
	int             i;

	AWT_LOCK();

	selection = (Atom) (*env)->GetIntField(env, this, MCachedIDs.X11Selection_atomFID);
	gotData = FALSE;
	atoms = (*env)->GetIntArrayElements(env, targets, NULL);
	count = (*env)->GetArrayLength(env, targets);

	for (i = 0; i < count && !gotData; i++) {
            /* TODO: check the need of this unused variable
              int             modalNum;
            */
		selectionProcessed = FALSE;
		XtGetSelectionValue(selection_w, selection, (Atom) atoms[i], getSelectionData,
		   (XtPointer) this, XtLastTimestampProcessed(awt_display));
	}

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11Selection_registerTargetForFlavor(JNIEnv * env, jobject this,
				       jobject flavor, jstring targetString)
{
	Atom            target;
	char           *ts;

	AWT_LOCK();

	if (targetString == NULL || flavor == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	ts = (char *) (*env)->GetStringUTFChars(env, targetString, NULL);
	target = XInternAtom(awt_display, ts, False);
	(*env)->ReleaseStringUTFChars(env, targetString, ts);
	(*env)->SetIntField(env, flavor, MCachedIDs.java_awt_datatransfer_DataFlavor_atomFID, target);

	AWT_UNLOCK();
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_X11Selection_getAtomForTarget(JNIEnv * env, jobject this, jstring targetString)
{
	Atom            target;
	char           *target_str;

	AWT_LOCK();

	if (targetString == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return -1;
	}
	target_str = (char *) (*env)->GetStringUTFChars(env, targetString, NULL);
	target = XInternAtom(awt_display, target_str, False);
	(*env)->ReleaseStringUTFChars(env, targetString, target_str);

	AWT_UNLOCK();
	return (jint) target;
}
