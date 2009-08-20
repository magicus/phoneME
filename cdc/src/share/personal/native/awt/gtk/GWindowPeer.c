/*
 * @(#)GWindowPeer.c	1.24 06/10/10
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

#include <gtk/gtk.h>

/* Unfortuantely, there are some features in the X API that the Gtk+ API does not support.
   We need to do some X specific calls for getting window border sizes and settings the bounds of the
   window. If Gtk frame buffer is used instead of Gtk on X these portions will need to be ported. */

#include <gdk/gdkx.h>

#include "jni.h"
#include "sun_awt_gtk_GWindowPeer.h"
#include "java_awt_event_WindowEvent.h"
#include "GWindowPeer.h"


static void
windowResized (GtkWidget *widget, GtkAllocation *allocation, GWindowPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	jobject target;
	jint topBorder, leftBorder, bottomBorder, rightBorder;

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;

	awt_gtk_callbackEnter();

	if (data->resizeCallback != NULL)
		data->resizeCallback (data, allocation);

	/* Because the size we get here is the size of our actaul X window and not the window manager's
	   we need to add the border sizes on becase Java expects the size of a window to include
	   any external border. */

	topBorder = (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_topBorderFID);
	leftBorder = (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_leftBorderFID);
	bottomBorder = (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_bottomBorderFID);
	rightBorder = (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_rightBorderFID);

	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
	(*env)->SetIntField (env, target, GCachedIDs.java_awt_Component_widthFID, (jint)allocation->width + leftBorder + rightBorder);
	(*env)->SetIntField (env, target, GCachedIDs.java_awt_Component_heightFID, (jint)allocation->height + topBorder + bottomBorder);

	(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_postResizedEventMID);

	awt_gtk_callbackLeave();

	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

static gboolean
windowMoved (GtkWidget *widget, GdkEventConfigure *event, GWindowPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	jobject target;
	jint x, y;
	jint oldX, oldY;

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return TRUE;

	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);

	/* Because the positoon is of our actual X window and not the window manager's we need to subtract
	   the top and left border sizes to get the position of the window on the screen. */

	x = event->x - (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_leftBorderFID);
	y = event->y - (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_topBorderFID);

	/* We get the old values to see if the window has actually been moved. */

	oldX = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Component_xFID);
	oldY = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Component_yFID);

	if (oldX != x || oldY != y)
	{
		(*env)->SetIntField (env, target, GCachedIDs.java_awt_Component_xFID, x);
		(*env)->SetIntField (env, target, GCachedIDs.java_awt_Component_yFID, y);

		(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_postMovedEventMID);

		if ((*env)->ExceptionCheck (env))
			(*env)->ExceptionDescribe (env);
	}

	return TRUE;
}

static void
postWindowEvent (GWindowPeerData *data, jint id)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_postWindowEventMID, id);

	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

static gint
windowDeleteEvent (GtkWidget *widget, GdkEvent *event, GWindowPeerData *data)
{
	postWindowEvent (data, java_awt_event_WindowEvent_WINDOW_CLOSING);
	return TRUE;
}

static gboolean
windowActivated (GtkWidget *widget, GdkEventFocus *event, GWindowPeerData *data)
{
	GtkWidget *grabWidget = gtk_grab_get_current();

	/* Gtk seems to post window activated events wehn a menu destroyed.
	   This workaround detects the menu and then ignores the activate event. */

	if (grabWidget == NULL || !GTK_IS_MENU(grabWidget))
		postWindowEvent (data, java_awt_event_WindowEvent_WINDOW_ACTIVATED);

	return TRUE;
}

static gboolean
windowDeactivated (GtkWidget *widget, GdkEventFocus *event, GWindowPeerData *data)
{
	GtkWidget *grabWidget = gtk_grab_get_current();

	/* Gtk seems to post window deactivated events wehn a menu is popped up.
	   This workaround detects the menu and then ignores the deactivate event. */

	if (grabWidget == NULL || !GTK_IS_MENU(grabWidget))
		postWindowEvent (data, java_awt_event_WindowEvent_WINDOW_DEACTIVATED);

	return TRUE;
}

static void
focusChanged (GtkWindow *window, GtkWidget *widget, GWindowPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	GComponentPeerData *newFocusData;

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;

	/* If no widget has focus then we say focus has moved to the window. */

	if (widget == NULL)
		newFocusData = (GComponentPeerData *)data;

	/* Determine which Java component just gained focus. */

	else newFocusData = awt_gtk_getDataFromWidget (widget);

	(*env)->CallVoidMethod (env, this,
							GCachedIDs.GWindowPeer_focusChangedMID,
							newFocusData->peerGlobalRef);

		if ((*env)->ExceptionCheck (env))
			(*env)->ExceptionDescribe (env);
}

