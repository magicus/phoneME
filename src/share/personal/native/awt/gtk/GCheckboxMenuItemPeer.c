/*
 * @(#)GCheckboxMenuItemPeer.c	1.10 06/10/10
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
#include "sun_awt_gtk_GCheckboxMenuItemPeer.h"
#include "GCheckboxMenuItemPeer.h"

static void
menuItemToggled (GtkCheckMenuItem *item, GCheckboxMenuItemPeerData *data)
{
	jobject this = ((GMenuItemPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	jboolean state = (item->active == TRUE) ? JNI_TRUE : JNI_FALSE;
	jobject target;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
		
	target = (*env)->GetObjectField (env, this, GCachedIDs.GMenuItemPeer_targetFID);
	(*env)->SetBooleanField (env, target, GCachedIDs.java_awt_CheckboxMenuItem_stateFID, state);
	(*env)->CallVoidMethod (env, this, GCachedIDs.GCheckboxMenuItemPeer_postItemEventMID, state);
	
	awt_gtk_callbackLeave();
	
	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxMenuItemPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GCheckboxMenuItemPeer_postItemEventMID, "postItemEvent", "(Z)V");

	cls = (*env)->FindClass (env, "java/awt/CheckboxMenuItem");
	
	if (cls == NULL)
		return;

	GET_FIELD_ID (java_awt_CheckboxMenuItem_stateFID, "state", "Z");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxMenuItemPeer_create (JNIEnv *env, jobject this)
{
	GCheckboxMenuItemPeerData *data = (GCheckboxMenuItemPeerData *)calloc (1, sizeof (GCheckboxMenuItemPeerData));
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	/* Initialize the menu item data for this checkbox menu item. */
	
	awt_gtk_GMenuItemPeerData_init (env, this, (GMenuItemPeerData *)data, CHECK_MENU_ITEM_TYPE);
	
	/* Connect a signal to post an ItemEvent when the state is toggled. */
	
	gtk_signal_connect (GTK_OBJECT (((GMenuItemPeerData *)data)->menuItemWidget), "toggled", GTK_SIGNAL_FUNC (menuItemToggled), data);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GCheckboxMenuItemPeer_setState (JNIEnv *env, jobject this, jboolean state)
{
	GMenuItemPeerData *data = (GMenuItemPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID);
	
	awt_gtk_threadsEnter();
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM(data->menuItemWidget), (gboolean)state);
	awt_gtk_threadsLeave();
}
