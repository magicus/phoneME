/*
 * @(#)GScrollPanePeer.c	1.20 06/10/10
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
#include <string.h>

#include "jni.h"
#include "java_awt_Adjustable.h"
#include "java_awt_ScrollPane.h"
#include "sun_awt_gtk_GScrollPanePeer.h"
#include "GScrollPanePeer.h"

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_initIDs (JNIEnv *env, jclass cls)
{
	cls = (*env)->FindClass (env, "java/awt/ScrollPane");

	if (cls == NULL)
		return;

	GET_FIELD_ID (java_awt_ScrollPane_scrollbarDisplayPolicyFID, "scrollbarDisplayPolicy", "I");
	cls = (*env)->FindClass (env, "sun/awt/gtk/GScrollPanePeer");

	if (cls == NULL)
		return;

	GET_METHOD_ID (GScrollPanePeer_postAdjustableEventMID, "postAdjustableEvent", "(Ljava/awt/Adjustable;I)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_create (JNIEnv *env, jobject this)
{
	GScrollPanePeerData *data = (GScrollPanePeerData *)calloc (1, sizeof (GScrollPanePeerData));
	GtkWidget *scrolledWindow;
	GtkWidget *eventBox;
	jobject target;
	jint scrollbarDisplayPolicy;
	GtkPolicyType gscrollbarDisplayPolicy = GTK_POLICY_AUTOMATIC;

	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}

	data->scrolledWindowWidget = NULL;
	data->viewportWidget = NULL;
	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);

	/* Determine scrollbar policy to use. */

	scrollbarDisplayPolicy = (*env)->GetIntField (env, target, GCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID);

	switch (scrollbarDisplayPolicy)
	{
		case java_awt_ScrollPane_SCROLLBARS_ALWAYS:
			gscrollbarDisplayPolicy = GTK_POLICY_ALWAYS;
		break;

		case java_awt_ScrollPane_SCROLLBARS_NEVER:
			gscrollbarDisplayPolicy = GTK_POLICY_NEVER;
		break;

		case java_awt_ScrollPane_SCROLLBARS_AS_NEEDED:
		default:
			gscrollbarDisplayPolicy = GTK_POLICY_AUTOMATIC;
		break;
	}

	awt_gtk_threadsEnter();
	scrolledWindow = data->scrolledWindowWidget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledWindow), gscrollbarDisplayPolicy, gscrollbarDisplayPolicy);

	eventBox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER(eventBox), scrolledWindow);
	gtk_widget_show_all (eventBox);

	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, eventBox, eventBox, scrolledWindow, JNI_FALSE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_add (JNIEnv *env, jobject this, jobject peer)
{
	GScrollPanePeerData *data;
	GComponentPeerData *peerData;

	data = (GScrollPanePeerData *)awt_gtk_getData (env, this);

	if (data == NULL)
		return;

	peerData = awt_gtk_getData (env, peer);

	if (peerData == NULL)
		return;

	awt_gtk_threadsEnter();

	/* Create a viewport to display the child in. If we already have a child in a viewport
	   (because a child was added to the scrollpane before) then remove the old child. */

	if (data->viewportWidget != NULL)
	{
		GtkBin *bin = GTK_BIN(data->viewportWidget);

		if (bin->child != NULL)
			gtk_container_remove (GTK_CONTAINER(bin), bin->child);
	}

	else
	{
		GtkWidget *viewport = data->viewportWidget = gtk_viewport_new (NULL, NULL);
		gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
		gtk_widget_show (viewport);

		/* Add the viewport to the scrolled window. */

		gtk_container_add (GTK_CONTAINER(data->scrolledWindowWidget), viewport);
	}

        gtk_viewport_set_hadjustment(GTK_VIEWPORT(data->viewportWidget),
               gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget)));

        gtk_viewport_set_vadjustment(GTK_VIEWPORT(data->viewportWidget),
               gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget)));

	/* Add the child to the viewport. */
	gtk_container_add (GTK_CONTAINER(data->viewportWidget), peerData->widget);

	awt_gtk_threadsLeave();
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GScrollPanePeer_calculateHScrollbarHeight (JNIEnv *env, jclass cls)
{
	GtkWidget *scrollbar;
	GtkRequisition size;

	awt_gtk_threadsEnter();
	scrollbar = gtk_hscrollbar_new(NULL);
	gtk_widget_size_request (scrollbar, &size);
	gtk_widget_destroy (scrollbar);
	awt_gtk_threadsLeave();

	return size.height + 4;
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GScrollPanePeer_calculateVScrollbarWidth (JNIEnv *env, jclass cls)
{
	GtkWidget *scrollbar;
	GtkRequisition size;

	awt_gtk_threadsEnter();
	scrollbar = gtk_vscrollbar_new(NULL);
	gtk_widget_size_request (scrollbar, &size);
	gtk_widget_destroy (scrollbar);
	awt_gtk_threadsLeave();

	return size.width + 4;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_setUnitIncrementNative (JNIEnv *env, jobject this, jint orientation, jint increment)
{
	GScrollPanePeerData *data = (GScrollPanePeerData *)awt_gtk_getData (env, this);
	GtkAdjustment *adjustment;

	if (data == NULL)
		return;

	awt_gtk_threadsEnter();

	if (orientation == java_awt_Adjustable_HORIZONTAL)
		adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget));

	else adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget));

	if (adjustment != NULL)
	{
		adjustment->step_increment = increment;
		gtk_adjustment_changed(adjustment);
	}

	awt_gtk_threadsLeave();
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_setAdjusterNative(JNIEnv *env, jobject this, jint orientation, jint value, jint max, jint pageSize)
{
	GScrollPanePeerData *data = (GScrollPanePeerData *)awt_gtk_getData (env, this);
	GtkAdjustment *adjustment;

	if (data == NULL)
		return;

	awt_gtk_threadsEnter();

	if (orientation == java_awt_Adjustable_HORIZONTAL)
		adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget));

	else adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget));

	if (adjustment != NULL)
	{
		adjustment->page_size = (gfloat)pageSize;
                adjustment->lower = (pageSize==max) ? (gfloat)max : 0;
                adjustment->upper = (gfloat)max;
                adjustment->value = (gfloat)value;
		gtk_adjustment_changed(adjustment);
	}

	awt_gtk_threadsLeave();
}

