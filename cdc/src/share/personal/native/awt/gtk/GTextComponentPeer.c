/*
 * @(#)GTextComponentPeer.c	1.18 06/10/10
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
#include "sun_awt_gtk_GTextComponentPeer.h"
#include "GTextComponentPeer.h"

#include <string.h>

/* Callback function called when the value of the GtkEditable widget changes.
   Posts an event to the AWT event dispatch thread for processing. */

static void
textChanged (GtkEditable *editable, GTextComponentPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;

	awt_gtk_callbackEnter();
	
	(*env)->CallVoidMethod (env, this, GCachedIDs.GTextComponentPeer_postTextEventMID);
	
	awt_gtk_callbackLeave();
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextComponentPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GTextComponentPeer_postTextEventMID, "postTextEvent", "()V");
}

jboolean
awt_gtk_GTextComponentPeerData_init (JNIEnv *env,
                                     jobject this,
                                     GTextComponentPeerData *data,
                                     GtkEditable *editable,
                                     GtkWidget *widget)
{
	data->editableWidget = editable;
	
	gtk_signal_connect (GTK_OBJECT(editable), "changed", GTK_SIGNAL_FUNC (textChanged), data);
	
	return awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, widget, widget, GTK_WIDGET(editable), JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextComponentPeer_setEditable (JNIEnv *env, jobject this, jboolean editable)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_editable_set_editable (data->editableWidget, (gboolean)editable);
	awt_gtk_threadsLeave();
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GTextComponentPeer_getTextNative (JNIEnv *env, jobject this)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	gchar *gtext;
	jbyteArray text;
	
	if (data == NULL)
		return NULL;
	
	awt_gtk_threadsEnter();
	gtext = (gchar *)gtk_editable_get_chars (data->editableWidget, 0, -1);

	text = NULL;

	if(gtext!=NULL)
	{
	    int gtextLen = strlen(gtext);

	    if((text = (*env)->NewByteArray (env, gtextLen))!=NULL)
	        (*env)->SetByteArrayRegion(env, text, 0, gtextLen, (jbyte *)gtext);

	    g_free (gtext);
	}

	awt_gtk_threadsLeave();
	
	return text;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextComponentPeer_setTextNative (JNIEnv *env, jobject this, jbyteArray text)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	const gchar *gtext;
	int position = 0;
	
	if (data == NULL)
		return;
	
	gtext = (const gchar *)(*env)->GetByteArrayElements (env, text, NULL);

	awt_gtk_threadsEnter();
	gtk_editable_delete_text (data->editableWidget, 0, -1);
	gtk_editable_insert_text (data->editableWidget, gtext, strlen(gtext), &position);
	awt_gtk_threadsLeave();

	(*env)->ReleaseByteArrayElements (env, text, (jbyte *)gtext, JNI_ABORT);
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GTextComponentPeer_getSelectionStart (JNIEnv *env, jobject this)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	jint pos;
	
	if (data == NULL)
		return 0;
	
	awt_gtk_threadsEnter();

	pos = (jint)(data->editableWidget->selection_start_pos < data->editableWidget->selection_end_pos ?
                data->editableWidget->selection_start_pos : data->editableWidget->selection_end_pos);

	awt_gtk_threadsLeave();
	
	return pos;
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GTextComponentPeer_getSelectionEnd (JNIEnv *env, jobject this)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	jint pos;
	
	if (data == NULL)
		return 0;
	
	awt_gtk_threadsEnter();

	pos = (jint)(data->editableWidget->selection_end_pos > data->editableWidget->selection_start_pos ?
                data->editableWidget->selection_end_pos : data->editableWidget->selection_start_pos);

	awt_gtk_threadsLeave();
	
	return pos;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextComponentPeer_select (JNIEnv *env, jobject this, jint selStart, jint selEnd)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();

	gtk_editable_select_region (data->editableWidget, (gint)selStart, (gint)selEnd);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextComponentPeer_setCaretPosition (JNIEnv *env, jobject this, jint pos)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_editable_set_position (data->editableWidget, (gint)pos);
	awt_gtk_threadsLeave();
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GTextComponentPeer_getCaretPosition (JNIEnv *env, jobject this)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	jint pos;
	
	if (data == NULL)
		return 0;

	awt_gtk_threadsEnter();
	pos = (jint)gtk_editable_get_position (data->editableWidget);
	awt_gtk_threadsLeave();
	
	return pos;
}
