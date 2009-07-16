/*
 * @(#)awt_util.c	1.77 06/10/10
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
#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>

/* TODO : check the need of this variable
   static int      winmgr_running = 0;
*/

Widget
awt_util_createWarningWindow(Widget parent, char *warning)
{
	Widget          warningWindow;

#ifdef NETSCAPE
	extern Widget   FE_MakeAppletSecurityChrome(Widget parent, char *message);

	warningWindow = FE_MakeAppletSecurityChrome(parent, warning);
#else
	Widget          label;
	int             argc;
	Arg             args[10];
	Pixel           yellow = AwtColorMatch(255, 225, 0);
	Pixel           black = AwtColorMatch(0, 0, 0);

	argc = 0;
	XtSetArg(args[argc], XmNbackground, yellow);
	argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	warningWindow = XmCreateForm(parent, "main", args, argc);

	XtManageChild(warningWindow);
	label = XtVaCreateManagedWidget(warning,
					xmLabelWidgetClass, warningWindow,
					XmNhighlightThickness, 0,
					XmNbackground, yellow,
					XmNforeground, black,
					XmNalignment, XmALIGNMENT_CENTER,
					XmNrecomputeSize, False, NULL);
	XtVaSetValues(label,
		      XmNbottomAttachment, XmATTACH_FORM,
		      XmNtopAttachment, XmATTACH_FORM,
		      XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, NULL);
#endif
	return warningWindow;
}

void
awt_setWidgetGravity(Widget w, int gravity)
{
	XSetWindowAttributes xattr;
	Window          win = XtWindow(w);

	if (win != 0) {
		xattr.bit_gravity = StaticGravity;
		xattr.win_gravity = StaticGravity;
		XChangeWindowAttributes(XtDisplay(w), win, CWBitGravity | CWWinGravity, &xattr);
	}
}


jboolean
awt_util_reshape(JNIEnv * env, Widget w, jint x, jint y, jint wd, jint ht)
{
    /* TODO: check the use of these variables
       Dimension       ww, wh;
       Position        wx, wy;
    */

	if (!XtIsWidget(w)) {
	  /*		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL); */

		return JNI_FALSE;
	}

	  XtVaSetValues(w, XmNx, (Position) x,
		      XmNy, (Position) y,
		      XmNwidth, (Dimension) wd,
		      XmNheight, (Dimension) ht,
		      NULL);


	  XtVaGetValues(w, XmNx, &wx,
		      XmNy, &wy,
		      XmNwidth, &ww,
		      XmNheight, &wh,
		      NULL);

#if 0
	  if((wx!=x)||(wy!=y)||(wd!=ww)||(ht!=wh)) {
	    
	    printf("I (%s) wanted to be: %d,%d %d,%d\n", XtName(w), x, y, wd, ht);
	    printf("Im (%s) actually at: %d,%d %d,%d\n", XtName(w), wx, wy, ww, wh);
	  }
#endif

	return JNI_TRUE;
}

jboolean
awt_util_hide(JNIEnv * env, Widget w)
{
	if (!XtIsWidget(w)) {
	  /*		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL); */
		return JNI_FALSE;
	}

	XtSetMappedWhenManaged(w, False);

	return JNI_TRUE;
}

/*
 * Returns JNI_TRUE on success or JNI_FALSE if, and only if, an exception has
 * been thrown.
 */

jboolean
awt_util_show(JNIEnv * env, Widget w)
{

	/*
	 * extern Boolean  scrollBugWorkAround;
	 */
	if (!XtIsWidget(w)) {
	  /*		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL); */
		return JNI_FALSE;
	}

	XtSetMappedWhenManaged(w, True);

	return JNI_TRUE;

	/*
	 * NOTE: causes problems on 2.5 if (!scrollBugWorkAround) {
	 * awt_setWidgetGravity(w, StaticGravity); }
	 */
}

void
awt_util_enable(Widget w)
{
	XtSetSensitive(w, True);
}

void
awt_util_disable(Widget w)
{
	XtSetSensitive(w, False);
}

