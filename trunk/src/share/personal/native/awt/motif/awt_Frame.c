/*
 * @(#)awt_Frame.c	1.156 06/10/10
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

#include <X11/Shell.h>
#include <X11/IntrinsicP.h>
#include <X11/Xmu/Editres.h>
#include <Xm/VendorS.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>


#include "java_awt_Frame.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include "java_awt_Insets.h"
#include "java_awt_Image.h"
#include "java_awt_Font.h"

#ifdef PERSONAL
#include "java_awt_MenuBar.h"
#endif

#include "sun_awt_motif_MFramePeer.h"
#include "sun_awt_motif_MComponentPeer.h"

#ifdef PERSONAL
#include "sun_awt_motif_MMenuBarPeer.h"
#include "sun_awt_EmbeddedFrame.h"
#include "sun_awt_motif_MEmbeddedFrame.h"
#include "sun_awt_motif_MEmbeddedFramePeer.h"
#endif

#include "canvas.h"
#include "image.h"
#include "awt_util.h"
#include "multi_font.h"


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MFramePeer_insetsFID = (*env)->GetFieldID(env, cls, "insets", "Ljava/awt/Insets;");
	MCachedIDs.MFramePeer_handleActivateMID = (*env)->GetMethodID(env, cls, "handleActivate", "()V");
	MCachedIDs.MFramePeer_handleDeactivateMID = (*env)->GetMethodID(env, cls, "handleDeactivate", "()V");
	MCachedIDs.MFramePeer_handleDeiconifyMID = (*env)->GetMethodID(env, cls, "handleDeiconify", "()V");
	MCachedIDs.MFramePeer_handleIconifyMID = (*env)->GetMethodID(env, cls, "handleIconify", "()V");
	MCachedIDs.MFramePeer_handleMovedMID = (*env)->GetMethodID(env, cls, "handleMoved", "(II)V");
	MCachedIDs.MFramePeer_handleResizeMID = (*env)->GetMethodID(env, cls, "handleResize", "(II)V");
	MCachedIDs.MFramePeer_handleQuitMID = (*env)->GetMethodID(env, cls, "handleQuit", "()V");

	if (MCachedIDs.java_awt_Window_warningStringFID == NULL) {
		cls = (*env)->FindClass(env, "java/awt/Window");

		if (cls == NULL)
			return;

		MCachedIDs.java_awt_Window_warningStringFID =
			(*env)->GetFieldID(env, cls, "warningString", "Ljava/lang/String;");
	}
	cls = (*env)->FindClass(env, "java/awt/Frame");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Frame_mbManagementFID = (*env)->GetFieldID(env, cls, "mbManagement", "Z");
	MCachedIDs.java_awt_Frame_resizableFID = (*env)->GetFieldID(env, cls, "resizable", "Z");
}

/*
 * Since query XmNiconic will not give reliable state, we have to use the
 * following alternative function to return one of IconicState | NormalState
 * | WithdrawnState | NULL.
 * 
 * Note: memory free task of pState is left to user.
 */
#define WM_STATE_ELEMENTS 1

static unsigned long *
getWindowState(Window win)
{
	unsigned long  *pState = NULL;
	unsigned long   nitems, leftover;
	Atom            xa_WM_STATE, actual_type;
	int             actual_format, status;

	xa_WM_STATE = XInternAtom(awt_display, "WM_STATE", False);
	status = XGetWindowProperty(awt_display, win,
				    xa_WM_STATE, 0L, WM_STATE_ELEMENTS,
			   False, xa_WM_STATE, &actual_type, &actual_format,
			    &nitems, &leftover, (unsigned char **) &pState);

	if (!((status == Success) && (actual_type == xa_WM_STATE) && (nitems == WM_STATE_ELEMENTS))) {
		if (pState) {
			XtFree((char *) pState);
			pState = NULL;
		}
	}
	return (pState);
}

/* changeInsets() sets target's insets equal to X/Motif values. */
static void
changeInsets(JNIEnv * env, jobject this, struct FrameData * wdata)
{
	jobject         insets;

	if (this == NULL) {
		return;
	}
insets = (*env)->GetObjectField(env, this, MCachedIDs.MFramePeer_insetsFID);

	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_topFID, (jint) wdata->top);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_leftFID, (jint) wdata->left);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_bottomFID, (jint) wdata->bottom);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_rightFID, (jint) wdata->right);
}

/*
 * setMbAndWwHeightAndOffsets() attempts to establish the heights of frame's
 * menu bar and warning window (if present in frame).
 * setMbAndWwHeightAndOffsets() also adjusts appropriately the X/Motif
 * offsets and calls changeInsets() to set target insets. A warning window,
 * if present, is established during ...create(). wdata->warningWindow is set
 * there, wdata->wwHeight is set here. Routine pSetMenuBar() sets value of
 * the wdata->menuBar field. This routine reads that value. If it is not
 * null, a menubar has been added.  In this case, calculate the current
 * height of the menu bar.  This may be a partial (incomplete) menubar
 * because ths routine may be called before the X/Motif menubar is completely
 * realized. In this case, the menubar height may be adjusted incrementally.
 * This routine may be called from ...pSetMenuBar(), innerCanvasEH(), and
 * ...pReshape(). It is designed to (eventually) obtain the correct menubar
 * height. On the other hand, if wdata->menuBar is NULL and the stored
 * menubar height is not zero, then we subtract off the height.
 */

