/*
 * @(#)GButtonPeer.c	1.14 06/10/10
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

#include "jni.h"
#include "sun_awt_gtk_GButtonPeer.h"
#include "GButtonPeer.h"

/* Called when the button is clicked. Posts an ActionEvent to the AWT event dispatch
   thread for processing. */

static void
buttonClicked (GtkButton *button, GButtonPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
		
	(*env)->CallVoidMethod (env, this, GCachedIDs.GButtonPeer_postActionEventMID);
	
	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
		
	awt_gtk_callbackLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GButtonPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GButtonPeer_postActionEventMID, "postActionEvent", "()V");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GButtonPeer_create (JNIEnv *env, jobject this)
{
	GButtonPeerData *data = (GButtonPeerData *)calloc (1, sizeof (GButtonPeerData));
	GtkWidget *button;
	GtkWidget *label;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	/* Create the button with a label object inside it. */
	
	button = gtk_button_new ();
	data->labelWidget = label = gtk_label_new ("");
	gtk_container_add (GTK_CONTAINER(button), label);
	gtk_widget_show_all (button);
	
	/* Connect a signal so that when the button is clicked it notifies
	   the EventDispatchThread with an ActionEvent. */
	
	gtk_signal_connect (GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC (buttonClicked), data);
	
	/* Initialize GComponentPeerData using button as the main widget and for drawing. */

	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, button, button, button, JNI_FALSE);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GButtonPeer_setLabelNative (JNIEnv *env, jobject this, jbyteArray label)
{
	GButtonPeerData *data = (GButtonPeerData *)awt_gtk_getData (env, this);
	gchar *glabel;
	
	if (data == NULL)
		return;
	
	/* The label should be a NUL terminated multibyte array */
	
	glabel = (gchar *)(*env)->GetByteArrayElements (env, label, NULL);
	
	/* Set the label on the label widget inside the button. */
	
	awt_gtk_threadsEnter();
	gtk_label_set_text(GTK_LABEL(data->labelWidget), glabel);
	awt_gtk_threadsLeave();
	
	/* Gtk makes a copy of the label so we can free this here. */
	
	(*env)->ReleaseByteArrayElements (env, label, glabel, JNI_ABORT);
}