void
awt_util_mapChildren(Widget w, void (*func) (Widget, void *), int applyToCurrent, void *data) {

	/* The widget may have been destroyed by another thread. */
	if ((w == 0) || (!XtIsObject(w)) || (w->core.being_destroyed))
		return;

	/* if (applyToCurrent) { */
	(*func) (w, data);
	/* } */

	/*
	 * if (!XtIsComposite(w)) { return; }
	 * 
	 * XtVaGetValues(w, XmNchildren, &wlist, XmNnumChildren, &wlen, NULL);
	 * 
	 * for (i = 0; i < wlen; i++) { if(!wlist[i]->core.being_destroyed)
	 * (*func)(wlist[i], data); }
	 */
}

void
awt_changeAttributes(Display * dpy, Widget w, unsigned long mask, XSetWindowAttributes * xattr)
{
	int             i;
	WidgetList      wlist;
	Cardinal        wlen = 0;

	if (XtWindow(w) && XtIsRealized(w)) {
		XChangeWindowAttributes(dpy, XtWindow(w), mask, xattr);
	} else {
		return;
	}
	XtVaGetValues(w, XmNchildren, &wlist, XmNnumChildren, &wlen, NULL);
	for (i = 0; i < wlen; i++) {
		if (XtWindow(wlist[i]) && XtIsRealized(wlist[i])) {
			XChangeWindowAttributes(dpy, XtWindow(wlist[i]), mask, xattr);
		}
	}
}

int
awt_util_setCursor(Widget w, Cursor c)
{

	if (XtIsRealized(w)) {

		unsigned long   valuemask = 0;
		XSetWindowAttributes attributes;

		valuemask = CWCursor;
		attributes.cursor = c;
		XChangeWindowAttributes(awt_display, XtWindow(w), valuemask, &attributes);
		return 1;
	} else
		return 0;
}

/*
 * Part fix for bug id 4017222. Return the widget at the given screen coords
 * by searching the widget tree beginning at root. This function will return
 * null if the pointer is not over the root widget or child of the root
 * widget.
 */
Widget
awt_WidgetAtXY(Widget root, Position pointerx, Position pointery)
{
	int             i;
	WidgetList      wl;
	Cardinal        wlen = 0;
	Position        wx, wy;
	Dimension       width, height;
	int             lastx, lasty;

	if (XtIsComposite(root)) {
		XtVaGetValues(root, XmNchildren, &wl, XmNnumChildren, &wlen, NULL);

		for (i = 0; i < wlen; i++) {

			XtVaGetValues(wl[i], XmNx, &wx, XmNy, &wy, XmNwidth, &width, XmNheight, &height, NULL);

			XtTranslateCoords(root, 0, 0, &wx, &wy);
			lastx = wx + width;
			lasty = wy + height;

			if (pointerx >= wx && pointerx <= lastx && pointery >= wy && pointery <= lasty)
				return wl[i];
		}
	}
	return NULL;
}


/*
 * awt_util_getIMStatusHeight is a cut and paste of the ImGetGeo() function
 * found in CDE Motif's Xm/XmIm.c.  It returns the height of the Input Method
 * Status region attached to the given VendorShell.  This is needed in order
 * to calculate geometry for Frames and Dialogs that contain TextField or
 * TextArea widgets.
 * 
 * Copying this function out of the Motif source is a not good. 
 * Unfortunately, Motif tries to hide the existance of the IM Status region
 * from us so it does not provide any public way to get this info. Clearly a
 * better long term solution needs to be found.
 */

#include <Xm/VendorSEP.h>
#include <Xm/VendorSP.h>

typedef struct _XmICStruct {
	struct _XmICStruct *next;
	Widget          icw;
	Window          focus_window;
	XtArgVal        foreground;
	XtArgVal        background;
	XtArgVal        background_pixmap;
	XtArgVal        font_list;
	XtArgVal        line_space;
	int             status_width;
	int             status_height;
	int             preedit_width;
	int             preedit_height;
	Boolean         has_focus;
	Boolean         need_reset;
}               XmICStruct;