static void
setMbAndWwHeightAndOffsets(JNIEnv * env, jobject this, struct FrameData * wdata)
{
	Dimension       warningHeight,	/* Motif warning window height  */
	                labelHeight;	/* Motif warning label's height */
	WidgetList      warningChildrenWL;	/* warning children widgets  */

	Dimension       menuBarWidth,	/* Motif menubar width      */
	                menuBarHeight,	/* Motif menubar height     */
	                menuBarBorderSize,	/* Motif menubar border size */
	                marginHeight,	/* Motif menubar margin height  */
	                menuHeight,	/* Motif menubar's menu height  */
	                menuBorderSize,	/* Motif menu border size   */
	                actualHeight;	/* height: menu+margins+borders */

	WidgetList      menuBarChildrenWL;	/* menubar children widgets  */
	Cardinal        numberChildren;	/* number of menubar children   */

	/*
	 * If warning window height not yet known, try to get it now. It will
	 * be added to top or bottom (iff NETSCAPE) offset.
	 */
	if (wdata->warningWindow != NULL) {
		XtVaGetValues(wdata->warningWindow,
			      XmNheight, &warningHeight,
			      XmNchildren, &warningChildrenWL, XmNnumChildren, &numberChildren, NULL);

		/*
		 * We may be doing this before warning window is realized !
		 * So, check for a child label in the warning. If its height
		 * is not yet accounted for in the warning height, then use
		 * it here.
		 */
		if (numberChildren != 0) {
			XtVaGetValues(warningChildrenWL[0], XmNheight, &labelHeight, NULL);
			if (warningHeight < labelHeight) {
				warningHeight = labelHeight;
			}
		}
		if (wdata->wwHeight < warningHeight) {
#ifdef NETSCAPE
			wdata->bottom += (warningHeight - wdata->wwHeight);
#else
			wdata->top += (warningHeight - wdata->wwHeight);
#endif				/* NETSCAPE */
			changeInsets(env, this, wdata);
			wdata->wwHeight = warningHeight;
		}
	}
	/* Now we adjust offsets for an added or removed menu bar   */
	if (wdata->menuBar != NULL) {
		XtVaGetValues(wdata->menuBar,
			      XmNwidth, &menuBarWidth,
			      XmNheight, &menuBarHeight,
			      XmNchildren, &menuBarChildrenWL,
			      XmNnumChildren, &numberChildren,
			      XmNborderWidth, &menuBarBorderSize, XmNmarginHeight, &marginHeight, NULL);

		/*
		 * We may be doing this before menu bar is realized ! Hence,
		 * check for a menu in the menu bar. If its height is not yet
		 * accounted for in the menu bar height, then add it in here.
		 */
		if (numberChildren != 0) {
			XtVaGetValues(menuBarChildrenWL[0],
				      XmNheight, &menuHeight, XmNborderWidth, &menuBorderSize, NULL);

			/*
			 * Calculate real height of menu bar by adding height
			 * of its child menu and borders, margins, and the
			 * menu bar borders
			 */
			actualHeight = menuHeight + (2 * menuBorderSize) + (2 * marginHeight) + (2 * menuBarBorderSize);
			if (menuBarHeight < actualHeight) {
				menuBarHeight = actualHeight;
			}
		}
		if (wdata->mbHeight < menuBarHeight) {
			/*
			 * Adjust the (partially) added menu bar height, top
			 * offset.
			 */
			wdata->top += (menuBarHeight - wdata->mbHeight);
			changeInsets(env, this, wdata);
			wdata->mbHeight = menuBarHeight;
		}
	} else if ((wdata->menuBar == NULL) && (wdata->mbHeight > 0)) {
		/*
		 * A menu bar has been removed; subtract height from top
		 * offset.
		 */
		wdata->top -= wdata->mbHeight;
		changeInsets(env, this, wdata);
		wdata->mbHeight = 0;
	}
}

#if 0
static void
reshape(JNIEnv * env, jobject this, struct FrameData * wdata, jint x, jint y, jint w, jint h)
{
  if((w==0)||(h==0)) return; 

	XtVaSetValues(wdata->winData.shell,
		      XmNx, x,
		      XmNy, y,
		      XmNwidth, w,
		      XmNheight, h,
		      NULL);

}

#endif

#define reshape(env, this, wd, x, y, w, h) awt_util_reshape(env, wd->winData.shell, x, y, w, h)

/*
 * shellEH() is event handler for the Motif shell widget. It handles focus
 * change, map notify, configure notify events for the shell. Please see
 * internal comments pertaining to these specific events.
 */

