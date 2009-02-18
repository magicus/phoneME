/*
 * @(#)GFramePeer.c	1.23 06/10/10
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
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include "jni.h"
#include "sun_awt_gtk_GFramePeer.h"
#include "java_awt_event_WindowEvent.h"
#include "java_awt_Frame.h"
#include "GFramePeer.h"
#include "GMenuBarPeer.h"
#include "GPanelPeer.h"
#include "ImageRepresentation.h"

/* Defines the state values for the WM_STATE property. */

typedef enum
{
	WITHDRAWN_STATE = 0,
	NORMAL_STATE = 1,
	ICONIC_STATE = 3
} GtkWindowState;

static gint gtk_frame_key_press (GtkWidget   *widget,
                                 GdkEventKey *event,
                                 gpointer     user_data);


static GdkAtom WM_STATE;

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_FIELD_ID (GFramePeer_menuBarHeightFID, "menuBarHeight", "I");
	GET_METHOD_ID (GFramePeer_postWindowIconifiedMID, "postWindowIconified", "()V");
	GET_METHOD_ID (GFramePeer_postWindowDeiconifiedMID, "postWindowDeiconified", "()V");

	awt_gtk_threadsEnter();
	WM_STATE = gdk_atom_intern ("WM_STATE", FALSE);
	awt_gtk_threadsLeave();
}


/* Because the menu bar is added to the fixed container we need to do the resizing of the
   menubar ourselves. This function is called when the frame is resized so that the
   menu bar size is also updated. We set the width of the menu bar to be the new width of the
   window. */

static void
updateMenuBarWidth (GWindowPeerData *data, GtkAllocation *allocation)
{
	GMenuBarPeerData *menuBarData = ((GFramePeerData *)data)->menuBarData;
	GtkWidget *menuBar;

	/* If there is no menu bar attatched then don't need to do anything. */

	if (menuBarData != NULL && (menuBar = menuBarData->menuBarWidget) != NULL)
	{
		GtkAllocation menuBarAllocation;
		menuBarAllocation.x = 0;
		menuBarAllocation.y = 0;
		menuBarAllocation.width = allocation->width;
		menuBarAllocation.height = menuBar->allocation.height;

		gtk_widget_size_allocate (menuBar, &menuBarAllocation);
	}
}

/* Posts an event to Java to indicate that this window has been iconified. */

