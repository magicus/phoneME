/*
 * @(#)GDialogPeer.c	1.14 06/10/10
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
#include "sun_awt_gtk_GDialogPeer.h"
#include "GDialogPeer.h"

jboolean
awt_gtk_GDialogPeerData_init (JNIEnv *env,
							 jobject this,
							 GDialogPeerData *data,
							 GtkWidget *window,
							 jboolean canAddToPanel)
{
	/* Initialise the window peer data. */
	
	return awt_gtk_GWindowPeerData_init (env, this, (GWindowPeerData*)data, window, canAddToPanel);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GDialogPeer_create (JNIEnv *env, jobject this)
{
	GDialogPeerData *data = calloc (1, sizeof (GDialogPeerData));
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	awt_gtk_GDialogPeerData_init (env, this, data, gtk_window_new (GTK_WINDOW_DIALOG), JNI_TRUE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GDialogPeer_setTitleNative (JNIEnv *env, jobject this, jbyteArray title)
{
	GComponentPeerData *data = (GComponentPeerData *)awt_gtk_getData (env, this);
	gchar *gtitle;
	
	if (data == NULL)
		return;
	
	gtitle = (gchar *)(*env)->GetByteArrayElements (env, title, NULL);
	
	awt_gtk_threadsEnter();
	gtk_window_set_title (GTK_WINDOW(data->widget), gtitle);
	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, title, gtitle, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GDialogPeer_setModal (JNIEnv *env, jobject this, jboolean modal)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_window_set_modal (GTK_WINDOW(data->widget), (gboolean)modal);
	awt_gtk_threadsLeave();
}
