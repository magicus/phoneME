/*
 * @(#)awt_Dialog.c	1.122 06/10/10
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
#include <X11/Xmu/Editres.h>
#include <Xm/VendorS.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>

#ifdef PERSONAL
#include <Xm/MenuShell.h>

#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#endif

#include "java_awt_Dialog.h"
#include "sun_awt_motif_MDialogPeer.h"
#include "sun_awt_motif_MFramePeer.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "java_awt_Image.h"
#include "java_awt_Font.h"
#include "java_awt_Insets.h"
#include "canvas.h"
#include "color.h"
#include "awt_util.h"


static void     reshape();

extern Boolean  awt_isModal();
extern Boolean  awt_isWidgetModal(Widget w);
extern void     firstShellEH(Widget w, XtPointer data, XEvent * event);

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MDialogPeer_insetsFID = (*env)->GetFieldID(env, cls, "insets", "Ljava/awt/Insets;");
	MCachedIDs.MDialogPeer_handleDeactivateMID = (*env)->GetMethodID(env, cls, "handleDeactivate", "()V");
	MCachedIDs.MDialogPeer_handleMovedMID = (*env)->GetMethodID(env, cls, "handleMoved", "(II)V");
	MCachedIDs.MDialogPeer_handleQuitMID = (*env)->GetMethodID(env, cls, "handleQuit", "()V");
	MCachedIDs.MDialogPeer_handleResizeMID = (*env)->GetMethodID(env, cls, "handleResize", "(II)V");
	MCachedIDs.MDialogPeer_handleActivateMID = (*env)->GetMethodID(env, cls, "handleActivate", "()V");

	if (MCachedIDs.java_awt_Window_warningStringFID == NULL) {
		cls = (*env)->FindClass(env, "java/awt/Window");

		if (cls == NULL)
			return;

		MCachedIDs.java_awt_Window_warningStringFID =
			(*env)->GetFieldID(env, cls, "warningString", "Ljava/lang/String;");
	}
	cls = (*env)->FindClass(env, "java/awt/Dialog");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Dialog_modalFID = (*env)->GetFieldID(env, cls, "modal", "Z");
	MCachedIDs.java_awt_Dialog_resizableFID = (*env)->GetFieldID(env, cls, "resizable", "Z");
}

/* TODO: check the need of these functions */
#if 0  
/* this function generates a mouse press event and a release event */
static int
SendButtonClick(Display * display, Window window)
{
	XButtonEvent    ev;
	int             status;

	ev.type = ButtonPress;
	ev.display = display;
	ev.window = window;
	ev.send_event = True;

	ev.root = RootWindow(display, DefaultScreen(display));
	ev.subwindow = (Window) None;
	ev.time = CurrentTime;
	ev.x = 0;
	ev.y = 0;
	ev.x_root = 0;
	ev.y_root = 0;
	ev.same_screen = True;
	ev.button = Button1;
	ev.state = Button1Mask;

	status = XSendEvent(display, window, True, ButtonPressMask, (XEvent *) & ev);

	if (status != 0) {
		ev.type = ButtonRelease;
		ev.time = CurrentTime;
		status = XSendEvent(display, window, False, ButtonReleaseMask, (XEvent *) & ev);
	}
	return status;
}

/* this function test if the menu has the current focus */
static int
FocusIsOnMenu(Display * display)
{
	Window          window;
	Widget          widget;
	int             rtr;

	XGetInputFocus(display, &window, &rtr);
	if (window == None) {
		return 0;
	}
	widget = XtWindowToWidget(display, window);
	if (widget == NULL) {
		return 0;
	}


#ifdef PERSONAL
	if (XtIsSubclass(widget, xmMenuShellWidgetClass)) {
		return 1;
	} else {
		return 0;
	}
#endif

}
#endif 
/* TODO: check the need of this function
static void
Dialog_resize(Widget wd, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env = NULL;

		Dimension       width;
		Dimension       height;
		jobject         target;

		if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
			return;

		target = (*env)->GetObjectField(env, (jobject) client_data, MCachedIDs.MComponentPeer_targetFID);

		XtVaGetValues(wd, XmNwidth, &width, XmNheight, &height, NULL);

		(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_widthFID, (jint) width);
		(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_heightFID, (jint) height);

		(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MDialogPeer_handleResizeMID,
				       (jint) width, (jint) height);
}
*/

