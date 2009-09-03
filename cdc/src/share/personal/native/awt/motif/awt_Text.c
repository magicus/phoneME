/*
 * @(#)awt_Text.c	1.11 06/10/10
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
#include "sun_awt_motif_MTextPeer.h"
#include "awt_Text.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MTextPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MTextPeer_pasteFromClipboardMID = (*env)->GetMethodID(env, cls, "pasteFromClipboard", "()V");
	MCachedIDs.MTextPeer_valueChangedMID = (*env)->GetMethodID(env, cls, "valueChanged", "()V");
	MCachedIDs.MTextPeer_selectMapNotifyMID = (*env)->GetMethodID(env, cls, "selectMapNotify", "()V");
}

/*
 * Event handler used by both TextField/TextArea to correctly process
 * cut/copy/paste keys such that interaction with our own clipboard mechanism
 * will work properly.
 */
void
Text_handlePaste(Widget w, XtPointer client_data, XEvent * event, Boolean * cont)
{
	KeySym          keysym;
	long            mods;
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	/*
	 * Any event handlers which take peer instance pointers as
	 * client_data should check to ensure the widget has not been marked
	 * as destroyed as a result of a dispose() call on the peer (which
	 * can result in the peer instance pointer already haven been gc'd by
	 * the time this event is processed)
	 */

	if (w->core.being_destroyed)
		return;


	if (event->type == KeyPress) {

		XtTranslateKeycode(event->xkey.display, (KeyCode) event->xkey.keycode,
			  event->xkey.state, (Modifiers *) & mods, &keysym);

		/*
		 * allow Ctrl-V and Shift-Insert to function as Paste when VM
		 * has clipboard, otherwise the applicatin hangs
		 *//* Bug#unknown */
		if ((event->xkey.state & ControlMask) && (keysym == 'v' || keysym == 'V')) {	/* Bug#unknown */
			keysym = osfXK_Paste;	/* Bug#unknown */
		}		/* Bug#unknown */
		if ((event->xkey.state & ShiftMask) && (keysym == (osfXK_Insert & 0xffff))) {	/* Bug#unknown */
			keysym = osfXK_Paste;	/* Bug#unknown */
		}		/* Bug#unknown */
		switch (keysym) {
		case osfXK_Paste:
			/*
			 * If we own the selection, then paste the data
			 * directly
			 */
			if (awt_isSelectionOwner(env, "CLIPBOARD")) {
				(*env)->CallVoidMethod(env, this, MCachedIDs.MTextPeer_pasteFromClipboardMID);
				*cont = FALSE;
			}
			break;
		case osfXK_Cut:
		case osfXK_Copy:
			/*
			 * For some reason if we own the selection, our
			 * loseSelection callback is not automatically called
			 * on cut/paste from text widgets.
			 */
			if (awt_isSelectionOwner(env, "CLIPBOARD")) {
				awt_notifySelectionLost(env, "CLIPBOARD");
			}
			break;
		}
	}
}

void
Text_valueChanged(Widget w, XtPointer client_data, XtPointer call_data)
{
	JNIEnv         *env;
	jobject         this = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod(env, this, MCachedIDs.MTextPeer_valueChangedMID);
}

void
Text_mapNotify(Widget w, XtPointer client_data, XEvent * event, Boolean * cont)
{
	if (event->type == MapNotify) {
		JNIEnv         *env;
		jobject         this = (jobject) client_data;

		if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
			return;

		(*env)->CallVoidMethod(env, this, MCachedIDs.MTextPeer_selectMapNotifyMID);
	}
}
