/*
 * @(#)GCheckboxPeer.c	1.12 06/10/10
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
#include "GCheckboxPeer.h"

static void
checkboxToggled (GtkToggleButton *checkbox, GCheckboxPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	jboolean state = (jboolean)gtk_toggle_button_get_active (checkbox);
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
		
	(*env)->CallVoidMethod (env, this, GCachedIDs.GCheckboxPeer_postItemEventMID, state);
	
	awt_gtk_callbackLeave();
	
	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GCheckboxPeer_postItemEventMID, "postItemEvent", "(Z)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxPeer_create (JNIEnv *env, jobject this, jboolean radio)
{
	GCheckboxPeerData *data = (GCheckboxPeerData *)calloc (1, sizeof (GCheckboxPeerData));
	GtkWidget *checkbox;
	GtkWidget *label;
	GtkWidget *eventBox;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	/* Create the checkbox with a label object inside it. */
	
	if (radio)
	{
		/* For a radio button we create 2 radio buttons (one which is never shown). This
		   is needed because Gtk won't allow a radio button to be unselected if it is the
		   only radio button in the group. */
	
		checkbox = gtk_radio_button_new (NULL);
		data->hiddenRadioButtonWidget = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON(checkbox));
	}
	
	else
	{
		checkbox = gtk_check_button_new ();
		data->hiddenRadioButtonWidget = NULL;
	}
	
	data->checkboxWidget =  checkbox;
	data->labelWidget = label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_container_add (GTK_CONTAINER(checkbox), label);
	gtk_widget_show (checkbox);
	gtk_widget_show (label);
	
	/* Create an event box to respond to events like mouse moves etc and add the checkbox to it. */
	
	eventBox = gtk_event_box_new();
	gtk_container_add (GTK_CONTAINER(eventBox), checkbox);
	gtk_widget_show (eventBox);
	
	/* Connect a signal so that when the button is clicked it notifies
	   the EventDispatchThread with an ActionEvent. */
	
	gtk_signal_connect (GTK_OBJECT(checkbox), "toggled", GTK_SIGNAL_FUNC (checkboxToggled), data);
	
	/* Initialize GComponentPeerData using button as the main widget and for drawing. */

	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, eventBox, eventBox, checkbox, JNI_FALSE);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxPeer_setState (JNIEnv *env, jobject this, jboolean state)
{
	GCheckboxPeerData *data = (GCheckboxPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GComponentPeer_dataFID);
	
	awt_gtk_threadsEnter();
	
	/* If we have a hidden radio button then this is a radio button. In order to deselect it
	    we select the hidden radio button. */
	    
	if (data->hiddenRadioButtonWidget != NULL && state == JNI_FALSE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(data->hiddenRadioButtonWidget), TRUE);
	
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(data->checkboxWidget), (gboolean)state);
		
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxPeer_setLabelNative (JNIEnv *env, jobject this, jbyteArray label)
{
	GCheckboxPeerData *data = (GCheckboxPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GComponentPeer_dataFID);
	gchar *glabel = NULL;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	/* The label should be a NUL terminated multibyte string */
	
	if (label != NULL)
		glabel = (gchar *)(*env)->GetByteArrayElements (env, label, NULL);
	
	/* Set the label on the label widget inside the button. */
	
	awt_gtk_threadsEnter();
	gtk_label_set_text(GTK_LABEL(data->labelWidget), glabel);
	awt_gtk_threadsLeave();
	
	/* Gtk makes a copy of the label so we can free this here. */
	
	if (label != NULL)
		(*env)->ReleaseByteArrayElements (env, label, glabel, JNI_ABORT);
}