static void
Dialog_event_handler(Widget w, XtPointer client_data, XEvent * event, Boolean * cont)
{
	JNIEnv         *env = NULL;
	struct FrameData *wdata;

	int             x, y;
	jobject         this, target;

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

	this = (jobject) client_data;

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	if (wdata == NULL) {
		return;
	}

	switch (event->type) {
	case FocusOut:
printf("Dialog FocusOut\n");

/*
		if (event->xfocus.mode == NotifyNormal) {
			awt_setDeactivatedShell(w);
			*/
			(*env)->CallVoidMethod(env, this, MCachedIDs.MDialogPeer_handleDeactivateMID);
			/*		} */
		break;

	case FocusIn:
printf("Dialog FocusIn\n");

/*		if (event->xfocus.mode == NotifyNormal) {
			awt_setActivatedShell(w);
			*/
			(*env)->CallVoidMethod(env, this, MCachedIDs.MDialogPeer_handleActivateMID);

			/*		} */
		break;

	case MapNotify:

printf("Dialog mapped\n");

		if (wdata->mappedOnce == 0) {
			wdata->mappedOnce = 1;
			(*env)->CallVoidMethod(env, this, MCachedIDs.MPanelPeer_makeCursorsVisibleMID);



		}
		break;

	case ConfigureNotify:
		target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

		/*
		 * We can detect the difference between a move and a resize
		 * by checking the send_event flag on the event; if it's
		 * true, then it's indeed a move, if it's false, then this is
		 * a resize and we do not want to process it as a "move"(for
		 * resizes the x,y values are misleadingly set to 0,0, and so
		 * just checking for an x,y delta won't work).
		 */

		x = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID);
		y = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID);

		if (event->xconfigure.send_event == True &&
		    (x != event->xconfigure.x || y != event->xconfigure.y)) {

			x = event->xconfigure.x - wdata->left;
			y = event->xconfigure.y - wdata->top;

			(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_xFID, (jint) x);
			(*env)->SetIntField(env, target, MCachedIDs.java_awt_Component_yFID, (jint) y);

			(*env)->CallVoidMethod(env, this, MCachedIDs.MDialogPeer_handleMovedMID, (jint) x, (jint) y);
		}
		break;
	default:
		break;
	}
}

/**
  static int WaitForUnmap(void *data)

This function is called in the event loop for modal dialogs.  Its
return value indicates whether the event loop for the modal dialog
should continue or return.  We check to see whether the dialog is
still managed, and answer accordingly.  Unfortunately, there seems to
be a bug in event delivery under X11 such that we occasionally get
calls to this function even long after the dialog has been unmanaged
and destroyed.  This can have disastrous consequences if we just
blindly check the dialog's management status.  Instead, we use a
structure to wrap the widget, which lets us gracefully recover in the
case where this function is called after the widget has been
destroyed.  **/

/* TODO: verify the need of this function */
#if 0 
static int
WaitForUnmap(void *data)
{
	struct ModalDialogHolder *mdh = (struct ModalDialogHolder *) data;

	if (mdh->widget != 0) {
		return XtIsManaged(mdh->widget) == False;
	} else {
		/* Encountered destroyed widget -- exit modalLoop */
		return True;
	}
}


static void
Dialog_quit(Widget w, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env = NULL;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MDialogPeer_handleQuitMID);
}
#endif 

#if 0
static void
Dialog_activate(Widget w, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	if (!awt_isModal() || awt_isWidgetModal(w)) {
		awt_setActivatedShell(w);

		(*env)->CallVoidMethod(env, (jobject) client_data, MCachedIDs.MDialogPeer_handleActivateMID);
	}
}
#endif