typedef struct {
	Widget          im_widget;
	XIMStyle        input_style;
	XIC             xic;
	int             status_width;
	int             status_height;
	int             preedit_width;
	int             preedit_height;
	XmICStruct     *iclist;
	XmICStruct     *current;
}               XmImInfo;

#define NO_ARG_VAL -1
#define SEPARATOR_HEIGHT 2

int
awt_util_getIMStatusHeight(Widget vw)
{
	return 0;

}


/*
 * Sets or unsets minimum and maximum size properties of the specified shell
 * widget, according to the value of `set' parameter (0 means unset)
 */

void
awt_util_setMinMaxSizeProps(Widget shellW, Boolean set)
{

#if 0
	Display        *disp = XtDisplay(shellW);
	Window          win = XtWindow(shellW);
	XSizeHints     *hints = XAllocSizeHints();
	long            k = 0;

	XGetWMNormalHints(disp, win, hints, &k);
	if (set)
		hints->flags |= (PMinSize | PMaxSize);

	else
		hints->flags &= ~(PMinSize | PMaxSize);
	XSetWMNormalHints(disp, win, hints);
	XFree(hints);

#endif
}


/*
 * For a specified shell widget, make it resizable by setting its minimum and
 * maximum width and height to the screen size range. Also, set its window
 * manager decorations and menu accordingly.
 */

void
awt_util_setShellResizable(Widget shellW, Boolean isMapped)
{
#if 0
	unsigned        wm;

	/*
	 * Iff shell is mapped, we must unmap and later remap in order to
	 * effect the changes we make in the window manager decorations.
	 * N.B.This unmapping / remapping of the shell exposes a bug in
	 * X/Motif or the Motif Window Manager.  When you attempt to map a
	 * widget which is positioned (partially) offscreen, the window is
	 * relocated to be entirely on screen. Good idea.  But if both the x
	 * and the y coordinates are less than the origin (0,0), the first
	 * (re)map will move the window to the origin, and any subsequent
	 * (re)map will relocate the window at some other point on the
	 * screen. I have written a short Motif test program to discover this
	 * bug.  Nevertheless, it's not possible to effect the changes in the
	 * wm decorations other than an unmap and remap.  Also, this should
	 * occur infrequently and it does not cause any real problem.  So for
	 * now we'll let it be.
	 */

	if (isMapped)
		XtUnmapWidget(shellW);

	/* Make it resizable.  Also, set the resources for MWM decorations. */
	/*
	 * Fix for 4106068: shouldn't set maximum/minimum size properties for
	 * a resizable frame
	 */

	/*
	 * screenWidth =
	 * XWidthOfScreen(XDefaultScreenOfDisplay(awt_display));
	 */

	/*
	 * screenHeight =
	 * XHeightOfScreen(XDefaultScreenOfDisplay(awt_display));
	 */

	/* subtract decorations when calculating max window size */
	XtVaSetValues(shellW,

	/* XmNminWidth, 0, */

	/* XmNmaxWidth, screenWidth, */

	/* XmNminHeight, 0, */

	/* XmNmaxHeight, screenHeight, */
		      XmNmwmDecorations, AWT_MWM_RESIZABLE_DECOR, XmNmwmFunctions, AWT_MWM_RESIZABLE_FUNC, NULL);

	/* On Motif, use decor to make frame non-resizable       */
	wm = awt_util_runningWindowManager();
	if ((wm != MOTIF_WM) && (wm != CDE_WM)) {
		awt_util_setMinMaxSizeProps(shellW, 0);
	}
	/*
	 * To set OLWM decorations, we must use the appropriate properties.
	 * N.B. It is unfortunate that the OLWM does not allow us to adjust
	 * these properties dynamically; the following can not be used here.
	 * OLDecorAddAtom = XInternAtom( XtDisplay(shellW), "_OL_DECOR_ADD",
	 * False); OLDecorResizeAtom = XInternAtom( XtDisplay(shellW),
	 * "_OL_DECOR_RESIZE", False); XChangeProperty( XtDisplay(shellW),
	 * XtWindow(shellW), OLDecorAddAtom, XA_ATOM, 32, PropModeReplace,
	 * (unsigned char *)&OLDecorResizeAtom, 1 );
	 */
	/* How would we set the appropriate decorations for other WM's ?!   */

	if (isMapped)
		XtMapWidget(shellW);

#endif
}


