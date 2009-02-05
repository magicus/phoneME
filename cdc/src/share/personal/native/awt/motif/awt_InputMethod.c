/*
 * @(#)awt_InputMethod.c	1.39 06/10/10
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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/MwmUtil.h>
#include <Xm/MenuShell.h>

#include "awt.h"
#include "awt_p.h"

#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_X11InputMethod.h"

#define SETARG(name, value)	XtSetArg(args[argc], name, value); argc++

#define MCOMPONENTPEER_CLASS_NAME	"sun/awt/motif/MComponentPeer"

extern XVaNestedList awt_util_getXICStatusAreaList(Widget);
extern Widget   awt_util_getXICStatusAreaWindow(Widget);


#define ROOT_WINDOW_STYLES	(XIMPreeditNothing | XIMStatusNothing)
#define NO_STYLES		(XIMPreeditNone | XIMStatusNone)


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11InputMethod_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.X11InputMethod_dispatchCommittedTextMID =
	(*env)->GetMethodID(env, cls, "dispatchCommittedText", "(Ljava/lang/String;J)V");
}

/*
 * X11InputMethodData keeps per X11InputMethod instance information. A
 * pointer to this data structure is kept in an X11InputMethod object
 * (pData).
 */
typedef struct _X11InputMethodData {
	XIC             current_ic;	/* current X Input Context (always
					 * ic_passive in JDK1.1.x) */
	XIC             ic_active;	/* X Input Context for active clients
					 * (not used in JDK1.1.x) */
	XIC             ic_passive;	/* X Input Context for passive
					 * clients */
	jobject         peer;	/* MComponentPeer of client Window */

	jobject         x11inputmethod;	/* global ref to X11InputMethod
					 * instance associated with the XIC */
}
                X11InputMethodData;

jobject         currentX11InputMethodInstance;	/* reference to the current
						 * X11InputMethod instance */

static XIM      X11im = NULL;

static void
destroyX11InputMethodData(X11InputMethodData * pX11IMData)
{
	/*
	 * Destroy XIC
	 */
	if (pX11IMData->ic_passive != (XIC) 0) {
		if (pX11IMData->current_ic != (XIC) 0)
			XDestroyIC(pX11IMData->ic_passive);
		pX11IMData->ic_passive = (XIC) 0;
		pX11IMData->current_ic = (XIC) 0;
	}
	XtFree((void *) pX11IMData);
}

/*
 * Sets or unsets the focus to the given XIC.
 */
static void
setXICFocus(XIC ic, unsigned short req)
{
	if (ic == NULL) {
		(void) fprintf(stderr, "Couldn't find X Input Context\n");
		return;
	}
	if (req == 1)
		XSetICFocus(ic);
	else
		XUnsetICFocus(ic);
}

/*
 * Sets the focus window to the given XIC.
 */
static void
setXICWindowFocus(XIC ic, Window w)
{
	if (ic == NULL) {
		(void) fprintf(stderr, "Couldn't find X Input Context\n");
		return;
	}
	(void) XSetICValues(ic, XNFocusWindow, w, NULL);
}

/*
 * Invokes XmbLookupString() to get something from the XIM. It invokes
 * X11InputMethod.dispatchCommittedText() if XmbLookupString() returns
 * committed text.  This function is called from handleKeyEvent in canvas.c
 * and it's under the Motif event loop thread context.
 * 
 * Buffer usage: The function uses the local buffer first. If it's too small to
 * get the committed text, it allocates memory for the buffer. Note that
 * XmbLookupString() sometimes produces a non-null-terminated string.
 * 
 * Returns True when there is a keysym value to be handled.
 */
#define LOOKUP_BUF_SIZE 64