/* When the window is displayed we need to determine what the window manager's borders are.
   Tlhis needs to be done when the window is displayed. Unfortunately, there are no Gtk+ APIs
   to perform this so we need to use X APIs for this purpose.
   These borders need to be set as the default values for future windows that get created.
   If the borders that we "guessed" in java are not correct then we need to resize the window
   to acount for this. */

static void
updateBorderSizes (GtkWidget *widget, GdkEvent *event, GWindowPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	jint topBorder = 0, leftBorder = 0, bottomBorder, rightBorder;
	jint frameX, frameY, frameWidth, frameHeight;
	jobject target;
	GdkWindow *window = widget->window;
	Display *display = GDK_WINDOW_XDISPLAY (window);
	Window xwindow = GDK_WINDOW_XWINDOW(window), xroot, xparent;
	unsigned int x, y, width, height, border, depth;
	unsigned int windowWidth, windowHeight;
	gboolean bordersChanged = FALSE;

	/* Not used but needed for XQueryTree. */

	Window *children;
  	unsigned int nchildren;

	awt_gtk_callbackEnter();


	/* Because this function is called when the X window for this window is mapped, it is possible
	   that the window has been destroyed. This happens most noticably on a remote X seession where
	   it may takes some time for the XMapEvent to reach the client and, by this time, a request to destroy
	   the window may have been performed. If we perform XQueryTree then we will get BadWindow errors
	   and Gdk's X error handler function will terminate the application. These X errors are actually
	   harmless and all we really need to do is not tell Java the new window's border sizes. Gdk has a mechanism
	   to ignore errors when we know the errors are harmless. This is done through the gdk_error_trap_push
	   function. */

	gdk_error_trap_push();

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		goto finish;

	/* Get the width of our X window. */

	XGetGeometry (display, xwindow, &xroot, &x, &y, &width, &height, &border, &depth);

	/* Now traverse up the window hierachy until we find the window manager's window (the X window whose
	   parent window is the root window). As we traverse up we increase our left and top borders. */

	while (TRUE)
	{
		if (!XQueryTree (display, xwindow,
		       &xroot, &xparent,
		       &children, &nchildren))
			goto finish;

		if (children != NULL)
			XFree (children);

		XGetGeometry (display, xwindow, &xroot, &x, &y, &windowWidth, &windowHeight, &border, &depth);

		if (xparent == xroot)	/* Found window manager's X window. */
			break;

		leftBorder += x;
		topBorder += y;
		xwindow = xparent;
	}

	rightBorder = windowWidth - width - leftBorder;
	bottomBorder = windowHeight - height - topBorder;

	/* Having determined the window borders we now need to store these values so that next time
	   a window of this type is created these values will be used for the borders. */

	(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_setDefaultBorders, topBorder, leftBorder, bottomBorder, rightBorder);

	/* If the insets have changed then notify the window so it can layout its components again. */

	if (topBorder != (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_topBorderFID))
	{
		(*env)->SetIntField (env, this, GCachedIDs.GWindowPeer_topBorderFID, topBorder);
		bordersChanged = TRUE;
	}

	if (leftBorder != (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_leftBorderFID))
	{
		(*env)->SetIntField (env, this, GCachedIDs.GWindowPeer_leftBorderFID, leftBorder);
		bordersChanged = TRUE;
	}

	if (bottomBorder != (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_bottomBorderFID))
	{
		(*env)->SetIntField (env, this, GCachedIDs.GWindowPeer_bottomBorderFID, bottomBorder);
		bordersChanged = TRUE;
	}

	if (rightBorder != (*env)->GetIntField (env, this, GCachedIDs.GWindowPeer_rightBorderFID))
	{
		(*env)->SetIntField (env, this, GCachedIDs.GWindowPeer_rightBorderFID, rightBorder);
		bordersChanged = TRUE;
	}

	if (bordersChanged)
	{
		(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_updateInsetsMID);

		/* Resize window to take acount of the different border sizes. */

		target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
		frameX = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Component_xFID);
		frameY = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Component_yFID);
		frameWidth = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Component_widthFID);
		frameHeight = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Component_heightFID);

		gdk_window_move_resize (window, x, y, frameWidth - leftBorder - rightBorder, frameHeight - topBorder - bottomBorder);
	}

finish:

	gdk_error_trap_pop();


	awt_gtk_callbackLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_FIELD_ID (GWindowPeer_topBorderFID, "topBorder", "I");
	GET_FIELD_ID (GWindowPeer_leftBorderFID, "leftBorder", "I");
	GET_FIELD_ID (GWindowPeer_bottomBorderFID, "bottomBorder", "I");
	GET_FIELD_ID (GWindowPeer_rightBorderFID, "rightBorder", "I");
	GET_METHOD_ID (GWindowPeer_updateInsetsMID, "updateInsets", "()V");
	GET_METHOD_ID (GWindowPeer_postResizedEventMID, "postResizedEvent", "()V");
	GET_METHOD_ID (GWindowPeer_postMovedEventMID, "postMovedEvent", "()V");
	GET_METHOD_ID (GWindowPeer_postWindowEventMID, "postWindowEvent", "(I)V");
	GET_METHOD_ID (GWindowPeer_setDefaultBorders, "setDefaultBorders", "(IIII)V");
	GET_METHOD_ID (GWindowPeer_focusChangedMID, "focusChanged", "(Lsun/awt/gtk/GComponentPeer;)V");
}

