/*
 * @(#)awt_Choice.c	1.68 06/10/10
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
#include "awt_Font.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MChoicePeer.h"

#include "color.h"
#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MChoicePeer_actionMID = (*env)->GetMethodID(env, cls, "action", "(I)V");
}

static void
Choice_callback(Widget menu_item, XtPointer client_data, XmAnyCallbackStruct * cbs)
{
	JNIEnv         *env;
	jint            index;

	XtVaGetValues(menu_item, XmNuserData, &index, NULL);

	/* index stored in user-data is 1-based instead of 0-based because */
	/* of a bug in XmNuserData */

	index--;

	(*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL);

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MChoicePeer_actionMID, index);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_create(JNIEnv * env, jobject this, jobject parent)
{
	struct ChoiceData *choiceData;
	struct ComponentData *parentChoiceData;
	Arg             args[30];
	int             argc;
	Pixel           bg;
	Pixel           fg;

	Widget          label;

	jobject         thisGlobalRef;

	AWT_LOCK();


	parentChoiceData = (struct ComponentData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);

	if (parentChoiceData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	choiceData = (struct ChoiceData *)XtCalloc(1, sizeof(struct ChoiceData));

	if (choiceData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) choiceData);

	/*
	choiceData->items = 0;
	choiceData->maxitems = 0;
	choiceData->n_items = 0;
	*/


	XtVaGetValues(parentChoiceData->widget, XmNbackground, &bg, NULL);
	XtVaGetValues(parentChoiceData->widget, XmNforeground, &fg, NULL);

	argc = 0;
	/*
	XtSetArg(args[argc], XmNx, 0);
	argc++;
	XtSetArg(args[argc], XmNy, 0);
	argc++;
	*/
	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNforeground, fg);
	argc++;

	choiceData->menu = XmCreatePulldownMenu(parentChoiceData->widget, "drpdwn:", args, argc);

	thisGlobalRef = choiceData->comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		XtFree((char *) choiceData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}

	argc = 0;
	XtSetArg(args[argc], XmNx, 0);
	argc++;
	XtSetArg(args[argc], XmNy, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;

	/*
	XtSetArg(args[argc], XmNspacing, 0);
	argc++;

	XtSetArg(args[argc], XmNresizeHeight, False);
	argc++;
	XtSetArg(args[argc], XmNresizeWidth, False);
	argc++;
	*/

	XtSetArg(args[argc], XmNborderWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNnavigationType, XmTAB_GROUP);
	argc++;
	XtSetArg(args[argc], XmNtraversalOn, True);
	argc++;
	XtSetArg(args[argc], XmNorientation, XmVERTICAL);
	argc++;

	/*
	XtSetArg(args[argc], XmNadjustMargin, False);
	argc++;
	*/
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNforeground, fg);
	argc++;
	XtSetArg(args[argc], XmNsubMenuId, choiceData->menu);
	argc++;
	XtSetArg(args[argc], XmNuserData, (XtPointer) thisGlobalRef);
	argc++;

	choiceData->comp.widget = XmCreateOptionMenu(parentChoiceData->widget, "drpopt:", args, argc);

	label = XmOptionLabelGadget(choiceData->comp.widget);
	if (label != 0) XtUnmanageChild(label);


	 /* 
	 * XtSetMappedWhenManaged(choiceData->comp.widget, False);
	 */

	XtManageChild(choiceData->comp.widget);

	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_paddItem(JNIEnv * env, jobject this, jbyteArray item, jint index)
{
	struct ChoiceData *odata;
	Widget          bw;
	Arg             args[10];
	int             argc;
	Pixel           bg, fg;

	XmString        xim;
	XmFontList      fontlist;

	jobject         font, target;

	jbyte          *bytereg;
	int             blen;

	if (item == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	font = (*env)->CallObjectMethod(env, target, MCachedIDs.java_awt_Component_getFontMID);
	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return;
	}
	odata = (struct ChoiceData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (odata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (odata->maxitems == 0 || (index > (odata->maxitems - 1))) {
		odata->maxitems += 20;
		if (odata->n_items > 0) {
			odata->items = (Widget *)XtRealloc((char *) odata->items, odata->maxitems * sizeof(Widget));
		} else {
			odata->items = (Widget *) XtCalloc(sizeof(Widget), odata->maxitems);
		}
		if (odata->items == 0) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
			AWT_UNLOCK();
			return;
		}
	}
	XtVaGetValues(odata->comp.widget, XmNbackground, &bg, NULL);
	XtVaGetValues(odata->comp.widget, XmNforeground, &fg, NULL);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, bg);
	argc++;
	XtSetArg(args[argc], XmNforeground, fg);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNuserData, index + 1);
	argc++;

	blen = (*env)->GetArrayLength(env, item);
	bytereg = (jbyte *) XtCalloc(blen + 1, sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, item, 0, blen, bytereg);
	xim = XmStringCreateLocalized((char *) bytereg);
	XtFree((char *)bytereg);

	fontlist = getFontList(env, font);

	if (fontlist != NULL) {
		XtSetArg(args[argc], XmNfontList, fontlist);
		argc++;
	}
	XtSetArg(args[argc], XmNlabelString, xim);
	argc++;

	bw = XmCreatePushButton(odata->menu, "chobtn:", args, argc);

	XtAddCallback(bw, XmNactivateCallback, (XtCallbackProc) Choice_callback, (XtPointer) odata->comp.peerGlobalRef);
	odata->items[index] = bw;
	odata->n_items++;

	if (xim != NULL)
		XmStringFree(xim);

	XtManageChild(bw);
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_select(JNIEnv * env, jobject this, jint index)
{
	struct ChoiceData *odata;

	AWT_LOCK();

	odata = (struct ChoiceData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	if (odata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (index > odata->n_items || index < 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(odata->comp.widget, XmNmenuHistory, odata->items[index], NULL);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_setFont(JNIEnv * env, jobject this, jobject f)
{
	struct ChoiceData *cdata;
	XmFontList      fontlist;

	if (f == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	cdata = (struct ChoiceData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	fontlist = getFontList(env, f);

	if (fontlist != NULL) {
		XtVaSetValues(cdata->comp.widget, XmNfontList, fontlist, NULL);
		XtVaSetValues(cdata->menu, XmNfontList, fontlist, NULL);
	}
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_setBackground(JNIEnv * env, jobject this, jobject c)
{
	struct ChoiceData *bdata;
	Pixel           bg, fg;
	int             i;

	if (c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "null color");
		return;
	}
	AWT_LOCK();

	bdata = (struct ChoiceData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	/* Get background color */
	bg = awt_getColor(env, c);

	/*
	 * XmChangeColor(), in addtion to changing the background and
	 * selection colors, also changes the foreground color to be what it
	 * thinks should be. However, we want to use the color that gets set
	 * by setForeground() instead. We therefore need to save the current
	 * foreground color here, and then set it again after the
	 * XmChangeColor() occurs.
	 */
	XtVaGetValues(bdata->comp.widget, XmNforeground, &fg, NULL);

	/* Set color */
	XmChangeColor(bdata->comp.widget, bg);
	XtVaSetValues(bdata->comp.widget, XmNforeground, fg, NULL);

	XmChangeColor(bdata->menu, bg);
	XtVaSetValues(bdata->menu, XmNforeground, fg, NULL);

	for (i = 0; i < bdata->n_items; i++) {
		XmChangeColor(bdata->items[i], bg);
		XtVaSetValues(bdata->items[i], XmNforeground, fg, NULL);
	}
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_setForeground(JNIEnv * env, jobject this, jobject c)
{
	struct ChoiceData *bdata;
	Pixel           color;
	int             i;

	if (c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	bdata = (struct ChoiceData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	color = awt_getColor(env, c);

	XtVaSetValues(bdata->comp.widget, XmNforeground, color, NULL);
	XtVaSetValues(bdata->menu, XmNforeground, color, NULL);

	for (i = 0; i < bdata->n_items; i++) {
		XtVaSetValues(bdata->items[i], XmNforeground, color, NULL);
	}

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_pReshape(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct ComponentData *cdata;
	Widget          button;


	if((w>0)&&(h>0))
	  {
	AWT_LOCK();
	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	button = XmOptionButtonGadget(cdata->widget);

	awt_util_reshape(env, cdata->widget, x, y, w, h);


	/*	awt_util_reshape(env, button, x, y, w, h); */
	  XtVaSetValues(button, XmNx, (Position) x,
		      XmNy, (Position) y,
		      XmNwidth, (Dimension) w,
		      XmNheight, (Dimension) h,
		      NULL);

	AWT_UNLOCK();
	  }
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MChoicePeer_remove(JNIEnv * env, jobject this, jint index)
{
	struct ChoiceData *cdata;
	int             i;

	AWT_LOCK();

	cdata = (struct ChoiceData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->comp.widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (index < 0 || index > cdata->n_items) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_IllegalArgumentExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(cdata->items[index]);
	awt_util_consumeAllXEvents(cdata->items[index]);
	awt_util_cleanupBeforeDestroyWidget(cdata->items[index]);
	XtDestroyWidget(cdata->items[index]);

	for (i = index; i < cdata->n_items - 1; i++) {
		cdata->items[i] = cdata->items[i + 1];
		/*
		 * need to reset stored index value, (adding 1 to
		 * disambiguate it
		 */
		/* from an arg list terminator)                   */
		/* bug fix 4079027 robi.khan@eng                  */
		XtVaSetValues(cdata->items[i], XmNuserData, i + 1, NULL);
	}
	cdata->items[cdata->n_items - 1] = NULL;
	cdata->n_items--;
	AWT_UNLOCK();
}
