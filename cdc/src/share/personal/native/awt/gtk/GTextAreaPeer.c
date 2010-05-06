/*
 * @(#)GTextAreaPeer.c	1.15 06/10/10
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
#include "java_awt_TextArea.h"
#include "sun_awt_gtk_GTextAreaPeer.h"
#include "GTextAreaPeer.h"
#include "GdkGraphics.h"

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextAreaPeer_createTextArea (JNIEnv *env, jobject this, jint scrollPolicy)
{
	GTextAreaPeerData *data = (GTextAreaPeerData *)calloc (1, sizeof (GTextAreaPeerData));
	GtkWidget *textArea;
	GtkWidget *scrollWindow;
	GtkWidget *eventBox;
	GtkPolicyType gscrollPolicyH, gscrollPolicyV;
        gint wordWrap, lineWrap;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();

	switch (scrollPolicy)
	{
       		case java_awt_TextArea_SCROLLBARS_VERTICAL_ONLY:
			gscrollPolicyV = GTK_POLICY_ALWAYS;
                        gscrollPolicyH = GTK_POLICY_NEVER;
                        wordWrap = TRUE;
                        lineWrap = TRUE;
		break;
		
		case java_awt_TextArea_SCROLLBARS_HORIZONTAL_ONLY:
			gscrollPolicyH = GTK_POLICY_ALWAYS;
                        gscrollPolicyV = GTK_POLICY_NEVER;
                        wordWrap = FALSE;
                        lineWrap = TRUE;
		break;
		
		case java_awt_TextArea_SCROLLBARS_NONE:
			gscrollPolicyH = gscrollPolicyV = GTK_POLICY_NEVER;
                        wordWrap = TRUE;
                        lineWrap = TRUE;
		break;

	        case java_awt_TextArea_SCROLLBARS_BOTH:
		default:
			gscrollPolicyH = gscrollPolicyV = GTK_POLICY_ALWAYS;
                        wordWrap = FALSE;
                        lineWrap = TRUE;
		break;
	}
	
	scrollWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrollWindow), gscrollPolicyH, gscrollPolicyV);
	gtk_widget_show (scrollWindow);
	textArea = gtk_text_new(NULL, NULL);
	
	/* TODO: We have to use line wrap as GTk does not support horizontal scrolling in GtkText widgets yet.
	   This will need to be changed when this feature is supported. */
	
	gtk_text_set_line_wrap (GTK_TEXT(textArea), lineWrap);
	gtk_text_set_word_wrap (GTK_TEXT(textArea), wordWrap);
	gtk_widget_show (textArea);
	gtk_container_add (GTK_CONTAINER(scrollWindow), textArea);
	eventBox = gtk_event_box_new ();
	gtk_widget_show(eventBox);
	gtk_container_add (GTK_CONTAINER(eventBox), scrollWindow);
	awt_gtk_GTextComponentPeerData_init (env, this, (GTextComponentPeerData *)data, GTK_EDITABLE(textArea), eventBox);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextAreaPeer_insertNative (JNIEnv *env, jobject this, jbyteArray text, jint pos)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	const gchar *gtext;
	gint position;
	
	if (data == NULL)
		return;
	
	gtext = (gchar *)(*env)->GetByteArrayElements (env, text, NULL);
	
	awt_gtk_threadsEnter();

	position = gtk_text_get_length(GTK_TEXT(data->editableWidget));
        position = (pos < position) ? pos : position;

	gtk_editable_insert_text (data->editableWidget, gtext, strlen (gtext), &position);
	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, text, (jbyte *)gtext, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GTextAreaPeer_replaceRangeNative (JNIEnv *env, jobject this, jbyteArray text, jint start, jint end)
{
	GTextComponentPeerData *data = (GTextComponentPeerData *)awt_gtk_getData (env, this);
	const gchar *gtext;
	gint position;
	
	if (data == NULL)
		return;
	
	gtext = (gchar *)(*env)->GetByteArrayElements (env, text, NULL);
	awt_gtk_threadsEnter();

	position = gtk_text_get_length(GTK_TEXT(data->editableWidget));
        end = (end < position) ? end : position;

        position = start = start < end ? start : end;

	gtk_editable_delete_text(data->editableWidget, start, end);

	gtk_editable_insert_text (data->editableWidget, gtext, strlen (gtext), &position);

	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, text, (jbyte *)gtext, JNI_ABORT);
}
