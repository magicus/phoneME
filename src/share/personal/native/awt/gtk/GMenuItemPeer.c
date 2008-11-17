/*
 * @(#)GMenuItemPeer.c	1.17 06/10/10
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
#include <ctype.h>

#include "jni.h"
#include "sun_awt_gtk_GMenuItemPeer.h"
#include "KeyCodes.h"
#include "GMenuItemPeer.h"
#include "GMenuBarPeer.h"

/* Called when the menu item is selected. Post an event to the AWT event dispatch thread
   for processing. */

static void
menuItemSelected (GtkMenuItem *item, GMenuItemPeerData *data)
{
	jobject this = data->peerGlobalRef;
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
		
	(*env)->CallVoidMethod (env, this, GCachedIDs.GMenuItemPeer_postActionEventMID);
	
	awt_gtk_callbackLeave();
	
	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuItemPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_FIELD_ID (GMenuItemPeer_targetFID, "target", "Ljava/awt/MenuItem;");
	GET_FIELD_ID (GMenuItemPeer_dataFID, "data", "I");
	GET_FIELD_ID (GMenuItemPeer_menuBarFID, "menuBar", "Lsun/awt/gtk/GMenuBarPeer;");
	GET_METHOD_ID (GMenuItemPeer_postActionEventMID, "postActionEvent", "()V");
	
	cls = (*env)->FindClass (env, "java/awt/MenuItem");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_MenuItem_shortcutFID, "shortcut", "Ljava/awt/MenuShortcut;");
	
	cls = (*env)->FindClass (env, "java/awt/MenuShortcut");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_MenuShortcut_keyFID, "key", "I");
	GET_FIELD_ID (java_awt_MenuShortcut_usesShiftFID, "usesShift", "Z");
}