static void
shellEH(Widget w, XtPointer client_data, XEvent * event, Boolean * cont)
{
	JNIEnv         *env;

	struct FrameData *wdata;
	jobject         target;
	jobject         this = (jobject) client_data;


	extern Boolean  awt_isModal();
	extern Boolean  awt_isWidgetModal(Widget w);


	/*
	 * Any event handlers which take peer instance pointers as
	 * client_data should check to ensure the widget has not been marked
	 * as destroyed as a result of a dispose() call on the peer (which
	 * can result in the peer instance pointer already haven been gc'd by
	 * the time this event is processed)
	 */
	if (w->core.being_destroyed) {
		return;
	}
	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	if (wdata == NULL) {
		return;
	}
	switch (event->type) {
	case FocusOut:
		if (event->xfocus.mode == NotifyNormal && (!awt_isModal() || awt_isWidgetModal(w))) {
			awt_setDeactivatedShell(w);

			(*env)->CallVoidMethod(env, this, MCachedIDs.MFramePeer_handleDeactivateMID);
		}
		break;

	case FocusIn:
		if (event->xfocus.mode == NotifyNormal && (!awt_isModal() || awt_isWidgetModal(w))) {
			awt_setActivatedShell(w);

			(*env)->CallVoidMethod(env, this, MCachedIDs.MFramePeer_handleActivateMID);
		}
		break;

	case MapNotify:
		if (wdata->mappedOnce == 0) {
		        jint width, height;

			wdata->mappedOnce = 1;

			target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

			wdata->top = wdata->left = wdata->bottom = wdata->right = 0;
			wdata->fixInsets = 0;
			wdata->fixTargetSize = 0;

			changeInsets(env, this, wdata);

			width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
			height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

                        if(width==0)width=10;
			if(height==0)height=10;

			reshape(env, this, wdata,
				(*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID),
				(*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID),
				width, height);

			(*env)->CallVoidMethod(env, this, MCachedIDs.MPanelPeer_makeCursorsVisibleMID);

		} else {
			unsigned long  *pState = getWindowState(XtWindow(wdata->winData.shell));

			if (pState != NULL && wdata->isIconic && *pState == NormalState) {
				wdata->isIconic = False;

				(*env)->CallVoidMethod(env, this, MCachedIDs.MFramePeer_handleDeiconifyMID);
			}
			XtFree((char *) pState);
		}

		break;

	case UnmapNotify:
		{
			unsigned long  *pState = getWindowState(XtWindow(wdata->winData.shell));

			if (pState != NULL && wdata->isShowing && !wdata->isIconic && *pState == IconicState) {
				wdata->isIconic = True;

				(*env)->CallVoidMethod(env, this, MCachedIDs.MFramePeer_handleIconifyMID);

			}
			XtFree((char *) pState);
		}
		break;

	case ReparentNotify:
		{
			jint            width, height;
			Position        screenX, screenY;

			target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

			XtTranslateCoords(w, 0, 0, &screenX, &screenY);

			(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_xFID, (jint)screenX);
			(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_yFID, (jint)screenY);
			width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
			height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

                        if(width==0)width=10;
			if(height==0)height=10;

			reshape(env, this, wdata, screenX, screenY, width, height);
		}
		break;

	case ConfigureNotify:

		/*
		 * We can detect the difference between a move and a resize
		 * by checking the send_event flag on the event; if it's
		 * true, then it's indeed a move, if it's false, then this is
		 * a resize and we do not want to process it as a "move" (for
		 * resizes the x,y values are misleadingly set to 0,0, and so
		 * just checking for an x,y delta won't work).
		 */

		if (event->xconfigure.send_event == True) {
			Position            x, y;

	x = event->xconfigure.x;
	y = event->xconfigure.y;

			target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

			(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_xFID, (jint)x);
			(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_yFID, (jint)y);

			(*env)->CallVoidMethod(env, this, MCachedIDs.MFramePeer_handleMovedMID, (jint)x, (jint)y);
		} else {

	Dimension       width, height;

	width = event->xconfigure.width;
	height = event->xconfigure.height;

	if(wdata->menuBar) {
	  Dimension mh;
	  XtVaGetValues(wdata->menuBar, XmNheight, &mh, NULL);
	  height-=mh;
	}


	target = (*env)->GetObjectField(env, (jobject) client_data, MCachedIDs.MComponentPeer_targetFID);

	/*	XtVaGetValues(w, XmNwidth, &width, XmNheight, &height, NULL); */

	(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_widthFID, (jint) width);
	(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_heightFID, (jint) height);

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MFramePeer_handleResizeMID,
			       (jint) width, (jint) height);

		}
		break;

	default:
		break;
	}
}


static void
Frame_quit(Widget w, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MFramePeer_handleQuitMID);
}

/* TODO : verify the need of these functions */
#if 0 
static void
Frame_activate(Widget w, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	awt_setActivatedShell(w);

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MFramePeer_handleActivateMID);
}


static void
Frame_resize(Widget wd, XtPointer client_data, XtPointer call_data)
{
#if 0
	JNIEnv         *env = NULL;

	XmDrawingAreaCallbackStruct *data = (XmDrawingAreaCallbackStruct *)call_data;
	jobject         target;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if(data->event==NULL) return;
	if(data->event->type!=ConfigureNotify) return;

	target = (*env)->GetObjectField(env, (jobject) client_data, MCachedIDs.MComponentPeer_targetFID);

	(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_widthFID, (jint) data->event->xconfigure.width);
	(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_heightFID, (jint) data->event->xconfigure.height);

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MFramePeer_handleResizeMID,
			       (jint) data->event->xconfigure.width,
			       (jint) data->event->xconfigure.height);

#endif
}
#endif 


