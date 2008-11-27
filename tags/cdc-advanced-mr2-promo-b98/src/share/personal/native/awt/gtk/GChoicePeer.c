/*
 * @(#)GChoicePeer.c	1.16 06/10/10
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
#include "sun_awt_gtk_GChoicePeer.h"
#include "GChoicePeer.h"

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GChoicePeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GChoicePeer_postItemEventMID, "postItemEvent", "(I)V");

	cls = (*env)->FindClass (env, "java/awt/Choice");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_Choice_selectedIndexFID, "selectedIndex", "I");
}

static void
choiceSelected (GtkMenuShell *menuShell, GChoicePeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	jobject target;
	JNIEnv *env;
	GtkWidget *active;
	GList *children;
	int index;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
		
	active = gtk_menu_get_active (GTK_MENU(menuShell));
	children = gtk_container_children (GTK_CONTAINER(menuShell));
	index = g_list_index (children, (gpointer)active);
	
	if (index != -1)
	{
		GtkOptionMenu *optionMenu;
	
		target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
		(*env)->SetIntField (env, target, GCachedIDs.java_awt_Choice_selectedIndexFID, (jint)index);
		(*env)->CallVoidMethod (env, this, GCachedIDs.GChoicePeer_postItemEventMID, (jint)index);

		/*
		** Workaround:
		** the popup is gone now, let's set the focus back to the optionMenu
		*/
		optionMenu = GTK_OPTION_MENU(((GComponentPeerData *)data)->widget);
		gtk_widget_grab_focus((GtkWidget *)optionMenu);
	}
	
	awt_gtk_callbackLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GChoicePeer_create (JNIEnv *env, jobject this)
{
	GChoicePeerData *data = (GChoicePeerData *)calloc (1, sizeof (GChoicePeerData));
	GtkWidget *optionMenu;
	GtkWidget *menu;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	/* Create the GtkOptionMenu with a GtkMenu to display the choices. */
	
	optionMenu = gtk_option_menu_new();
	data->menuWidget = menu = gtk_menu_new();
	gtk_option_menu_set_menu (GTK_OPTION_MENU(optionMenu), menu);
	gtk_widget_show (optionMenu);
	gtk_widget_show (menu);
	
	/* Connect a signal to the menu so we can listen in to selections. */
	
	gtk_signal_connect (GTK_OBJECT(menu), "selection-done", GTK_SIGNAL_FUNC (choiceSelected), data);
	
	/* Initialize GComponentPeerData. */

	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, optionMenu, optionMenu, optionMenu, JNI_FALSE);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GChoicePeer_addNative (JNIEnv *env, jobject this, jbyteArray itemText, jint index, jint selectedIndex)
{
	GChoicePeerData *data = (GChoicePeerData *)awt_gtk_getData (env, this);
	gchar *gitemText;
	GtkWidget *item;
	GtkOptionMenu *optionMenu;
	
	if (data == NULL)
		return;
	
	gitemText = (gchar *)(*env)->GetByteArrayElements (env, itemText, NULL);
	awt_gtk_threadsEnter();
	item = gtk_menu_item_new_with_label (gitemText);
        /* lock accelerators, otherwise if key events are sent to the item */
        /* then that key sequence will appear in the accelerator field     */
        gtk_widget_lock_accelerators (item);
	gtk_menu_insert (GTK_MENU(data->menuWidget), item, (gint)index);
	gtk_widget_show (item);
	
	/* We need to remove the menu and then add it again inorder for gtk to
	   recalulate the preffered size. */
	   
	optionMenu = GTK_OPTION_MENU(((GComponentPeerData *)data)->widget);
	
	gtk_widget_ref (data->menuWidget);
	gtk_option_menu_remove_menu (optionMenu);
	gtk_option_menu_set_menu (optionMenu, data->menuWidget);
	gtk_widget_unref (data->menuWidget);
	
	/* Now make sure the correctly selected item is activated */
	
	gtk_option_menu_set_history (GTK_OPTION_MENU(((GComponentPeerData *)data)->widget), (gint)selectedIndex);
	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, itemText, gitemText, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GChoicePeer_removeNative (JNIEnv *env, jobject this, jint index, jint selectedIndex)
{
	GChoicePeerData *data = (GChoicePeerData *)awt_gtk_getData (env, this);
	GtkWidget *item;
	GList *children;
	GtkOptionMenu *optionMenu;
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	children = gtk_container_children (GTK_CONTAINER(data->menuWidget));
	item = GTK_WIDGET (g_list_nth_data (children, (gint)index));
	gtk_container_remove (GTK_CONTAINER(data->menuWidget), item);
	gtk_widget_destroy (item);
	
	/* We need to remove the menu and then add it again inorder for gtk to
	   recalulate the preffered size. It is necessary to reference the menu widget
	   first to prevent gtk_option_menu_remove_menu from destroying the menu. */
	
	optionMenu = GTK_OPTION_MENU(((GComponentPeerData *)data)->widget);
	
	gtk_widget_ref (data->menuWidget);
	gtk_option_menu_remove_menu (optionMenu);
	gtk_option_menu_set_menu (optionMenu, data->menuWidget);
	gtk_widget_unref (data->menuWidget);
	
	/* Now make sure the correctly selected item is activated */
	
	gtk_option_menu_set_history (GTK_OPTION_MENU(((GComponentPeerData *)data)->widget), (gint)selectedIndex);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GChoicePeer_select (JNIEnv *env, jobject this, jint index)
{
    GChoicePeerData *data = (GChoicePeerData *)awt_gtk_getData (env, this);
    GtkOptionMenu *optionMenu;
    GtkMenu *optionMenuMenu;
    GtkWidget *orig_active;
    int orig_index;

    if (data == NULL)
        return;
	
    optionMenu = GTK_OPTION_MENU(((GComponentPeerData *)data)->widget);
    optionMenuMenu = (GtkMenu *)gtk_option_menu_get_menu(optionMenu);

    awt_gtk_threadsEnter();
    /*
    ** check to see if we need an ITEM_CHANGED event for this
    ** gtk doesn't call the callback when changes programmatically
    */
    orig_active = gtk_menu_get_active (optionMenuMenu);
    orig_index = g_list_index (GTK_MENU_SHELL (optionMenuMenu)->children, orig_active);
    
    if (index != orig_index) {
      (*env)->CallVoidMethod (env, this, GCachedIDs.GChoicePeer_postItemEventMID, (jint)index);
    }

    gtk_option_menu_set_history(optionMenu, (gint)index);
    /* since we are selecting an item */
    /* deactivate the pop-up: bugid 4714552 */
    gtk_menu_shell_deactivate((GtkMenuShell *)optionMenuMenu);
    awt_gtk_threadsLeave();
}