Bool
awt_x11inputmethod_lookupString(XEvent * event, KeySym * keysymp)
{
	X11InputMethodData *pX11IMData;
	int             buf_len = LOOKUP_BUF_SIZE;
	char            mbbuf[LOOKUP_BUF_SIZE];
	char           *buf;
	KeySym          keysym = NoSymbol;
	Status          status;
	int             mblen;
	XIC             ic;
	Bool            result = True;

	jstring         javastr;

	static Bool     composing = False;

	JNIEnv         *env;

	(*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL);

	pX11IMData =
		(X11InputMethodData *) (*env)->GetIntField(env, currentX11InputMethodInstance,
					MCachedIDs.MComponentPeer_pDataFID);

	if (pX11IMData == NULL) {
		(void) fprintf(stderr, "Couldn't find X Input method Context\n");
		return result;
	}
	if ((ic = pX11IMData->current_ic) == (XIC) 0)
		return result;

	buf = mbbuf;
	mblen = XmbLookupString(ic, (XKeyPressedEvent *) event, buf, buf_len - 1, &keysym, &status);

	/*
	 * In case of overflow, a buffer is allocated and it retries
	 * XmbLookupString().
	 */
	if (status == XBufferOverflow) {
		buf_len = mblen + 1;
		buf = (char *) XtMalloc(buf_len);
		if (buf == NULL) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
			return result;
		}
		mblen = XmbLookupString(ic, (XKeyPressedEvent *) event, buf, buf_len, &keysym, &status);
	}
	buf[mblen] = 0;

	switch (status) {
	case XLookupBoth:
		if (!composing) {
			if (keysym < 128 || ((keysym & 0xff00) == 0xff00)) {
				*keysymp = keysym;
				result = False;
				break;
			}
		}
		composing = False;
	 /* FALLTHRU */ case XLookupChars:

		javastr = (*env)->NewStringUTF(env, buf);

		if (javastr != NULL)
			(*env)->CallVoidMethod(env,
					       currentX11InputMethodInstance,
			 MCachedIDs.X11InputMethod_dispatchCommittedTextMID,
					       javastr, event->xkey.time);
		break;

	case XLookupKeySym:

		if (keysym == XK_Multi_key)
			composing = True;

		if (!composing) {
			*keysymp = keysym;
			result = False;
		}
		break;

	case XLookupNone:
		break;
	}

	if (buf != mbbuf)
		XtFree((void *) buf);

	return result;
}


/*
 * Creates XIC (for passive clients). All information on this XIC is stored
 * in the X11InputMethodData given by the pX11IMData parameter.
 * 
 * For passive clients: Try to use root-window styles. If failed, fallback to
 * None styles.
 */