static void
setDeleteCallback(jobject this, struct FrameData * wdata)
{
	Atom            xa_WM_DELETE_WINDOW;
	Atom            xa_WM_PROTOCOLS;

	XtVaSetValues(wdata->winData.shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
	xa_WM_DELETE_WINDOW = XmInternAtom(XtDisplay(wdata->winData.shell), "WM_DELETE_WINDOW", False);

	xa_WM_PROTOCOLS = XmInternAtom(XtDisplay(wdata->winData.shell), "WM_PROTOCOLS", False);

	XmAddProtocolCallback(wdata->winData.shell,
	xa_WM_PROTOCOLS, xa_WM_DELETE_WINDOW, Frame_quit, (XtPointer) this);

	/*
	 * Now using FocusIn event in the shellEH function.  OpenLook Window
	 * Managers do not appear to adhere to the ICCCM protocol when it
	 * comes to WM_TAKE_FOCUS xa_WM_TAKE_FOCUS =
	 * XmInternAtom(XtDisplay(wdata->winData.shell), "WM_TAKE_FOCUS",
	 * False); XmAddProtocolCallback(wdata->winData.shell,
	 * xa_WM_PROTOCOLS, xa_WM_TAKE_FOCUS, Frame_activate,
	 * (XtPointer)this);
	 */

}


/* sun_awt_motif_MFramePeer_create() is native (X/Motif) create routine	 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pCreate(JNIEnv * env, jobject this, jobject parent, jobject arg, jobject font)
{
	Arg             args[40];
	int             argc;
	struct FrameData *wdata;
	int             isEmbedded;

	XmFontList      fontList;

	jobject         target, /* insets, */ thisGlobalRef;
	jstring         warnstr;
	/*	jint            top, bottom, left, right; */

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	/*
	insets = (*env)->GetObjectField(env, this, MCachedIDs.MFramePeer_insetsFID);

	top = (*env)->GetIntField(env, arg, MCachedIDs.java_awt_Insets_topFID);
	left = (*env)->GetIntField(env, arg, MCachedIDs.java_awt_Insets_leftFID);
	bottom = (*env)->GetIntField(env, arg, MCachedIDs.java_awt_Insets_bottomFID);
	right = (*env)->GetIntField(env, arg, MCachedIDs.java_awt_Insets_rightFID);

	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_topFID, top);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_leftFID, left);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_bottomFID, bottom);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_rightFID, right);
	*/

	wdata = (struct FrameData *)XtCalloc(1, sizeof(struct FrameData));

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) wdata);

	if (wdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}

	fontList = getFontList(env, font);

	/*
	 * A variation on Netscape's hack for embedded frames: the client
	 * area of the browser is a Java Frame for parenting purposes, but
	 * really a Motif child window
	 */

	wdata->isModal = 0;
	wdata->mappedOnce = 0;
	wdata->hasIMStatusWindow = 0;
	wdata->isShowing = False;
	wdata->shellResized = False;
	wdata->canvasResized = False;
	wdata->menuBarReset = False;
	wdata->initialReshape = True;
	wdata->fixTargetSize = True;	/* only relevant if fixInsets is
					 * nonzero */

	/* create a top-level shell */
	argc = 0;


#if 0
	if ((*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Frame_resizableFID)==JNI_TRUE) {
		XtSetArg(args[argc], XmNallowShellResize, True);
		argc++;

		XtSetArg(args[argc], XmNmwmDecorations, AWT_MWM_RESIZABLE_DECOR);
		argc++;
		XtSetArg(args[argc], XmNmwmFunctions, AWT_MWM_RESIZABLE_FUNC);
		argc++;
	} else {
		XtSetArg(args[argc], XmNallowShellResize, False);
		argc++;
		XtSetArg(args[argc], XmNmwmDecorations, AWT_MWM_NONRESIZABLE_DECOR);
		argc++;
		XtSetArg(args[argc], XmNmwmFunctions, AWT_MWM_NONRESIZABLE_FUNC);
		argc++;
		/* cannot set size hints until reshape */
	}
