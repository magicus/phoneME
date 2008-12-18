/*
 * @(#)awt_Component.c	1.103 06/10/10
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
#include "color.h"
#include "java_awt_Color.h"
#include "java_awt_Cursor.h"
#include "java_awt_Font.h"
#include "java_awt_Point.h"
#include "java_awt_Component.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "awt_Font.h"

#include <X11/cursorfont.h>

#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MComponentPeer_cursorSetFID = (*env)->GetFieldID(env, cls, "cursorSet", "I");
	MCachedIDs.MComponentPeer_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.MComponentPeer_targetFID = (*env)->GetFieldID(env, cls, "target", "Ljava/awt/Component;");
	MCachedIDs.MComponentPeer_getZOrderPositionMID =
		(*env)->GetMethodID(env, cls, "getZOrderPosition", "()I");
	MCachedIDs.MComponentPeer_handleExposeMID = (*env)->GetMethodID(env, cls, "handleExpose", "(IIII)V");
	MCachedIDs.MComponentPeer_handleRepaintMID = (*env)->GetMethodID(env, cls, "handleRepaint", "(IIII)V");
	MCachedIDs.MComponentPeer_postEventMID =
		(*env)->GetMethodID(env, cls, "postEvent", "(Ljava/awt/AWTEvent;)V");
}

/*
 * Looking at the state variables in target, initialize the component
 * properly.
 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pInitialize(JNIEnv * env, jobject this)
{
	jobject         target;
	struct ComponentData *componentData;
	Position x, y;

	AWT_LOCK();

	componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	x = (Position) (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID);
	y = (Position) (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID);

      	XtVaSetValues(componentData->widget,
		      XmNx, x,
		      XmNy, y,
		      XmNvisual, awt_visual,
		      NULL);

	if (componentData->peerGlobalRef == NULL)
		componentData->peerGlobalRef = (*env)->NewGlobalRef(env, this);

	/*
	 * if (!XtIsSubclass(cdata->widget,xmDrawingAreaWidgetClass)) {
	 * EventMask mask = ExposureMask | FocusChangeMask;
	 * 
	 * XtAddEventHandler(cdata->widget, mask, True,
	 * awt_canvas_event_handler, thisgbl); }
	 */

	componentData->repaintPending = RepaintPending_NONE;
	componentData->cursor = 0;
	/*
	 * Fix for BugTraq ID 4186663 - Pural PopupMenus appear at the same
	 * time. This field keeps the pointer to the currently showing popup.
	 */
	componentData->activePopup = NULL;
	awt_addWidget(env, componentData->widget, XtParent(componentData->widget),
		      componentData->peerGlobalRef,
		      java_awt_AWTEvent_KEY_EVENT_MASK |
		      java_awt_AWTEvent_MOUSE_EVENT_MASK |
		      java_awt_AWTEvent_MOUSE_MOTION_EVENT_MASK |
		      java_awt_AWTEvent_FOCUS_EVENT_MASK);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pCleanup(JNIEnv * env, jobject this)
{
	struct ComponentData *componentData;

	componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	AWT_LOCK();

	if (componentData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (componentData->peerGlobalRef != NULL)
		(*env)->DeleteGlobalRef(env, componentData->peerGlobalRef);

	if (componentData->cursor != 0 && componentData->cursor != None)
		XFreeCursor(awt_display, componentData->cursor);

	XtFree((char *) componentData);
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) 0);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pDispose(JNIEnv * env, jobject this)
{
	struct ComponentData *componentData = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	AWT_LOCK();

	XtUnmanageChild(componentData->widget);

	awt_delWidget(componentData->widget);

	/*
	awt_util_consumeAllXEvents(componentData->widget);
	awt_util_cleanupBeforeDestroyWidget(componentData->widget);
	*/

	XtDestroyWidget(componentData->widget);

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pEnable(JNIEnv * env, jobject this)
{
	struct ComponentData *cdata;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_util_enable(cdata->widget);
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pDisable(JNIEnv * env, jobject this)
{
	struct ComponentData *cdata;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_util_disable(cdata->widget);
	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pShow(JNIEnv * env, jobject this)
{
	struct ComponentData *cdata;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_util_show(env, cdata->widget);

	if ((*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_cursorSetFID) == 0)
		(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_cursorSetFID,
			  awt_util_setCursor(cdata->widget, cdata->cursor));

	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pHide(JNIEnv * env, jobject this)
{
	struct ComponentData *cdata;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	awt_util_hide(env, cdata->widget);

	AWT_UNLOCK();
}

JNIEXPORT jobject JNICALL
Java_sun_awt_motif_MComponentPeer_pGetLocationOnScreen(JNIEnv * env, jobject this)
{
	struct ComponentData *cdata;
	jobject         point;
	Position        rx = 0;
	Position        ry = 0;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if ((cdata == NULL) || (cdata->widget == NULL)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return NULL;
	}
	if (!XtIsRealized(cdata->widget)) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_InternalErrorClass, NULL);
		AWT_UNLOCK();
		return NULL;
	}
	/* Translate the component to the screen coordinate system */
	XtTranslateCoords(cdata->widget, 0, 0, &rx, &ry);

	/* Construct the Point to return to the java caller */
	point = (*env)->NewObject(env,
				  MCachedIDs.java_awt_PointClass,
	    MCachedIDs.java_awt_Point_constructorMID, (jint) rx, (jint) ry);

	AWT_UNLOCK();
	return point;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pReshape(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct ComponentData *cdata;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	awt_util_reshape(env, cdata->widget, x, y, w, h);

	AWT_UNLOCK();
}

/*
 * This native method is needed because if the cursor was set before the
 * window for the component's widget has been created, it's not possible
 * truly set the cursor on the window at that moment; we need to be able to
 * call this function after the parent frame/dialog has been mapped and we
 * *know* all widget windows have been created.
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pMakeCursorVisible(JNIEnv * env, jobject this)
{
	struct ComponentData *cdata;

	AWT_LOCK();
	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_cursorSetFID, awt_util_setCursor(cdata->widget, cdata->cursor));

	AWT_UNLOCK();
}


/*
 * Fix for 4090493.  When Motif computes indicator size, it uses
 * (effectively) XmTextExtents, so the size of the indicator depends on the
 * text of the label.  The side effect is that if the label text is rendered
 * using different platform fonts (for a single Java logical font) the
 * display is inconsistent.  E.g. for 12pt font English label will have a
 * check mark, while Japanese label will not, because underlying X11 fonts
 * have different metrics.
 * 
 * The fix is to override Motif calculations for the indicatorSize and compute
 * it ourselves based on the font metrics for all the platform fonts given
 * Java font maps onto.  Every time we set XmNfontList we should set
 * XmNindicatorSize as well.
 * 
 * The logic is in awt_computeIndicatorSize that just compute the arithmetic
 * mean of platform fonts by now.  HIE should take a look at this.
 */


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pSetFont(JNIEnv * env, jobject this, jobject f)
{
	struct ComponentData *cdata;
	XmFontList      fontlist;

	if (f == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	fontlist = getFontList(env, f);

	if (fontlist != NULL) {
		XtVaSetValues(cdata->widget, XmNfontList, fontlist, NULL);
	}
	AWT_UNLOCK();
}

static void
changeForeground(Widget w, void *fg)
{
	XtVaSetValues(w, XmNforeground, fg, NULL);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pSetForeground(JNIEnv * env, jobject this, jobject c)
{
	struct ComponentData *bdata;
	Pixel           color;

	if (c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	bdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	color = awt_getColor(env, c);
	awt_util_mapChildren(bdata->widget, changeForeground, 1, (void *) color);
	AWT_UNLOCK();
}

static void
changeBackground(Widget w, void *bg)
{
	Pixel           fg;

	XtVaGetValues(w, XmNforeground, &fg, NULL);
	XmChangeColor(w, (Pixel) bg);
	XtVaSetValues(w, XmNforeground, fg, NULL);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_pSetBackground(JNIEnv * env, jobject this, jobject c)
{
	struct ComponentData *bdata;
	Pixel           color;

	if (c == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "null color");
		return;
	}
	AWT_LOCK();
	bdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	color = awt_getColor(env, c);
	awt_util_mapChildren(bdata->widget, changeBackground, 1, (void *) color);
	AWT_UNLOCK();
}

static void
setTreeTraversal(Widget w, Boolean trav)
{
	WidgetList      children;
	int             numChildren;
	int             i;

	XtVaGetValues(w, XmNchildren, &children, XmNnumChildren, &numChildren, NULL);

	for (i = 0; i < numChildren; i++) {
		if (!XtIsSubclass(children[i], xmScrollBarWidgetClass)) {
			XtVaSetValues(children[i], XmNtraversalOn, trav, NULL);
		}
		if (XtIsSubclass(children[i], xmDrawingAreaWidgetClass)) {
			setTreeTraversal(children[i], trav);
		}
	}
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_requestFocus(JNIEnv * env, jobject this)
{
	struct ComponentData *bdata;
	Boolean         result;

	AWT_LOCK();

	bdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (bdata == NULL || bdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	/*
	 * Motif won't let us set focus to a Panel/Canvas which has
	 * traversable children, so temporarily turn off traversal on it's
	 * children before requesting the focus.
	 */
	if (XtIsSubclass(bdata->widget, xmDrawingAreaWidgetClass)) {
		setTreeTraversal(bdata->widget, FALSE);
	}
	/*
	 * Fix for bug 4157017: replace XmProcessTraversal with
	 * XtSetKeyboardFocus because XmProcessTraversal does not allow focus
	 * to go to non-visible widgets.
	 * 
	 * (There is a corresponding change to awt_MToolkit.c:dispatchToWidget)
	 * 
	 * I found a last minute problem with this fix i.e. it broke the test
	 * case for bug 4053856. XmProcessTraversal does something else (that
	 * XtSetKeyboardFocus does not do) that stops this test case from
	 * failing. So, as I do not have time to investigate, and having both
	 * XmProcessTraversal and XtSetKeyboardFocus fixes 4157017 and
	 * 4053856 and should be harmless (reviewer agreed), we have both
	 * below - XmProcessTraversal AND XtSetKeyboardFocus.
	 */

	result = XmProcessTraversal(bdata->widget, XmTRAVERSE_CURRENT);
	{
		Widget          shell;
		Widget          w = bdata->widget;

		shell = w;
		while (shell && !XtIsShell(shell)) {
			shell = XtParent(shell);
		}
		XtSetKeyboardFocus(shell, w);
	}
	/* end 4157017 */

	if (XtIsSubclass(bdata->widget, xmDrawingAreaWidgetClass)) {
		setTreeTraversal(bdata->widget, TRUE);
	}
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MComponentPeer_setCursor(JNIEnv * env, jobject this, jobject cursor)
{
	Cursor          xcursor;
	struct ComponentData *cdata;
	int             cursorType;

	AWT_LOCK();

	cdata = (struct ComponentData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL || cdata->widget == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (cursor == NULL) {
		xcursor = None;
	} else {
		cursorType = (int) (*env)->GetIntField(env, cursor, MCachedIDs.java_awt_Cursor_typeFID);

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
					 "Unknown Cursor Type");
			AWT_UNLOCK();
			return;
		}
		xcursor = XCreateFontCursor(awt_display, cursorType);
	}
	if (cdata->cursor != 0 && cdata->cursor != None) {
		XFreeCursor(awt_display, cdata->cursor);
	}
	cdata->cursor = xcursor;

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_cursorSetFID,
		   (jint) awt_util_setCursor(cdata->widget, cdata->cursor));

	AWT_UNLOCK();
}


Dimension
awt_computeIndicatorSize(struct FontData * fdata)
{
	int             height;
	int             acc;
	int             i;

	if (fdata == NULL)
		return MOTIF_XmINVALID_DIMENSION;

	/*
	 * If Java font maps into single platform font - there's no problem.
	 * Let Motif use it's usual calculations in this case.
	 */
	if (fdata->charset_num == 1)
		return MOTIF_XmINVALID_DIMENSION;

	acc = 0;
	for (i = 0; i < fdata->charset_num; ++i) {
		XFontStruct    *xfont = fdata->flist[i].xfont;

		acc += xfont->ascent + xfont->descent;
	}

	height = acc / fdata->charset_num;
	if (height < MOTIF_XmDEFAULT_INDICATOR_DIM)
		height = MOTIF_XmDEFAULT_INDICATOR_DIM;

	return height;
}

Dimension
awt_adjustIndicatorSizeForMenu(Dimension indSize)
{
	if (indSize == 0 || indSize == MOTIF_XmINVALID_DIMENSION)
		return MOTIF_XmINVALID_DIMENSION;	/* let motif do the job */

	/*
	 * Indicators in menus are smaller. 2/3 is a magic number from Motif
	 * internals.
	 */
	indSize = indSize * 2 / 3;
	if (indSize < MOTIF_XmDEFAULT_INDICATOR_DIM)
		indSize = MOTIF_XmDEFAULT_INDICATOR_DIM;

	return indSize;
}