jboolean
awt_gtk_GWindowPeerData_init (JNIEnv *env, jobject this, GWindowPeerData *data, GtkWidget *window, jboolean canAddToPanel)
{
	GtkWidget *drawWidget = NULL;
	GtkObject *windowObject = GTK_OBJECT(window);

	/* If we can't add to the panel (eg for a FileDialog) then we use the window as the drawWidget.
	   Normally we would use the panel but as this is not created in this case we use the window. */

	if (!canAddToPanel)
		drawWidget = window;

	/* Initialize the panel data using the window as the main widget and the GtkFixedContainer
	   as our drawing widget. */

	if (!awt_gtk_GPanelPeerData_init (env, this, (GPanelPeerData *)data, window, drawWidget, canAddToPanel))
		return JNI_FALSE;

	/* Connect a signal to notify the Frame that its size has changed. */

	gtk_signal_connect (windowObject, "size-allocate", GTK_SIGNAL_FUNC(windowResized), data);
	gtk_signal_connect (windowObject, "configure-event", GTK_SIGNAL_FUNC(windowMoved), data);
	gtk_signal_connect (windowObject, "delete-event", GTK_SIGNAL_FUNC(windowDeleteEvent), data);
	gtk_signal_connect_after (windowObject, "set-focus", GTK_SIGNAL_FUNC(focusChanged), data);
	gtk_signal_connect_after (windowObject, "focus-in-event", GTK_SIGNAL_FUNC(windowActivated), data);
	gtk_signal_connect_after (windowObject, "focus-out-event", GTK_SIGNAL_FUNC(windowDeactivated), data);

	/* When the window is mapped (ie made visible) we need to determine the borders. */

	gtk_signal_connect (windowObject, "map-event", GTK_SIGNAL_FUNC (updateBorderSizes), data);

	return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_create (JNIEnv *env, jobject this)
{
	GWindowPeerData *data = (GWindowPeerData *)calloc (1, sizeof (GWindowPeerData));
        GtkWidget *win;

	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}

	awt_gtk_threadsEnter();

	/* Gtk lacks an API to turn off the borders around a top level window. Origionally, we
	   tried GTK_WINDOW_POPUP which is a borderless window but this uses the override redirect
	   feature of X and so the window manager does not know about it and there are keyboard
	   focus problems. To get around this we create a GTK_WINDOW_TOPLEVEL and then realize it
	   in order to create the GDK window. We then use the GDK API to set the hints to tell the
	   window manager not to display any borders. */

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize (win);
	gdk_window_set_decorations (win->window, 0);
	gtk_window_set_policy (GTK_WINDOW(win), FALSE, FALSE, FALSE);

	awt_gtk_GWindowPeerData_init (env, this, data, win, JNI_TRUE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_requestFocus (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);

	if (data == NULL)
		return;

	awt_gtk_threadsEnter();

	gtk_window_set_focus (GTK_WINDOW(data->widget), NULL);

	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_setBoundsNative (JNIEnv *env, jobject this, jint x, jint y, jint width, jint height)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);

	if (data == NULL)
		return;

	awt_gtk_threadsEnter();

	gtk_widget_set_usize(data->widget, (gint)width, (gint)height);
	gtk_widget_set_uposition(data->widget, (gint)x, (gint)y);

	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_toFront (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);

	if (data == NULL || data->widget->window == NULL)
		return;

	awt_gtk_threadsEnter();
	gdk_window_raise (data->widget->window);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_toBack (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);

	if (data == NULL || data->widget->window == NULL)
		return;

	awt_gtk_threadsEnter();
	gdk_window_lower (data->widget->window);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_setOwner (JNIEnv *env, jobject this, jobject ownerPeer)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	GComponentPeerData *ownerData;

	if (data == NULL)
		return;

	ownerData = awt_gtk_getData (env, ownerPeer);

	if (ownerData == NULL)
		return;

	awt_gtk_threadsEnter();
	gtk_window_set_transient_for (GTK_WINDOW(data->widget), GTK_WINDOW(ownerData->widget));
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GWindowPeer_setResizable (JNIEnv *env, jobject this, jboolean resizable)
{
	GComponentPeerData *data = (GComponentPeerData *)awt_gtk_getData (env, this);

	if (data == NULL)
		return;

	awt_gtk_threadsEnter();

	if (resizable)
		gtk_window_set_policy (GTK_WINDOW(data->widget), TRUE, TRUE, TRUE);

	else
		gtk_window_set_policy (GTK_WINDOW(data->widget), FALSE, FALSE, FALSE);

	awt_gtk_threadsLeave();
}