#endif

	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;
	XtSetArg(args[argc], XmNcolormap, awt_cmap);
	argc++;
	XtSetArg(args[argc], XmNdepth, awt_depth);
	argc++;
	XtSetArg(args[argc], XmNdefaultFontList, fontList);
	argc++;

	wdata->winData.shell = XtAppCreateShell("J2ME-PP App", "XApplication",
			   vendorShellWidgetClass, awt_display, args, argc);

	thisGlobalRef = wdata->winData.comp.peerGlobalRef = (*env)->NewGlobalRef(env, this);

	if (thisGlobalRef == NULL) {
		XtFree((char *)wdata);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	setDeleteCallback(thisGlobalRef, wdata);

	/*
	 * Establish resizability.  For the case of not resizable, do not yet
	 * set a fixed size here; we must wait until in the routine
	 * sun_awt_motif_MFramePeer_pReshape() after insets have been fixed.
	 * This is because correction of the insets may affect shell size.
	 * (See comments in shellEH() concerning correction of the insets.
	 */

	wdata->isResizable =
		(((*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Frame_resizableFID)==JNI_TRUE) ? True : False);
	wdata->isFixedSizeSet = False;
	wdata->isIconic = False;

	XtAddEventHandler(wdata->winData.shell, StructureNotifyMask | FocusChangeMask, False, (XtEventHandler)shellEH, thisGlobalRef);

       	XtAddEventHandler(wdata->winData.shell, (EventMask)0, True, (XtEventHandler) _XEditResCheckMessages, NULL);
	

	argc = 0;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNspacing, 0);
	argc++;
	XtSetArg(args[argc], XmNresizePolicy, XmRESIZE_NONE);
	argc++;

	wdata->mainWindow = XmCreateForm(wdata->winData.shell, "frame:", args, argc);

	wdata->winData.comp.widget = awt_canvas_create(env, (XtPointer) thisGlobalRef,
	   wdata->mainWindow, XtName(wdata->mainWindow), 0, 0, True, wdata);

	/*	XtAddCallback(wdata->winData.comp.widget, XmNresizeCallback, Frame_resize, thisGlobalRef); */

	XtVaSetValues(wdata->winData.comp.widget, XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, NULL);

	/* No menu bar initially */
	wdata->menuBar = NULL;
	wdata->mbHeight = 0;

	/* If a warning window (string) is needed, establish it now. */

	warnstr = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_Window_warningStringFID);

	if ((warnstr != NULL) && !isEmbedded) {
		char           *tmpstr;

		/*
		 * Insert a warning window. It's height can't be set yet; it
		 * will later be set in setMbAndWwHeightAndOffsets().
		 */

		tmpstr = (char *) (*env)->GetStringUTFChars(env, warnstr, NULL);

		wdata->warningWindow = awt_util_createWarningWindow(wdata->mainWindow, tmpstr);
		wdata->wwHeight = 0;

		XtVaSetValues(wdata->warningWindow,
			      XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, NULL);



#ifdef NETSCAPE			/* For NETSCAPE, warning window is at bottom
				 * of the form */

		XtVaSetValues(wdata->winData.comp.widget, XmNtopAttachment, XmATTACH_FORM, NULL);
		XtVaSetValues(wdata->warningWindow,
			      XmNtopAttachment, XmATTACH_WIDGET,
			      XmNtopWidget, wdata->winData.comp.widget, XmNbottomAttachment, XmATTACH_FORM, NULL);

#else				/* NETSCAPE - Otherwise (not NETSCAPE),
				 * warning is at top of form    */


		XtVaSetValues(wdata->warningWindow, XmNtopAttachment, XmATTACH_FORM, NULL);
		XtVaSetValues(wdata->winData.comp.widget,
			      XmNtopAttachment, XmATTACH_WIDGET,
			      XmNtopWidget, wdata->warningWindow, XmNbottomAttachment, XmATTACH_FORM, NULL);

#endif				/* NETSCAPE */

		(*env)->ReleaseStringUTFChars(env, warnstr, tmpstr);

	} else {
		/* No warning window present */

		XtVaSetValues(wdata->winData.comp.widget,
			      XmNtopAttachment, XmATTACH_FORM, XmNbottomAttachment, XmATTACH_FORM, NULL);


		wdata->warningWindow = NULL;
		wdata->wwHeight = 0;
	}


	AWT_FLUSH_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pDispose(JNIEnv * env, jobject this)
{
	struct FrameData *frameData;

	AWT_LOCK();

	frameData = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (frameData == NULL || frameData->mainWindow == NULL || frameData->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(frameData->mainWindow);
	awt_util_consumeAllXEvents(frameData->mainWindow);
	awt_util_consumeAllXEvents(frameData->winData.shell);
	XtDestroyWidget(frameData->mainWindow);
	XtDestroyWidget(frameData->winData.shell);

	AWT_UNLOCK();
}

/*
 * Gets the nearest Motif icon size to the given Java icon size
 */
JNIEXPORT jobject JNICALL
Java_sun_awt_motif_MFramePeer_pGetIconImageSize(JNIEnv * env, jobject this, jint widthHint, jint heightHint)
{
	struct FrameData *wdata;
	unsigned int    border_width, depth;
	Window          win;
	int             x, y;
	unsigned int    dist = 0xffffffff;
	int             diff = 0;
	int             closestWidth;
	int             closestHeight;
	int             newDist;
	int             found = 0;
	unsigned int    saveWidth = 0xffffffff;
	unsigned int    saveHeight = 0xffffffff;
	jobject         hjad;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return NULL;
	}
	XtVaGetValues(wdata->winData.shell, XmNiconWindow, &win, NULL);
	if (!win) {
		int             count;
		int             i;
		XIconSize      *sizeList;

		if (!XGetIconSizes(awt_display, awt_root, &sizeList, &count)) {
			AWT_UNLOCK();
			/*
			 * No icon sizes so can't set it -- Should we throw
			 * an exception?
			 */
			return NULL;
		}
		for (i = 0; i < count; i++) {
			if (widthHint >= sizeList[i].min_width &&
			    widthHint <= sizeList[i].max_width &&
			    heightHint >= sizeList[i].min_height && heightHint <= sizeList[i].max_height) {
				found = 1;
				if ((((widthHint - sizeList[i].min_width)
				      % sizeList[i].width_inc) == 0) &&
				    (((heightHint - sizeList[i].min_height) % sizeList[i].height_inc) == 0)) {
					/* Found an exact match */
					saveWidth = widthHint;
					saveHeight = heightHint;
					dist = 0;
					break;
				}
				diff = widthHint - sizeList[i].min_width;
				if (diff == 0) {
					closestWidth = widthHint;
				} else {
					diff = diff % sizeList[i].width_inc;
					closestWidth = widthHint - diff;
				}
				diff = heightHint - sizeList[i].min_height;
				if (diff == 0) {
					closestHeight = heightHint;
				} else {
					diff = diff % sizeList[i].height_inc;
					closestHeight = heightHint - diff;
				}
				newDist = closestWidth * closestWidth + closestHeight * closestHeight;
				if (dist > newDist) {
					saveWidth = closestWidth;
					saveHeight = closestHeight;
					dist = newDist;
				}
			}
		}

		if (!found) {
			/* NOTE: Aspect ratio */
			if (widthHint >= sizeList[0].max_width && heightHint >= sizeList[0].max_height) {
				saveWidth = sizeList[0].max_width;
				saveHeight = sizeList[0].max_height;
			} else if (widthHint >= sizeList[0].min_width && heightHint >= sizeList[0].min_height) {
				saveWidth = sizeList[0].min_width;
				saveHeight = sizeList[0].min_height;
			} else {
				saveWidth = (sizeList[0].min_width + sizeList[0].max_width) / 2;
				saveHeight = (sizeList[0].min_height + sizeList[0].max_height) / 2;
			}
		}
		XtFree((void *) sizeList);
	} else {
		Window          root;

		if (XGetGeometry(awt_display, win, &root, &x, &y, &saveWidth, &saveHeight, &border_width, &depth)) {
		}
	}

	/* create a Dimension object to return */
	hjad = (*env)->NewObject(env,
				 MCachedIDs.java_awt_DimensionClass,
			       MCachedIDs.java_awt_Dimension_constructorMID,
				 (jint) saveWidth, (jint) saveHeight);

	AWT_UNLOCK();

	return hjad;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pSetIconImage(JNIEnv * env, jobject this, jobject ir)
{
	struct FrameData *wdata;
	XID             pixmap;
	unsigned int    width, height, border_width, depth;
	Window          win;
	Window          root;
	int             x, y;
	unsigned long   mask;
	XSetWindowAttributes attrs;

	if (ir == NULL) {
	  /*		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL); */
		return;
	}
	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	pixmap = image_getIRDrawable(env, ir);
	if (wdata == NULL || pixmap == 0 || wdata->winData.shell == 0) {
	  /*		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL); */
		AWT_UNLOCK();
		return;
	}
	XtVaGetValues(wdata->winData.shell, XmNiconWindow, &win, NULL);
	if (!win) {
		mask = CWBorderPixel | CWColormap;
		attrs.border_pixel = XFOREGROUND;
		attrs.colormap = XCOLORMAP;
		if (!XGetGeometry(awt_display,
				  pixmap,
				  &root,
				  &x,
				  &y,
				  &width,
				  &height,
				  &border_width,
				  &depth) ||
		    !(win = XCreateWindow(awt_display,
					  root,
					  0, 0, width, height,
					  (unsigned) 0, depth, InputOutput, awt_visual, mask, &attrs))) {
			XtVaSetValues(wdata->winData.shell, XmNiconPixmap, pixmap, NULL);
			AWT_UNLOCK();
			return;
		}
	}
	XtVaSetValues(wdata->winData.shell, XmNiconWindow, win, NULL);
	XSetWindowBackgroundPixmap(awt_display, win, pixmap);
	XClearWindow(awt_display, win);
	AWT_UNLOCK();
}


/*
 * sun_awt_motif_MFramePeer_pSetMenuBar() is native (X/Motif) routine which
 * handles insertion or deletion of a menubar from this frame.
 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pSetMenuBar(JNIEnv * env, jobject this, jobject mb)
{
	struct FrameData *wdata;
	struct ComponentData *mdata;
	jobject         target;
	/*	jint            height; */

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);
	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (target == NULL || wdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (mb == NULL) {
		if (wdata->menuBar != NULL) {
			/*
			 * Redo attachments of other form widgets
			 * appropriately now
			 */

			if (wdata->warningWindow == NULL) {
				/*
				 * no warning window: canvas is now attached
				 * to form
				 */
				XtVaSetValues(wdata->winData.comp.widget, XmNtopAttachment, XmATTACH_FORM, NULL);
			} else {
				/*
				 * warning window present - conditional on
				 * #define NETSCAPE: if NETSCAPE, warning
				 * window is at bottom, so canvas is attached
				 * to the form (as above); otherwise (not
				 * NETSCAPE), warning window itself is
				 * instead attached to form.
				 */

#ifdef NETSCAPE
				XtVaSetValues(wdata->winData.comp.widget, XmNtopAttachment, XmATTACH_FORM, NULL);
#else				/* NETSCAPE */
				XtVaSetValues(wdata->warningWindow, XmNtopAttachment, XmATTACH_FORM, NULL);
#endif				/* NETSCAPE */

			}

			wdata->menuBarReset = True;
		}
		wdata->menuBar = NULL;
		/*		setMbAndWwHeightAndOffsets(env, this, wdata); */

		(*env)->SetBooleanField(env, target, MCachedIDs.java_awt_Frame_mbManagementFID, (jboolean) False);
		AWT_UNLOCK();
		return;
	}
	mdata = (struct ComponentData *) (*env)->GetIntField(env, mb, MCachedIDs.MMenuBarPeer_pDataFID);

	if (mdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);

		(*env)->SetBooleanField(env, target, MCachedIDs.java_awt_Frame_mbManagementFID, (jboolean) False);

		AWT_UNLOCK();
		return;
	}
	/*
	 * If existing menu bar, unmap, unmanage it before placing new menu.
	 * This code will probably never be executed since in Frame.java's
	 * setMenuBar(), any existing menu bar is remove'd before the peer's
	 * setMenuBar(), and hence this native ...pSetMenuBar(), is called.
	 * Nevertheless, it won't hurt to leave this code here for safety.
	 */

	if (wdata->menuBar != NULL) {
		if (wdata->menuBar == mdata->widget) {
			/* new menu bar is the old (existing) menu bar. */
			(*env)->SetBooleanField(env, target, MCachedIDs.java_awt_Frame_mbManagementFID, (jboolean) False);
			AWT_UNLOCK();
			return;
		}
		XtUnmapWidget(wdata->menuBar);
		XtUnmanageChild(wdata->menuBar);
	}
	/*
	 * OK - insert the new menu bar into the form (at the top). Redo the
	 * attachments of other form widgets appropriately.
	 */

	if (wdata->menuBar == NULL)
		wdata->menuBarReset = True;

	wdata->menuBar = mdata->widget;

	XtVaSetValues(mdata->widget,
		      XmNtopAttachment, XmATTACH_FORM,
		      XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, NULL);

	if (wdata->warningWindow == NULL) {
		/* no warning window: menu bar at top, canvas attached to it    */
		XtVaSetValues(wdata->winData.comp.widget, XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, mdata->widget, NULL);
	} else {
		/*
		 * warning window present - conditional on #define NETSCAPE:
		 * if NETSCAPE, warning window is at bottom, so canvas is
		 * attached to menu bar (as above); otherwise (not NETSCAPE),
		 * the warning window is attached just below the menu bar.
		 */
#ifdef NETSCAPE
		XtVaSetValues(wdata->winData.comp.widget, XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, mdata->widget, NULL);
#else				/* NETSCAPE */
		XtVaSetValues(wdata->warningWindow,
			      XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, mdata->widget, NULL);
#endif				/* NETSCAPE */

	}


	XtManageChild(mdata->widget);


	/*	setMbAndWwHeightAndOffsets(env, this, wdata); */

	(*env)->SetBooleanField(env, target, MCachedIDs.java_awt_Frame_mbManagementFID, (jboolean) False);

	AWT_FLUSH_UNLOCK();
}