static          Bool
createXIC(Widget w, X11InputMethodData * pX11IMData)
{
	XIMStyle        passive_styles = 0, no_styles = 0;
	unsigned short  i;
	XIMStyles      *im_styles;

	if (X11im == NULL) {
		(void) fprintf(stderr, "Couldn't find X Input method\n");
		return False;
	}
	/*
	 * If the parent window has one or more TextComponents, the status
	 * area of Motif will be shared with the created XIC. Otherwise,
	 * root-window style status is used.
	 */

	(void) XGetIMValues(X11im, XNQueryInputStyle, &im_styles, NULL);

	for (i = 0; i < im_styles->count_styles; i++) {
		passive_styles |= im_styles->supported_styles[i] & ROOT_WINDOW_STYLES;
		no_styles |= im_styles->supported_styles[i] & NO_STYLES;
	}

	XtFree((char *)im_styles);

	if (passive_styles != ROOT_WINDOW_STYLES) {
		if (no_styles == NO_STYLES)
			passive_styles = NO_STYLES;
		else
			passive_styles = 0;
	}
	pX11IMData->ic_passive = XCreateIC(X11im,
					   XNClientWindow, XtWindow(w),
	    XNFocusWindow, XtWindow(w), XNInputStyle, passive_styles, NULL);
	if (pX11IMData->ic_passive == (XIC) 0) {
		return False;
	}
	return True;
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_motif_X11InputMethod_openXIM(JNIEnv * env, jobject this)
{
	AWT_LOCK();

	X11im = XOpenIM(awt_display, NULL, NULL, NULL);

	/*
	 * Workaround for Solaris 2.6 bug 4097754. We're affected by this
	 * problem because Motif also calls XOpenIM for us. Re-registering
	 * the error handler that MToolkit has registered already after
	 * calling XOpenIM avoids the problem.
	 */
	XSetErrorHandler(xerror_handler);

	AWT_UNLOCK();
	return (X11im != NULL) ? True : False;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_X11InputMethod_initializeXICNative(JNIEnv * env, jobject this, jobject comp)
{

	/*
	 * This module doesn't create XIC for two reasons.
	 * 
	 * 1. When this module is called from activate method of
	 * InputContext.java - The widget may not have been mapped into a
	 * window. Motif attempts to create window for each widget after top
	 * level widget is realized.
	 * 
	 * 2. X11 InputMethod protocol allows application to create XIC without
	 * clientWindow and focus window  and specify it later. But this
	 * protocol is not implemented well in htt server Solaris 2.5. It
	 * expects the client window at the time of XIC creation.
	 * 
	 */
	struct ComponentData *cdata;
	X11InputMethodData *pX11IMData;

	AWT_LOCK();

	if (comp == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "Component");
		AWT_UNLOCK();
		return (jint) 0;
	}
	pX11IMData = (X11InputMethodData *) XtCalloc(1, sizeof(X11InputMethodData));
	if (pX11IMData == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
		AWT_UNLOCK();
		return (jint) 0;
	}
	cdata = (struct ComponentData *) (*env)->GetIntField(env, comp, MCachedIDs.MComponentPeer_pDataFID);

	if (cdata == NULL) {
		XtFree((void *) pX11IMData);
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "CreateXIC");
		AWT_UNLOCK();
		return (long) 0;
	}
	pX11IMData->peer = comp;
	pX11IMData->x11inputmethod = this;

	/*
	 * Delay XIC creation as widget may not be mapped into a window by
	 * motif.
	 */

	AWT_UNLOCK();
	return (jint) pX11IMData;
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11InputMethod_setXICFocus(JNIEnv * env, jobject this, jobject comp, jboolean req,
					      jint data)
{
	X11InputMethodData *pX11IMData = (X11InputMethodData *) data;
	struct ComponentData *cdata;

	AWT_LOCK();

	if (req) {
		if (comp == NULL) {
			AWT_UNLOCK();
			return;
		}
		cdata = (struct ComponentData *) (*env)->GetIntField(env, comp, MCachedIDs.MComponentPeer_pDataFID);

		if (cdata == NULL) {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, "setXICFocus pData");
			AWT_UNLOCK();
			return;
		}
		/*
		 * In JDK1.1.x, it's always passive.
		 */
		pX11IMData->current_ic = pX11IMData->ic_passive;
		/*
		 * If window id is NULL, window is not created yet. postpone
		 * the window setting until it is created.
		 */
		if (pX11IMData->current_ic == NULL) {
			if (XtWindow(cdata->widget) != (Window) NULL) {
				/*
				 * Widget is mapped into window XIC can be
				 * created
				 */
				if (createXIC(cdata->widget, pX11IMData) == False) {
					/*
					 * Unable to create XIC clean up data
					 * structure
					 */
					destroyX11InputMethodData(pX11IMData);
					pX11IMData = (X11InputMethodData *) NULL;
					AWT_UNLOCK();
					return;
				} else
					pX11IMData->current_ic = pX11IMData->ic_passive;
			} else {
				/* Widget is not yet mapped into window */
				AWT_UNLOCK();
				return;
			}
		}
		/* XIC is created */
		setXICWindowFocus(pX11IMData->current_ic, XtWindow(cdata->widget));
		setXICFocus(pX11IMData->current_ic, (int) req);
		currentX11InputMethodInstance = pX11IMData->x11inputmethod;
	} else {
		currentX11InputMethodInstance = NULL;
		/* if xic is not created skip this step */
		if (pX11IMData->current_ic != NULL)
			setXICFocus(pX11IMData->current_ic, (int) req);
		pX11IMData->current_ic = (XIC) 0;
	}

	/* XSync(awt_display, False); */
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11InputMethod_disposeXIC(JNIEnv * env, jobject this, jint pdata)
{
	X11InputMethodData *pX11IMData = (X11InputMethodData *) pdata;

	AWT_LOCK();
	if (pX11IMData->x11inputmethod == currentX11InputMethodInstance)
		currentX11InputMethodInstance = NULL;
	destroyX11InputMethodData(pX11IMData);
	pX11IMData = (X11InputMethodData *) NULL;
	AWT_UNLOCK();
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11InputMethod_closeXIM(JNIEnv * env, jobject this)
{
	AWT_LOCK();

	if (X11im != NULL) {
		XCloseIM(X11im);
		X11im = NULL;
	}
	AWT_UNLOCK();
}

JNIEXPORT jstring JNICALL
Java_sun_awt_motif_X11InputMethod_resetXIC(JNIEnv * env, jobject this, jint data)
{
	X11InputMethodData *pX11IMData = (X11InputMethodData *) data;
	char           *xText;
	jstring         jText;

	AWT_LOCK();

	xText = XmbResetIC(pX11IMData->ic_passive);
	if (xText != NULL) {
		jText = (*env)->NewStringUTF(env, xText);

		XtFree((void *) xText);
	} else {
		jText = NULL;
	}

	AWT_UNLOCK();
	return jText;
}