static void
postWindowIconified (GFramePeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod (env, this, GCachedIDs.GFramePeer_postWindowIconifiedMID);

	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

/* Posts an event to Java to indicate that this window has been deiconified. */

static void
postWindowDeiconified (GFramePeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;

	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;

	(*env)->CallVoidMethod (env, this, GCachedIDs.GFramePeer_postWindowDeiconifiedMID);

	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

/* Gets the current state of a window. */

static GtkWindowState
getWindowState (GtkWidget *window)
{
	GdkWindow *gdkWindow = window->window;
	GdkAtom actualType;
	gint actualFormat, actualLength;
	glong *data;
	GtkWindowState state = WITHDRAWN_STATE;

	if (window == NULL)
		return WITHDRAWN_STATE;

	if (gdk_property_get (gdkWindow, WM_STATE, WM_STATE, 0, 1, FALSE, &actualType, &actualFormat, &actualLength, (guchar **)&data))
	{
		state = (GtkWindowState)*data;
		g_free (data);
	}

	return state;
}

/* This is called when a property is changed on the window. Although Gdk provides an API for this
   it is very tied to X and not portable at all. It seems the philosophy of Gdk was to create a
   one to one mapping of functions and structures to the X ones. What a rediculous porting layer!
   Much better would have been to provide higher level abstractions and events than X provides. */

static gboolean
propertyChanged (GtkWidget *widget, GdkEventProperty *event, GFramePeerData *data)
{
	awt_gtk_callbackEnter();

	if (event->atom == WM_STATE && event->state == GDK_PROPERTY_NEW_VALUE)
	{
		gint state = getWindowState(widget);

		if (state == ICONIC_STATE)
		{
			if (data->iconified == FALSE && GTK_WIDGET_VISIBLE(widget))
				postWindowIconified (data);

			data->iconified = TRUE;
		}

		else if (state == NORMAL_STATE)
		{
			if (data->iconified == TRUE && GTK_WIDGET_VISIBLE(widget))
				postWindowDeiconified (data);

			data->iconified = FALSE;
		}
	}

	awt_gtk_callbackLeave();

	return TRUE;
}

/* Changes the state of a visible Frame to either iconic or normal.
   We have to use X lib functions for this method as Gtk does not provide an API
   for this functionality. */

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_setStateNative (JNIEnv *env, jobject this, jint state)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	GdkWindow *window = NULL;
	Window xwindow;
	Display *display;

	awt_gtk_threadsEnter();
	gdk_error_trap_push();

	if (data == NULL)
		goto finish;

	window = data->widget->window;

	if (window == NULL)
		goto finish;

	xwindow = GDK_WINDOW_XWINDOW(window);
	display = GDK_WINDOW_XDISPLAY(window);

	if (state == java_awt_Frame_ICONIFIED)
		XIconifyWindow(display, xwindow, 0);

	else
		XMapRaised (display, xwindow);

	XFlush(display);

finish:
	gdk_error_trap_pop();
	awt_gtk_threadsLeave();
}

/* This is called just before a frame is made visible. It has to set the WM_STATE
   hints on the window to let the window manager know the initial state the window should be
   opened in. Gtk has no support for this at all and this is a rather ugly work around.
   */

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_setWMStateHints (JNIEnv *env, jobject this, jboolean iconified)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	GdkWindow *window = NULL;
	Window xwindow;
	Display *display;
	XWMHints hints;

	awt_gtk_threadsEnter();
	gdk_error_trap_push();

	if (data == NULL)
		goto finish;

	/* Make sure the window has been created first. */

	gtk_widget_realize (data->widget);

	window = data->widget->window;

	if (window == NULL)
		goto finish;

	/* Now set the hints on the X window to indicate the initial state we would like the
	   window to be shown in. */

	xwindow = GDK_WINDOW_XWINDOW(window);
	display = GDK_WINDOW_XDISPLAY(window);

	hints.flags = StateHint;
	hints.initial_state = iconified ? IconicState : NormalState;

	XSetWMHints (display, xwindow, &hints);
	((GFramePeerData *)data)->iconified = iconified;

finish:
	gdk_error_trap_pop();
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_create (JNIEnv *env, jobject this)
{
	GFramePeerData *data = calloc (1, sizeof (GFramePeerData));
	GtkWidget *window;

	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}

	awt_gtk_threadsEnter();

	data->menuBarData = NULL;
	((GWindowPeerData *)data)->resizeCallback = updateMenuBarWidth;
	data->topBorder = 0;
	data->leftBorder = 0;
	data->bottomBorder = 0;
	data->rightBorder = 0;
	data->iconified = FALSE;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy (GTK_WINDOW(window), TRUE, TRUE, FALSE);

        /* register callback for key press event, so that arrow keys can be */
        /* filetered out.                                                   */

        gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
                           (GtkSignalFunc) gtk_frame_key_press, (gpointer)NULL);

        /*gtk_signal_connect (GTK_OBJECT (window), "map-event", (GtkSignalFunc) frameMapped, (gpointer)data);
        gtk_signal_connect (GTK_OBJECT (window), "unmap-event", (GtkSignalFunc) frameUnmapped, (gpointer)data);*/

        /* register callback for a change in a property on this window. The only property we are actually
           interested in is the WM_STATE property as this indicates that the window has changed state
           to or from iconic. */

        gtk_widget_add_events (window, GDK_PROPERTY_CHANGE_MASK);
        gtk_signal_connect (GTK_OBJECT (window), "property-notify-event", (GtkSignalFunc) propertyChanged, (gpointer)data);

	/* Initialise the window peer data. */

	awt_gtk_GWindowPeerData_init (env, this, (GWindowPeerData*)data, window, JNI_TRUE);
	awt_gtk_threadsLeave();
}

