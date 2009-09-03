/*
 * @(#)GMenuBarPeer.c	1.10 06/10/10
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
#include "sun_awt_gtk_GMenuBarPeer.h"
#include "GMenuBarPeer.h"
#include "GMenuItemPeer.h"
#include "GFramePeer.h"

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuBarPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_FIELD_ID (GMenuBarPeer_dataFID, "data", "I");
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuBarPeer_create (JNIEnv *env, jobject this)
{
	GMenuBarPeerData *data = (GMenuBarPeerData *)calloc (1, sizeof (GMenuBarPeerData));
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	data->accelGroup = gtk_accel_group_new ();
	data->menuBarWidget = gtk_menu_bar_new ();
	data->frameData = NULL;
	gtk_widget_show (data->menuBarWidget);
	
	/* We add a reference count to the menu bar because when we set the menu bar on a frame to null
	   the menu bar gets removed from the frame. This causes Gtk to derease the reference count and the
	   menu bar will actually be destroyed at this point. However, we don't want this to happen until this destroy
	   method is called. We ensure this by increasing the reference count here. */
	   
	gtk_widget_ref (data->menuBarWidget);
	
	awt_gtk_threadsLeave();
	
	(*env)->SetIntField (env, this, GCachedIDs.GMenuBarPeer_dataFID, (jint)data);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuBarPeer_dispose (JNIEnv *env, jobject this)
{
	GMenuBarPeerData *data = (GMenuBarPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuBarPeer_dataFID);
	
	if (data == NULL || data->menuBarWidget == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	gtk_widget_destroy (data->menuBarWidget);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuBarPeer_add (JNIEnv *env, jobject this, jobject menuItemPeer)
{
	GMenuBarPeerData *data = (GMenuBarPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuBarPeer_dataFID);
	GMenuItemPeerData *menuItemData = (GMenuItemPeerData *)(*env)->GetIntField (env, menuItemPeer, GCachedIDs.GMenuItemPeer_dataFID);
	GtkWidget *menuBar;
	
	if (data == NULL || data->menuBarWidget == NULL || menuItemData == NULL || menuItemData->menuItemWidget == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	menuBar = data->menuBarWidget;
	
	gtk_menu_bar_append (GTK_MENU_BAR(menuBar), menuItemData->menuItemWidget);
	
	/* If this menu bar is attatched to a frame then recalculate the preferred height for the menu bar
	   now that a new menu has been added. If the preferred height has changed then we need to resize
	   the menu bar to it's preferred height and tell the frame to relayout it's children. */
	
	if (data->frameData != NULL)
	{
		GtkRequisition menuBarSize;
		int oldHeight = menuBar->allocation.height;
		
		gtk_widget_size_request (menuBar, &menuBarSize);
		
		if (menuBarSize.height != oldHeight)
		{
			jobject framePeer = ((GComponentPeerData *)data->frameData)->peerGlobalRef;
			GtkAllocation allocation = menuBar->allocation;
		
			allocation.height = menuBarSize.height;
			gtk_widget_size_allocate (GTK_WIDGET(menuBar), &allocation);
			
			/* Now tell the frame the new height of the menu bar and ask it to update its insets. */
			
			(*env)->SetIntField (env, framePeer, GCachedIDs.GFramePeer_menuBarHeightFID, (jint)menuBarSize.height);
			(*env)->CallVoidMethod (env, framePeer, GCachedIDs.GWindowPeer_updateInsetsMID);
		}
	}
	
	awt_gtk_threadsLeave();
}