void
awt_gtk_GMenuItemPeerData_init (JNIEnv *env, jobject this, GMenuItemPeerData *data, enum MenuItemType type)
{
	GtkWidget *menuItem;
	GtkWidget *label;
	
	data->peerGlobalRef = (*env)->NewGlobalRef (env, this);
	data->hasShortcutBeenSet = JNI_FALSE;
	
	/* Create the GtkMenuItem widget. */
	
	menuItem = data->menuItemWidget = (type == CHECK_MENU_ITEM_TYPE) ? gtk_check_menu_item_new () : gtk_menu_item_new ();
	gtk_widget_show (menuItem);
	
	/* Create a GtkLabel widget and add it to the menu item. */
	
	if (type != SEPARATOR_TYPE)
	{
		label = data->labelWidget = gtk_accel_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_container_add (GTK_CONTAINER (menuItem), label);
		gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menuItem);
		gtk_widget_show (label);
	
		/* Connect a signal to the menu item to post an event to the AWT event dispatch thread. */
	
		if(type != SUB_MENU)	
			gtk_signal_connect (GTK_OBJECT(menuItem), "activate", GTK_SIGNAL_FUNC (menuItemSelected), data);
	}
	
	(*env)->SetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID, (jint)data);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuItemPeer_create (JNIEnv *env, jobject this, jboolean isSeparator)
{
	GMenuItemPeerData *data = (GMenuItemPeerData *)calloc (1, sizeof (GMenuItemPeerData));
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	awt_gtk_GMenuItemPeerData_init (env, this, data, (isSeparator == JNI_TRUE) ? SEPARATOR_TYPE : MENU_ITEM_TYPE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuItemPeer_dispose (JNIEnv *env, jobject this)
{
	GMenuItemPeerData *data = (GMenuItemPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	gtk_widget_destroy (data->menuItemWidget);
	awt_gtk_threadsLeave();
	
	(*env)->SetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID, (jint)NULL);
	free (data);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuItemPeer_setLabelNative (JNIEnv *env, jobject this, jbyteArray label)
{
	GMenuItemPeerData *data = (GMenuItemPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID);
	gchar *glabel;
	int glabelLen, i;
	jobject menuBar;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	/* If we are a menu separator then don't try to set the label as no label
	   widget was created. */
	
	if (data->labelWidget == NULL)
		return;
	
	glabel = (gchar *)(*env)->GetByteArrayElements (env, label, NULL);
	glabelLen = strlen(glabel);

	for(i=0;i<glabelLen;i++)
		if(!isprint(glabel[i]))
			glabel[i] = ' ';
	
	awt_gtk_threadsEnter();
	
	/* Set the text on the label widget */
	
	gtk_label_set_text (GTK_LABEL(data->labelWidget), glabel);
	
	/* Check if the menu item has a shortcut. If so then add the shortcut.
	   First remove the previous shortcut from the menu if there was one. */
	
	if (data->hasShortcutBeenSet)
	{
		gtk_widget_remove_accelerator (data->menuItemWidget, data->accelGroup, data->accelKey, data->accelMods);
		data->hasShortcutBeenSet = JNI_FALSE;
	}

	menuBar = (*env)->GetObjectField (env, this, GCachedIDs.GMenuItemPeer_menuBarFID);
	
	if (menuBar != NULL)
	{
		/* Now get the MenuShortcut from the menu item and set the accelerator accordingly. */
		
		jobject target = (*env)->GetObjectField (env, this, GCachedIDs.GMenuItemPeer_targetFID);
		jobject shortcut = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_MenuItem_shortcutFID);
		
		if (shortcut != NULL)
		{
			/* Get the accel group from the menu bar. */
			
			GMenuBarPeerData *menuBarData = (GMenuBarPeerData *)(*env)->GetIntField (env, menuBar, GCachedIDs.GMenuBarPeer_dataFID);
			
			/* Determine the accelKey and mods from the menu shortcut object. */
			
			jint key = (*env)->GetIntField (env, shortcut, GCachedIDs.java_awt_MenuShortcut_keyFID);
			jboolean usesShift = (*env)->GetBooleanField (env, shortcut, GCachedIDs.java_awt_MenuShortcut_usesShiftFID);
			data->hasShortcutBeenSet = JNI_TRUE;
			data->accelGroup = menuBarData->accelGroup;
			data->accelKey = awt_gtk_getGdkKeyCode (key);
			
			if (usesShift == JNI_TRUE) {
			    data->accelMods = GDK_CONTROL_MASK | GDK_SHIFT_MASK;
			} else {
			    data->accelMods = GDK_CONTROL_MASK;
			}
			
			gtk_widget_add_accelerator (data->menuItemWidget, "activate", data->accelGroup, data->accelKey, data->accelMods, GTK_ACCEL_VISIBLE);
		}
	}
	
	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, label, glabel, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuItemPeer_setEnabled (JNIEnv *env, jobject this, jboolean enabled)
{
	GMenuItemPeerData *data = (GMenuItemPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID);
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	gtk_widget_set_sensitive (data->menuItemWidget, (gboolean)enabled);
	awt_gtk_threadsLeave();
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GMenuItemPeer_setFont(JNIEnv *env, jobject this, jobject font)
{
/*
	GMenuItemPeerData *data = (GMenuItemPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GMenuItemPeer_dataFID);

		if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();

        if(data->labelWidget != NULL)
        {
            jobject fontPeer = (*env)->GetObjectField (env, font, GCachedIDs.java_awt_Font_peerFID);
            GdkFont *gdkfont = (GdkFont *)(*env)->GetIntField (env, fontPeer, GCachedIDs.GFontPeer_dataFID);

            if(gdkfont != NULL && gdkfont != data->font)
            {
                GtkStyle *style = gtk_style_copy(data->labelWidget->style);

                if(style != NULL)
                {
                    if(data->font != NULL)
                        gdk_font_unref(data->font);

                    gdk_font_ref (gdkfont);
                    data->font = gdkfont;

                    style->font = gdkfont;
                    gtk_widget_set_style(data->labelWidget, style);
                    gtk_style_unref(style);
                }
            }
        }

	awt_gtk_threadsLeave();
*/
}