/* sun_awt_motif_MFramePeer_pShow() is native (X/Motif) mapping routine	 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pShow(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL ||
	    wdata->mainWindow == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	XtManageChild(wdata->mainWindow);
	XtRealizeWidget(wdata->winData.shell);

	XtPopup(wdata->winData.shell, XtGrabNone);

	wdata->isShowing = True;


	  if(wdata->menuBar)
	    {
	  Dimension menuBarHeight;
	  Dimension h;

        XtVaGetValues(wdata->menuBar, XmNheight, &menuBarHeight, NULL);
        XtVaGetValues(wdata->mainWindow, XmNheight, &h, NULL);


	h+=menuBarHeight;

        XtVaSetValues(wdata->winData.shell, XmNheight, h, NULL);

        XtVaGetValues(wdata->winData.shell, XmNheight, &h, NULL);

	    }




	XFlush(awt_display);

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pHide(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (XtWindow(wdata->winData.shell) != 0) {
		XtUnmanageChild(wdata->winData.comp.widget);
		XtPopdown(wdata->winData.shell);
	}
	wdata->isShowing = False;

	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_toBack(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL || wdata->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (XtWindow(wdata->winData.shell) != 0) {
		XLowerWindow(awt_display, XtWindow(wdata->winData.shell));
	}
	AWT_UNLOCK();
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pSetTitle(JNIEnv * env, jobject this, jbyteArray title)
{
	char           *ctitle;
	int            clen;
	struct FrameData *wdata;
	XTextProperty   text_prop;
	char           *c[1];

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL || wdata->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	clen = (*env)->GetArrayLength(env, title);
	if(clen>0) {

	  ctitle = (char *)XtCalloc(clen+1, sizeof(char));

	(*env)->GetByteArrayRegion(env, title, 0, clen, (jbyte *)ctitle);

	c[0] = ctitle;

	/* need to convert ctitle to CompoundText */
	XmbTextListToTextProperty(awt_display, c, 1, XCompoundTextStyle, &text_prop);
	XtVaSetValues(wdata->winData.shell,
		      XmNtitle, text_prop.value,
		      XmNtitleEncoding, text_prop.encoding,
		      XmNiconName, text_prop.value,
	    XmNiconNameEncoding, text_prop.encoding, XmNname, ctitle, NULL);
	XtFree((char *)(text_prop.value));
	XtFree(ctitle);

	}
	AWT_UNLOCK();
}