/*
 * For a specified shell widget, make it nonresizable by fixing its minimum
 * and maximum widths and heights to the values specified. Also adjust the
 * window manager decorations and menu accordingly.
 */

void
awt_util_setShellNotResizable(Widget shellW, long width, long height, Boolean isMapped)
{
#if 0

	unsigned        wm;

	/*
	 * Iff shell is mapped, we must unmap and later remap in order to
	 * effect the changes we make in the window manager decorations. N.B.
	 * See the corresponding comment above in setShellResizable().
	 */

	if (isMapped)
		XtUnmapWidget(shellW);

	/* Make it not resizable.  Also, set resources for MWM decorations. */
	XtVaSetValues(shellW,
		      XmNmwmDecorations, AWT_MWM_NONRESIZABLE_DECOR,
		      XmNmwmFunctions, AWT_MWM_NONRESIZABLE_FUNC, NULL);

	wm = awt_util_runningWindowManager();
	if ((wm != MOTIF_WM) && (wm != CDE_WM)) {
		XtVaSetValues(shellW,
			      XmNminWidth, (XtArgVal) width,
			      XmNmaxWidth, (XtArgVal) width,
			      XmNminHeight, (XtArgVal) height, XmNmaxHeight, (XtArgVal) height, NULL);
		awt_util_setMinMaxSizeProps(shellW, 1);
	}
	/*
	 * To set OLWM decorations, we must use the appropriate properties.
	 * N.B. It is unfortunate that the OLWM does not allow us to adjust
	 * these properties dynamically; the following can not be used here.
	 * OLDecorDelAtom = XInternAtom( XtDisplay(shellW), "_OL_DECOR_DEL",
	 * False); OLDecorResizeAtom = XInternAtom( XtDisplay(shellW),
	 * "_OL_DECOR_RESIZE", False); XChangeProperty( XtDisplay(shellW),
	 * XtWindow(shellW), OLDecorDelAtom, XA_ATOM, 32, PropModeReplace,
	 * (unsigned char *)&OLDecorResizeAtom, 1 );
	 */

	/* How would we set the appropriate decorations for other WM's ?!   */

	if (isMapped)
		XtMapWidget(shellW);

#endif
}

/* TODO: check the need of this function */
#if 0 
/*
 * awt_winmgrerr: temporary XError handler setup to determine if * any
 * windowow manager is running
 */
static int
awt_winmgrerr(Display * display, XErrorEvent * error)
{
	if (error->request_code == X_ChangeWindowAttributes) {
		winmgr_running = TRUE;
	}
	return 0;
}
#endif 

/* Determine current running Window Manager (Motif, OpenLook, or other) */
unsigned int
awt_util_runningWindowManager()
{
#if 0
	Atom            WMAtom;
	static int      wmgr = WM_YET_TO_BE_DETERMINED;
	unsigned long   nItems;
	unsigned long   bytesAfter;
	Status          status;
	Atom            actualType;
	int             actualFormat;
	unsigned char  *ptr;
	XSetWindowAttributes attrib;

	if (wmgr != WM_YET_TO_BE_DETERMINED) {
		return wmgr;
	}
	/*
	 * Is any window manager is running? * It would appear we must make
	 * this test first as the winmgr * (at least dtwm) when killed still
	 * appears to be running * presumably as its distinguishing atoms are
	 * still lying around
	 */
	/* if any window manager is running? */
	XSetErrorHandler(awt_winmgrerr);

	winmgr_running = 0;
	attrib.event_mask = SubstructureRedirectMask;
	XChangeWindowAttributes(awt_display, DefaultRootWindow(awt_display), CWEventMask, &attrib);

	/* XSync(awt_display, FALSE); */

	(void) XSetErrorHandler(xerror_handler);

	if (!winmgr_running) {
		wmgr = NO_WM;
		attrib.event_mask = 0;
		XChangeWindowAttributes(awt_display, DefaultRootWindow(awt_display), CWEventMask, &attrib);
	} else if ((WMAtom = XInternAtom(awt_display, "_DT_SM_WINDOW_INFO", TRUE)) != 0) {
		wmgr = MOTIF_WM;
	} else if ((WMAtom = XInternAtom(awt_display, "_MOTIF_WM_INFO", TRUE)) != 0) {
		status = XGetWindowProperty(awt_display,
					    DefaultRootWindow(awt_display),
					    WMAtom, 0, 1, False,
					    WMAtom, &actualType, &actualFormat, &nItems, &bytesAfter, &ptr);

		if (status == Success && actualFormat != 0) {
			XFree((char *) ptr);
			wmgr = MOTIF_WM;
		} else if ((WMAtom = XInternAtom(awt_display, "_SUN_WM_PROTOCOLS", TRUE)) != 0) {
			wmgr = OPENLOOK_WM;
		} else {
			wmgr = OTHER_WM;
		}
	} else if ((WMAtom = XInternAtom(awt_display, "_SUN_WM_PROTOCOLS", TRUE)) != 0) {
		wmgr = OPENLOOK_WM;
	} else {
		wmgr = OTHER_WM;
	}
	return (wmgr);

#endif

	return WM_YET_TO_BE_DETERMINED;

}