/*  Stop Gtk processing of arrow/tab keys (unless its a text widget that can use */
/*  these keys), by using gtk_signal_emit_stop_by_name.                          */
static gint
gtk_frame_key_press (GtkWidget   *widget,
                     GdkEventKey *event,
                     gpointer     user_data)
{

    GtkWidget *focusWidget = NULL;
    gboolean isTextWidget = FALSE;
    gboolean isCtrlTab = FALSE;
    guint kval;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    /* is current focus on a text, list or file selection */
    /* and is ctrl-tab being pressed ?                    */
    kval = event->keyval;
    if(GTK_IS_WINDOW(widget)) {
        if((focusWidget = ((GtkWindow *)widget)->focus_widget)) {
            isTextWidget = GTK_IS_ENTRY(focusWidget) || GTK_IS_TEXT(focusWidget) ||
                 GTK_IS_CLIST(focusWidget) || GTK_IS_FILE_SELECTION(focusWidget);
            isCtrlTab =  (kval == GDK_Tab || kval == GDK_ISO_Left_Tab) &&
                         (event->state & GDK_CONTROL_MASK);
        }
    }

    /* If its a text widget, then let them have a shot at the arrow/tab key event */
    /* i.e. return FALSE.                                                         */
    /* If not, stop further Gtk processing of these keys. Let the Java layer      */
    /* handle it. i.e. return TRUE                                                */
    /* Also return TRUE if ctrl-tab is being pressed in a text area               */
    /* Note: ctrl-tab can be used to navigate out   */
    /* of text areas, so needs to be passed up to   */
    /* Java's TabFocusManager                       */
    if ((!isTextWidget || (isTextWidget && isCtrlTab)) && (kval == GDK_Up ||
        kval == GDK_Down || kval == GDK_Left
        || kval == GDK_Right || kval == GDK_Tab ||
        kval == GDK_KP_Up || kval == GDK_KP_Down ||
        kval == GDK_KP_Left || kval == GDK_KP_Right ||
        kval == GDK_ISO_Left_Tab )) {

        gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");

        return TRUE;
      }

    return FALSE;
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_setTitleNative (JNIEnv *env, jobject this, jbyteArray title)
{
	GFramePeerData *data = (GFramePeerData *)awt_gtk_getData (env, this);
	gchar *gtitle;

	if (data == NULL)
		return;

	gtitle = (gchar *)(*env)->GetByteArrayElements (env, title, NULL);

	awt_gtk_threadsEnter();
	gtk_window_set_title (GTK_WINDOW(((GComponentPeerData *)data)->widget), gtitle);
	awt_gtk_threadsLeave();

	(*env)->ReleaseByteArrayElements (env, title, gtitle, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_setMenuBarNative (JNIEnv *env, jobject this, jobject menuBarPeer)
{
	GFramePeerData *data;
	GMenuBarPeerData *menuBarData;
	GtkRequisition menuBarSize;
	GtkAllocation menuBarAllocation;
	GtkContainer *panel;
	GtkWindow *window;

	if ((data = (GFramePeerData *)awt_gtk_getData (env, this)) == NULL)
		return;

	/* Get the window widget that represents this Java frame. */

	window = GTK_WINDOW(((GComponentPeerData *)data)->widget);

	/* Get the panel to which the menubar will be added. */

	panel = GTK_CONTAINER(((GPanelPeerData *)data)->panelWidget);

	/* If there is already a menu bar installed on this frame then remove it first. */

	if (data->menuBarData != NULL)
	{
		awt_gtk_threadsEnter();

		/* Remove menu bar from the panel. */

		gtk_container_remove (panel, data->menuBarData->menuBarWidget);

		/* Remove the menu bar accel group (used for menu shortcuts) from the window. */

		gtk_window_remove_accel_group (window, data->menuBarData->accelGroup);

		data->menuBarData->frameData = NULL;
		data->menuBarData = NULL;

		awt_gtk_threadsLeave();

		/* If we aren't going to supply a new menu bar then set the menu bar height to 0
		   and update the insets. */

		if (menuBarPeer == NULL)
		{
			(*env)->SetIntField (env, this, GCachedIDs.GFramePeer_menuBarHeightFID, 0);
			(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_updateInsetsMID);
		}
	}

	/* If we are supplying a new menu bar then install it. */

	if (menuBarPeer != NULL)
	{
		if ((menuBarData = (GMenuBarPeerData *)(*env)->GetIntField (env, menuBarPeer, GCachedIDs.GMenuBarPeer_dataFID)) == NULL)
			return;

		if (menuBarData->menuBarWidget == NULL)
		{
			(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
			return;
		}

		/* Store the menu bar in the frame's data. This is needed because we need to layout the
		   menu bar when the frame is resized ourselves. */

		data->menuBarData = menuBarData;

		/* Store this frame data in the menu bar's data. This is needed so that when new menus
		   are added and removed from the menu bar we can recaluclate the preferred height of the
		   menu bar and if this has changed then we can relayout the frame. */

		menuBarData->frameData = data;

		awt_gtk_threadsEnter();

		/* Add the menu bar accel group (used for menu shortcuts) to the window. */

		gtk_window_add_accel_group (window, menuBarData->accelGroup);

		/* Add the menu bar to the top of the fixed container widget and set its width to the width
		   of the panel and the preferred height of the menu bar. */

		gtk_container_add (panel, menuBarData->menuBarWidget);
		gtk_widget_size_request (menuBarData->menuBarWidget, &menuBarSize);
		menuBarAllocation.x = 0;
		menuBarAllocation.y = 0;
		menuBarAllocation.width = GTK_WIDGET(panel)->allocation.width;
		menuBarAllocation.height = menuBarSize.height;
		gtk_widget_size_allocate (menuBarData->menuBarWidget, &menuBarAllocation);

		/* Update the insets. */

		(*env)->SetIntField (env, this, GCachedIDs.GFramePeer_menuBarHeightFID, (jint)menuBarSize.height);
		(*env)->CallVoidMethod (env, this, GCachedIDs.GWindowPeer_updateInsetsMID);

		awt_gtk_threadsLeave();
	}
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFramePeer_setIconImageNative (JNIEnv *env, jobject this, jobject gdkImageRep)
{
    	ImageRepresentationData *imgData;
	GComponentPeerData *data;

	if ((data = (GComponentPeerData *)awt_gtk_getData (env, this)) == NULL)
		return;

	imgData = (ImageRepresentationData *) (*env)->GetIntField (env, gdkImageRep,
                                  GCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);

       if(imgData == NULL)
                return;

       awt_gtk_threadsEnter();
       gdk_window_set_icon(data->widget->window, NULL, imgData->pixmap, imgData->mask);
       awt_gtk_threadsLeave();

}
