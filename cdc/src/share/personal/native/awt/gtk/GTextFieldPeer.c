/*
 * @(#)GTextFieldPeer.c	1.12 06/10/10
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
#include <gdk/gdkkeysyms.h>

#include "jni.h"
#include "sun_awt_gtk_GTextFieldPeer.h"
#include "GTextFieldPeer.h"

static gboolean
keyPressed (GtkWidget *widget, GdkEventKey *event, GTextFieldPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	
	if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
		if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
			return TRUE;
			
		awt_gtk_callbackEnter();
			
		(*env)->CallVoidMethod (env, this, GCachedIDs.GTextFieldPeer_postActionEventMID);
		
		if ((*env)->ExceptionCheck (env))
			(*env)->ExceptionDescribe (env);
			
		awt_gtk_callbackLeave();
	}
	
	return TRUE;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextFieldPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GTextFieldPeer_postActionEventMID, "postActionEvent", "()V");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextFieldPeer_create (JNIEnv *env, jobject this)
{
	GTextFieldPeerData *data = (GTextFieldPeerData *)calloc (1, sizeof (GTextFieldPeerData));
	GtkWidget *textField;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	textField = gtk_entry_new();
	gtk_widget_show (textField);
	awt_gtk_GTextComponentPeerData_init (env, this, (GTextComponentPeerData *)data, GTK_EDITABLE(textField), textField);
	
	/* Call signal handler so that when return is pressed we can send an action event to Java. */
	
	gtk_signal_connect (GTK_OBJECT(textField), "key-press-event", GTK_SIGNAL_FUNC (keyPressed), data);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextFieldPeer_setEchoChar (JNIEnv *env, jobject this, jchar c)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);

	if (data == NULL)
		return;
	
	/* Gtk does not supoport setting the actual character displayed in a text field.
	   The best we can do is to set the visibility to false if the echo character
	   is not the null character. This results in Gtk displaying '*'s. */
	awt_gtk_threadsEnter();	   
	gtk_entry_set_visibility (GTK_ENTRY(data->editableWidget), (c == 0));
        awt_gtk_threadsLeave();
}