static void
valueChanged (GtkAdjustment *adjustment, GAdjustablePeerData *data)
{
        JNIEnv *env;
        jobject scrollPanePeer = data->scrollPaneData.componentPeerData.peerGlobalRef;
        jobject adjuster = data->adjustable;
        jint value = (jint)adjustment->value;

        if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
                return;

        awt_gtk_callbackEnter();

        (*env)->CallVoidMethod(env, scrollPanePeer,
                         GCachedIDs.GScrollPanePeer_postAdjustableEventMID,
                         adjuster, value);

        if ((*env)->ExceptionCheck (env))
                (*env)->ExceptionDescribe (env);

        awt_gtk_callbackLeave();

}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_connectAdjuster(JNIEnv *env, jobject this, jobject adjuster, jint orientation)
{
	GScrollPanePeerData *data = (GScrollPanePeerData *)awt_gtk_getData (env, this);
        GAdjustablePeerData *adjData = (GAdjustablePeerData *)calloc(1, sizeof(GAdjustablePeerData));
        GtkAdjustment *adjustment;

        if (data == NULL)
                return;

        if (adjData == NULL)
                return;

        if((adjData->adjustable = (*env)->NewWeakGlobalRef(env, adjuster)) == NULL)
                return;

        memcpy(&(adjData->scrollPaneData), data, sizeof(GScrollPanePeerData));

        awt_gtk_threadsEnter();

        if (orientation == java_awt_Adjustable_HORIZONTAL)
                adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(data->scrolledWindowWidget));
        else adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(data-> scrolledWindowWidget));

        if (adjustment != NULL)
                gtk_signal_connect (GTK_OBJECT(adjustment), "value-changed", GTK_SIGNAL_FUNC (valueChanged), adjData);

        awt_gtk_threadsLeave();

}

JNIEXPORT jboolean JNICALL
Java_sun_awt_gtk_GScrollPanePeer_scrollbarVisible(JNIEnv *env, jobject this, jint orientation)
{
	jboolean visible = JNI_FALSE;
        GScrollPanePeerData *data = (GScrollPanePeerData *)awt_gtk_getData (env, this);
        GtkScrolledWindow *scrolledWindow = GTK_SCROLLED_WINDOW(data->scrolledWindowWidget);


	switch(orientation)
	{
		case java_awt_Adjustable_HORIZONTAL:
			visible = scrolledWindow->hscrollbar_visible;
			break;

		case java_awt_Adjustable_VERTICAL:
			visible = scrolledWindow->vscrollbar_visible;
			break;
	}

	return visible ? JNI_TRUE : JNI_FALSE;
}

/* Gives explict control over scrollpane's scrollbars */
JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollPanePeer_enableScrollbarsNative(JNIEnv *env,
							jobject this,
							jboolean hBarOn,
							jboolean vBarOn)
{
    
    GScrollPanePeerData *data = (GScrollPanePeerData *)awt_gtk_getData (env, this);
    GtkScrolledWindow *scrolledWindow = GTK_SCROLLED_WINDOW(data->scrolledWindowWidget);

    awt_gtk_threadsEnter();
    gtk_scrolled_window_set_policy(scrolledWindow,
				   ((hBarOn == JNI_TRUE) ? GTK_POLICY_ALWAYS:GTK_POLICY_NEVER),
				   ((vBarOn == JNI_TRUE) ? GTK_POLICY_ALWAYS:GTK_POLICY_NEVER));
    awt_gtk_threadsLeave();
}


