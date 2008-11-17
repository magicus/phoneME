/*
 * @(#)GMenuPeer.c	1.12 06/10/10
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
#include "sun_awt_gtk_GMenuPeer.h"
#include "GMenuPeer.h"

void
awt_gtk_GMenuPeerData_init (JNIEnv *env, jobject this, GMenuPeerData *data)
{
    jobject menuObj;
    jboolean tearOff;
    GtkWidget *tearOffItem;

    awt_gtk_GMenuItemPeerData_init (env, this, (GMenuItemPeerData *)data, SUB_MENU);
    
    /* Create a menu and connect it to the menu item. */
    
    data->menuWidget = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM(((GMenuItemPeerData
					       *)data)->menuItemWidget), data->menuWidget);

    /* Get tearOff setting. */
    menuObj = (*env)->GetObjectField (env, this,
				      GCachedIDs.GMenuItemPeer_targetFID);
    tearOff = (*env)->GetBooleanField (env, menuObj,
				       GCachedIDs.java_awt_Menu_tearOffFID);
    
    if (tearOff == JNI_TRUE) {
	tearOffItem = gtk_tearoff_menu_item_new();
	gtk_widget_show(tearOffItem);
	gtk_menu_append(GTK_MENU(data->menuWidget), tearOffItem);
    }

}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuPeer_initIDs (JNIEnv *env, jclass cls)
{
    cls = (*env)->FindClass (env, "java/awt/Menu");
    
    if (cls == NULL)
	return;

    GET_FIELD_ID (java_awt_Menu_tearOffFID, "tearOff", "Z");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuPeer_create (JNIEnv *env, jobject this)
{
	GMenuPeerData *data = (GMenuPeerData *)calloc (1, sizeof (GMenuPeerData));
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	awt_gtk_GMenuPeerData_init(env, this, data);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuPeer_add (JNIEnv *env, jobject this, jobject menuItemPeer)
{
	GMenuPeerData *data = (GMenuPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID);
	GMenuItemPeerData *menuItemData = (GMenuItemPeerData *)(*env)->GetIntField (env, menuItemPeer, GCachedIDs.GMenuItemPeer_dataFID);
	
	if (data == NULL || data->menuWidget == NULL || menuItemData == NULL || menuItemData->menuItemWidget == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	gtk_menu_append (GTK_MENU(data->menuWidget), menuItemData->menuItemWidget);
	awt_gtk_threadsLeave();
}