/**
 * This method is called by an MTextAreaPeer or MTextFieldPeer to indicate
 * that a Motif Text or TextField widget has been added to this frame's peer.
 * This allows us to adjust this peer's geometry to accomodate an
 * Input Method Status area.
 *
 * In locales that use input methods, Motif automatically adds an IM Status
 * area to the bottom of any VendorShell that contains a Text or TextField
 * widget. We compensate by increasing our bottom inset by the height of
 * the IM Status area. We also reshape the FramePeer so that the exterior
 * dimensions of the peer widgets stay in sync with what the Java Frame
 * object expects.
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_addTextComponent(JNIEnv * env, jobject this)
{
#if 0

	struct FrameData *wdata;
	jobject         target;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	if (!wdata->hasIMStatusWindow) {
		wdata->hasIMStatusWindow = 1;
		wdata->bottom += awt_util_getIMStatusHeight(wdata->winData.shell);
		changeInsets(env, this, wdata);

		reshape(env, this, wdata,
			(*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID),
			(*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID),
			(*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID),
			(*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID));
	}
	AWT_UNLOCK();

#endif
}


/*
 * sun_awt_motif_MFramePeer_pReshape() is native (X/Motif) routine that is
 * called to effect a reposition and / or resize of the target frame. The
 * parameters x,y,w,h specify target's x, y position, width, height.
 */

/*
 * this functionality  is invoked from both java and native code, and we only
 * want to lock when invoking it from java, so wrap the native method version
 * with the locking
 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_pReshape(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct FrameData *wdata;
	jobject         target;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}


	  /*
	{

	  if(wdata->menuBar)
	    {
	  Dimension menuBarHeight;

        XtVaGetValues(wdata->menuBar, XmNheight, &menuBarHeight, NULL);

	h+=menuBarHeight*2+2;

	    }

	}
	*/

	reshape(env, this, wdata, x, y, w, h);

	/*	XFlush(awt_display); */

	AWT_UNLOCK();
}


/*
 * sun_awt_motif_MFramePeer_setResizable() is native (X/Motif) routine called
 * to establish the frame's resizability.  Parameter resizable.
 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFramePeer_setResizable(JNIEnv * env, jobject this, jboolean resizable)
{
#if 0

	struct FrameData *wdata;
	jobject         target;
	long            width,	/* fixed width if not resizable */
	                height;	/* fixed height if not resizable */
	long            verticalAdjust;	/* menubar, warning window, etc. */

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	XtVaSetValues(wdata->winData.shell, XmNallowShellResize, (resizable==JNI_TRUE) ? True : False, NULL);

	if ((!wdata->isResizable) && (resizable==JNI_TRUE)) {
		/*
		 * A previously nonresizable frame is now being made
		 * resizable
		 */
		awt_util_setShellResizable(wdata->winData.shell, wdata->isShowing);
		wdata->isFixedSizeSet = False;
	} else if ((wdata->isResizable) && (resizable==JNI_FALSE)) {
		/*
		 * A previously resizable frame is now being made
		 * nonresizable
		 */

		/*
		 * To calculate fixed window width, height, we must subtract
		 * off the window manager borders as stored in the wdata
		 * structure. But note that the wdata top and bottom fields
		 * may include space for warning window, menubar, IM status;
		 * this IS part of shell
		 */
		verticalAdjust = wdata->mbHeight;
		if (wdata->warningWindow != NULL) {
			verticalAdjust += wdata->wwHeight;
		}
		if (wdata->hasIMStatusWindow) {
			verticalAdjust += awt_util_getIMStatusHeight(wdata->winData.shell);
		}
		width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
		height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

		width -= wdata->left + wdata->right;
		height -= wdata->top + wdata->bottom + verticalAdjust;

		if ((width > 0) && (height > 0)) {
			awt_util_setShellNotResizable(wdata->winData.shell, width, height, wdata->isShowing);
			wdata->isFixedSizeSet = True;
		}
	}
	wdata->isResizable = (Boolean)(resizable==JNI_TRUE);

	AWT_UNLOCK();