/* TODO: verify the need of this function */
#if 0 
static void
setDeleteCallback(jobject this, struct FrameData * wdata)
{
	Atom            xa_WM_DELETE_WINDOW;
	Atom            xa_WM_PROTOCOLS;

	XtVaSetValues(wdata->winData.shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
	xa_WM_DELETE_WINDOW = XmInternAtom(XtDisplay(wdata->winData.shell), "WM_DELETE_WINDOW", False);
	xa_WM_PROTOCOLS = XmInternAtom(XtDisplay(wdata->winData.shell), "WM_PROTOCOLS", False);

	XmAddProtocolCallback(wdata->winData.shell,
	xa_WM_PROTOCOLS, xa_WM_DELETE_WINDOW, Dialog_quit, (XtPointer) this);
}
#endif

static void
changeInsets(JNIEnv * env, jobject this, struct FrameData * wdata)
{
	jobject         insets;

	if (this == NULL) {
		return;
	}
	insets = (*env)->GetObjectField(env, this, MCachedIDs.MDialogPeer_insetsFID);

	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_topFID, (jint) wdata->top);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_leftFID, (jint) wdata->left);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_bottomFID, (jint) wdata->bottom);
	(*env)->SetIntField(env, insets, MCachedIDs.java_awt_Insets_rightFID, (jint) wdata->right);

}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_create(JNIEnv * env, jobject this, jobject parent, jobject arg)
{
	Dimension       w, h;
	Position        x, y;
	Arg             args[20];
	int             argc;
	struct FrameData *frameData;
	struct FrameData *parentFrameData;
	jobject         target;
	jstring         warningString;
	jobject         thisGlobalRef;

	AWT_LOCK();

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (target == NULL || parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	frameData = (struct FrameData *)XtCalloc(1, sizeof(struct FrameData));

	if (frameData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return;
	}
	(*env)->SetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID, (jint) frameData);
	parentFrameData =
		(struct FrameData *) (*env)->GetIntField(env, parent, MCachedIDs.MComponentPeer_pDataFID);

	x = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID);
	y = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID);
	w = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
	h = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);
	if(w < 1) w=1;         if(h < 1) h=1;

	argc = 0;
	XtSetArg(args[argc], XmNtransientFor, parentFrameData->winData.shell);
	argc++;
	XtSetArg(args[argc], XmNsaveUnder, False);
	argc++;
	XtSetArg(args[argc], XmNx, x);
	argc++;
	XtSetArg(args[argc], XmNy, y);
	argc++;
	XtSetArg(args[argc], XmNwidth, w);
	argc++;
	XtSetArg(args[argc], XmNheight, h);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNvisual, awt_visual);
	argc++;
	XtSetArg(args[argc], XmNcolormap, awt_cmap);
	argc++;
	XtSetArg(args[argc], XmNdepth, awt_depth);
	argc++;
	frameData->winData.shell = XtCreatePopupShell("Dialog",
		     xmDialogShellWidgetClass, parentFrameData->winData.shell,
						      args, argc);

	thisGlobalRef = (*env)->NewGlobalRef(env, this);

	XtAddEventHandler(frameData->winData.shell, StructureNotifyMask|FocusChangeMask, FALSE, Dialog_event_handler, thisGlobalRef);

       	XtAddEventHandler(frameData->winData.shell, (EventMask)0, True, (XtEventHandler) _XEditResCheckMessages, NULL);


	argc = 0;
	XtSetArg(args[argc], XmNwidth, w);
	argc++;
	XtSetArg(args[argc], XmNheight, h);
	argc++;
	XtSetArg(args[argc], XmNmainWindowMarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmainWindowMarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNspacing, 0);
	argc++;

	frameData->mainWindow = XmCreateRowColumn(frameData->winData.shell, "dialog:", args, argc);
	frameData->top = frameData->left = frameData->bottom = frameData->right = 0;

	warningString = (*env)->GetObjectField(env, target, MCachedIDs.java_awt_Window_warningStringFID);

	if (warningString != NULL) {
		Dimension       hoffset;
		char           *cwarningString = (char *) (*env)->GetStringUTFChars(env, warningString, NULL);

		if (cwarningString == NULL)
			return;

		frameData->warningWindow = awt_util_createWarningWindow(frameData->mainWindow, cwarningString);
		(*env)->ReleaseStringUTFChars(env, warningString, cwarningString);
		XtVaGetValues(frameData->warningWindow, XmNheight, &hoffset, NULL);
		frameData->top += hoffset;
		changeInsets(env, this, frameData);
	} else
		frameData->warningWindow = NULL;

	frameData->winData.comp.widget = awt_canvas_create(env, (XtPointer) thisGlobalRef,
						      frameData->mainWindow,
							   XtName(frameData->mainWindow),
							   w,
							   h,
		      False, (frameData->warningWindow ? frameData : NULL));


	/*	awt_util_show(env, frameData->winData.comp.widget); */
	XtManageChild(frameData->mainWindow);
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_pDispose(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL || wdata->mainWindow == NULL || wdata->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtUnmanageChild(wdata->winData.shell);
	awt_util_consumeAllXEvents(wdata->mainWindow);
	awt_util_consumeAllXEvents(wdata->winData.shell);
	XtDestroyWidget(wdata->mainWindow);
	XtDestroyWidget(wdata->winData.shell);
	/*
	 * Zero widget field in modal dialog widget holder, so that any
	 * subsequent calls to the modal dialog 'continue' function will not
	 * try to ask the widget whether it is managed.
	 */
	if (wdata->modalDialogHolder != 0) {
		wdata->modalDialogHolder->widget = 0;
	}
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_pShow(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;
	jobject         target;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	/*
	 * tdv: fix for bug 4078176 - Modal dialogs don't act modal if
	 * addNotify() is called before setModal(true).
	 */

	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	wdata->isModal = (*env)->GetBooleanField(env, target, MCachedIDs.java_awt_Dialog_modalFID);

	if (wdata->isModal) {

		XtPopup(wdata->winData.shell, XtGrabNonexclusive);

		wdata->isShowing = TRUE;

	} else {
		XtPopup(wdata->winData.shell, XtGrabNone);
		XRaiseWindow(awt_display, XtWindow(wdata->winData.shell));
		wdata->isShowing = TRUE;
		/*	AWT_FLUSH_UNLOCK(); */
	}

	printf("Dialog Show\n");

	XFlush(awt_display);
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_pHide(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == 0 || wdata->winData.shell == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	XtUnmanageChild(wdata->winData.comp.widget);

}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_pSetTitle(JNIEnv * env, jobject this, jstring title)
{
	char           *ctitle = NULL;
	struct FrameData *wdata;
	XTextProperty   text_prop;
	char           *c[1];

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);

	if (wdata == 0 || wdata->winData.shell == 0) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	if (title == NULL)
		ctitle = "";

	else
		ctitle = (char *) (*env)->GetStringUTFChars(env, title, NULL);

	c[0] = ctitle;

	/* need to convert ctitle to CompoundText */
	XmbTextListToTextProperty(awt_display, c, 1, XCompoundTextStyle, &text_prop);
	XtVaSetValues(wdata->winData.shell,
		      XmNtitle, text_prop.value,
		      XmNtitleEncoding, text_prop.encoding,
		      XmNiconName, text_prop.value,
	    XmNiconNameEncoding, text_prop.encoding, XmNname, ctitle, NULL);

	XtFree((char *)(text_prop.value));

	if (title != NULL)
		(*env)->ReleaseStringUTFChars(env, title, ctitle);

	AWT_UNLOCK();
}

/**
 * This method is called by an MTextAreaPeer or MTextFieldPeer to indicate
 * that a Motif Text or TextField widget has been added to this dialog's peer.
 * This allows us to adjust this peer's geometry to accomodate an
 * Input Method Status area.
 *
 * In locales that use input methods, Motif automatically adds an IM Status
 * area to the bottom of any VendorShell that contains a Text or TextField
 * widget.  We compensate by increasing our bottom inset by the height of
 * the IM Status area. We also reshape the DialogPeer so that the exterior
 * dimensions of the peer widgets stay in sync with what the Java Dialog
 * object expects.
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_addTextComponent(JNIEnv * env, jobject this)
{
	struct FrameData *wdata;
	jobject         target;
	jint            x, y, width, height;

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

		x = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_xFID);
		y = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_yFID);
		width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
		height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);

		reshape(wdata, x, y, width, height);
	}
	AWT_UNLOCK();
}

/*
 * this functionality  is invoked from both java and native code, and we only
 * want to lock when invoking it from java, so wrap the native method version
 * with the locking
 */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_pReshape(JNIEnv * env, jobject this, jint x, jint y, jint w, jint h)
{
	struct FrameData *wdata;
	jobject         target;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (wdata == NULL || wdata->winData.comp.widget == 0 || wdata->winData.shell == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}

	reshape(wdata, x, y, w, h);

	AWT_UNLOCK();
}

static void
reshape(struct FrameData * wdata, jint x, jint y, jint w, jint h)
{
    /*TODO: check the need of these variables
	Dimension       hoffset;
	unsigned        wm;
    */

	/*
	 * The warning window is interior to the main window widget. Since
	 * its height is included in the top and bottom insets, we need to
	 * subtract them out again before sizing the main window widget. The
	 * IM Status Window is exterior to the main window so its height does
	 * not need to be removed from the insets.
	 */
#if 0

	hoffset = 0;

	if (wdata->warningWindow != 0) {
		XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
	}
	if (wdata->hasIMStatusWindow) {
		hoffset += awt_util_getIMStatusHeight(wdata->winData.shell);
	}

	w -= (jint) (wdata->left + wdata->right);
	h += (jint) (hoffset - (wdata->top + wdata->bottom));

	if (w < 1) w = 1;
	if (h < 1) h = 1;

	/*
	 * In order to get the Shell's location right, we have to manage the
	 * mainWindow - because DialogShell maps the XmNx and XmNy resources
	 * of its child to the shells origin.  See XmDialogShell(3X).
	 */

	XtManageChild(wdata->mainWindow);

	/*
	 * Motif ignores attempts to move a
	 * toplevel window to 0,0. Instead we set the position to 1,1. The
	 * expected value is returned by Frame.getBounds() since it uses the
	 * internally held rectangle rather than querying the peer.
	 */

	XMoveResizeWindow(XtDisplay(wdata->winData.shell),
			  XtWindow(wdata->winData.shell), x, y, w, h);

#endif


	XtVaSetValues(wdata->winData.shell,
		      XtNx, (XtArgVal) x,
		      XtNy, (XtArgVal) y, 
		      XtNwidth, (XtArgVal) w, 
		      XtNheight, (XtArgVal) h, NULL);

	XtVaSetValues(wdata->winData.comp.widget,
		      XtNx, (XtArgVal) x,
		      XtNy, (XtArgVal) y,
		      XtNwidth, (XtArgVal) w,
		      XtNheight, (XtArgVal) h, NULL);


}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_setResizable(JNIEnv * env, jobject this, jboolean resizable)
{

#if 0
	struct FrameData *wdata;
	jint            width,	/* fixed width if not resizable */
	                height;	/* fixed height if not resizable */
	Dimension       hoffset;/* warning window height offset */
	jobject         target;

	AWT_LOCK();

	wdata = (struct FrameData *) (*env)->GetIntField(env, this, MCachedIDs.MComponentPeer_pDataFID);
	target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

	if (wdata == NULL || wdata->winData.comp.widget == NULL || wdata->winData.shell == NULL || target == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		AWT_UNLOCK();
		return;
	}
	XtVaSetValues(wdata->winData.shell, XmNallowShellResize, resizable ? True : False, NULL);

	if ((!wdata->isResizable) && (resizable)) {
		/*
		 * A previously nonresizable dialog is now being made
		 * resizable
		 */
		awt_util_setShellResizable(wdata->winData.shell, wdata->isShowing);
		wdata->isFixedSizeSet = False;
	} else if ((wdata->isResizable) && (!resizable)) {
		/*
		 * A previously resizable dialog is now being made
		 * nonresizable
		 */

		/*
		 * To calculate fixed window width, height, we must subtract
		 * off the window manager borders as stored in the wdata
		 * structure. But note that the wdata top field can also
		 * include space for warning window; this must remain part of
		 * this dialog window.
		 */

		hoffset = 0;
		if (wdata->warningWindow != 0) {
			XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
		}
		if (wdata->hasIMStatusWindow) {
			hoffset += awt_util_getIMStatusHeight(wdata->winData.shell);
		}
		target = (*env)->GetObjectField(env, this, MCachedIDs.MComponentPeer_targetFID);

		width = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_widthFID);
		width -= (jint) (wdata->left + wdata->right);

		height = (*env)->GetIntField(env, target, MCachedIDs.java_awt_Component_heightFID);
		height -= (jint) ((wdata->top + wdata->bottom) + hoffset);

		if ((width > 0) && (height > 0)) {
			awt_util_setShellNotResizable(wdata->winData.shell, width, height, wdata->isShowing);
			wdata->isFixedSizeSet = True;
		}
	}
	wdata->isResizable = resizable;

	AWT_UNLOCK();

#endif
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MDialogPeer_toBack(JNIEnv * env, jobject this)
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