/* Determine current running Window Manager (Motif, OpenLook, or other)	 */

/*
 * unsigned awt_util_runningWindowManager(Widget w) { Atom *propAtomList; int
 * numProps; char      *atomName; unsigned  index;
 * 
 * propAtomList = XListProperties( XtDisplay(w),
 * RootWindowOfScreen(XtScreen(w)), &numProps );
 * 
 * if  (propAtomList != NULL) { for (index = 0; index < numProps; index++) {
 * atomName = XGetAtomName(XtDisplay(w), propAtomList[index]); if (atomName
 * != NULL) { if  (!strcmp(atomName, "_MOTIF_WM_INFO")) { XFree(atomName);
 * XFree(propAtomList); return(MOTIF_WM); } else if  (!strcmp(atomName,
 * "_SUN_WM_PROTOCOLS")) { XFree(atomName); XFree(propAtomList);
 * return(OPENLOOK_WM); } XFree(atomName); atomName = NULL; } }
 * XFree(propAtomList); } return(OTHER_WM); }
 */

/*
 * This callback proc is installed via setting the XmNinsertPosition resource
 * on a widget. It ensures that components added to a widget are inserted in
 * the correct z-order position to match up with their peer/target ordering
 * in Container.java
 */
Cardinal
awt_util_insertCallback(Widget w)
{
	jobject         peer;
	WidgetList      children;
	int             num_children;
	Widget          parent;
	XtPointer       userdata;
	Cardinal        index = 0;
	jint            pos;
	JNIEnv         *env;

	parent = XtParent(w);
	XtVaGetValues(w, XmNuserData, &userdata, NULL);

	if (userdata != NULL) {

		if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
			return (Cardinal) 0;

		peer = (jobject) userdata;
		AWT_UNLOCK();
		pos = (*env)->CallIntMethod(env, peer, MCachedIDs.MComponentPeer_getZOrderPositionMID);
		AWT_LOCK();

		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
			pos = -1;
		} else {
			XtVaGetValues(parent, XmNnumChildren, &num_children, XmNchildren, &children, NULL);
			index = num_children;	/* default is to add to end */
		}
		index = (Cardinal) (pos != -1 ? pos : num_children);
	}
	return index;
}

void
awt_util_consumeAllXEvents(Widget widget)
{
	/* Remove all queued X Events for the window of the widget. */

#define ALL_EVENTS_MASK 0xFFFF

	XEvent          xev;

	XFlush(awt_display);
	while (XCheckWindowEvent(awt_display, XtWindow(widget), ALL_EVENTS_MASK, &xev));
}

void
awt_util_cleanupBeforeDestroyWidget(Widget widget)
{
	/* Bug 4017222: Drag processing uses global prevWidget. */
	if (widget == prevWidget)
		prevWidget = 0;
}