#endif
}

jobject
CreateEmbeddedFrame(jint handle)
{
	JNIEnv         *env;
	jobject         embeddedFrame;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return NULL;

	embeddedFrame = (*env)->NewObject(env,
					  MCachedIDs.MEmbeddedFrameClass,
			  MCachedIDs.MEmbeddedFrame_constructorMID, handle);


	return embeddedFrame;
}



JNIEXPORT void  JNICALL
Java_sun_awt_motif_MEmbeddedFramePeer_NEFcreate(JNIEnv * env, jobject this, jobject parent,
						jobject arg, jint ladHandle)
{
	Arg             args[40];
	int             argc;
	int             isEmbedded;

	struct FrameData *wdata;

	Widget          innerCanvasW;

	jobject         target, insets, thisgbl;


	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetObjectField(env, this, MCachedIDs.MFramePeer_insetsFID, arg);
	insets = (*env)->GetObjectField(env, this, MCachedIDs.MFramePeer_insetsFID);

	wdata = (struct FrameData *)XtCalloc(1, sizeof(struct FrameData));

	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) wdata);

	if (wdata == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	/*
	 * A variation on Netscape's solution for embedded frames: the client
	 * area of the browser is a Java Frame for parenting purposes, but
	 * really a Motif child window
	 */
	isEmbedded = (*env)->IsInstanceOf(env, target, MCachedIDs.MEmbeddedFrameClass);

	if (isEmbedded) {
		wdata->winData.flags |= W_IS_EMBEDDED;

		(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_topFID, (jint) 0);
		(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_leftFID, (jint) 0);
		(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_bottomFID, (jint) 0);
		(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_rightFID, (jint) 0);

		wdata->top = 0;
		wdata->left = 0;
		wdata->bottom = 0;
		wdata->right = 0;

	} else {

		wdata->top = (*env)->GetIntField(env, insets, MCachedIDs.java_awt_Insets_topFID);
		wdata->left = (*env)->GetIntField(env, insets, MCachedIDs.java_awt_Insets_leftFID);
		wdata->bottom = (*env)->GetIntField(env, insets, MCachedIDs.java_awt_Insets_bottomFID);
		wdata->right = (*env)->GetIntField(env, insets, MCachedIDs.java_awt_Insets_rightFID);
	}

	wdata->isModal = 0;
	wdata->mappedOnce = 0;
	wdata->hasIMStatusWindow = 0;
	wdata->isShowing = False;
	wdata->shellResized = False;
	wdata->canvasResized = False;
	wdata->menuBarReset = False;

	wdata->winData.shell = (Widget) ladHandle;

	setDeleteCallback(this, wdata);
	wdata->isResizable =
		(*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Frame_resizableFID) ? True : False;
	wdata->isFixedSizeSet = False;

	if (wdata->isResizable) {
		awt_util_setShellResizable(wdata->winData.shell, wdata->isShowing);
	}
	thisgbl = (*env)->NewGlobalRef(env, this);

	XtAddEventHandler(wdata->winData.shell, StructureNotifyMask | FocusChangeMask, FALSE, shellEH, thisgbl);

	argc = 0;

	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNhorizontalSpacing, 0);
	argc++;
	XtSetArg(args[argc], XmNverticalSpacing, 0);
	argc++;

#ifdef NETSCAPE
	XtSetArg(args[argc], XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE);
	argc++;
#endif				/* NETSCAPE */

	XtSetArg(args[argc], XmNresizePolicy, XmRESIZE_NONE);
	argc++;
	wdata->mainWindow = XmCreateForm(wdata->winData.shell, "main", args, argc);

	wdata->winData.comp.widget = awt_canvas_create(env, (XtPointer) thisgbl,
			 wdata->mainWindow, "Eframe_", -1, -1, True, wdata);

	/*
	 * XtAddCallback(wdata->winData.comp.widget, XmNresizeCallback,
	 * outerCanvasResizeCB, (XtPointer) thisgbl);
	 */

	innerCanvasW = XtParent(wdata->winData.comp.widget);
	XtVaSetValues(innerCanvasW, XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, NULL);

	/*
	 * XtAddEventHandler(innerCanvasW, StructureNotifyMask, FALSE,
	 * innerCanvasEH, (XtPointer) thisgbl);
	 */

	wdata->menuBar = NULL;
	wdata->mbHeight = 0;

	XtVaSetValues(innerCanvasW, XmNtopAttachment, XmATTACH_FORM, XmNbottomAttachment, XmATTACH_FORM, NULL);
	wdata->warningWindow = NULL;
	wdata->wwHeight = 0;

	awt_util_show(env, wdata->winData.comp.widget);

	AWT_FLUSH_UNLOCK();


}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MEmbeddedFramePeer_create(JNIEnv * env, jobject this, jobject parent, jobject arg)
{


}
