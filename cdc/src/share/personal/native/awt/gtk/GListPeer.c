/*
 * @(#)GListPeer.c	1.17 06/10/10
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
#include <gdk/gdkkeysyms.h>  // added for Bug #4685270

#include "jni.h"
#include "sun_awt_gtk_GListPeer.h"
#include "GListPeer.h"

static void
postItemEvent (GListPeerData *data, jint row, jboolean selected, jboolean postActionEvent)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
	
	awt_gtk_callbackEnter();
		
	(*env)->CallVoidMethod (env, this, GCachedIDs.GListPeer_postItemEventMID, (jint)row, selected, postActionEvent);
	
	awt_gtk_callbackLeave();
	
	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

static void
rowSelected (GtkCList *list, gint row, gint column, GdkEventButton *event, GListPeerData *data)
{
	postItemEvent (data, row, JNI_TRUE, (event != NULL && event->type == GDK_2BUTTON_PRESS));
}

static void
rowDeselected (GtkCList *list, gint row, gint column, GdkEventButton *event, GListPeerData *data)
{
	postItemEvent (data, row, JNI_FALSE, JNI_FALSE);
}

// added method for Bug #4685270
static gboolean
keyPressed (GtkWidget *widget, GdkEventKey *event, GListPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;

        gint row;

	if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
		if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
			return TRUE;
			
		awt_gtk_callbackEnter();

                row = GTK_CLIST(widget)->focus_row;

		(*env)->CallVoidMethod (env, this, GCachedIDs.GListPeer_postActionEventMID, (jint)row);
		
		if ((*env)->ExceptionCheck (env))
			(*env)->ExceptionDescribe (env);
			
		awt_gtk_callbackLeave();
	}
	
	return TRUE;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GListPeer_postItemEventMID, "postItemEvent", "(IZZ)V");
	GET_METHOD_ID (GListPeer_postActionEventMID, "postActionEvent", "(I)V"); // added for Bug #4685270
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_createNative (JNIEnv *env, jobject this, jint rowHeight)
{
	GListPeerData *data = (GListPeerData *)calloc (1, sizeof (GListPeerData));
	GtkWidget *list;
	GtkWidget *scrolledWindow;
	GtkWidget *eventBox;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	data->listWidget = list = gtk_clist_new(1);
	gtk_clist_set_column_title (GTK_CLIST(list), 0, "");
	gtk_clist_column_titles_passive (GTK_CLIST(list));
        gtk_clist_set_column_auto_resize(GTK_CLIST(list), 0, 1) ;
	gtk_clist_set_row_height(GTK_CLIST(list), rowHeight);

	/*gtk_clist_column_titles_show (GTK_CLIST(list));*/
	gtk_widget_show (list);
	scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolledWindow);
	gtk_container_add (GTK_CONTAINER(scrolledWindow), list);
	eventBox = gtk_event_box_new ();
	gtk_widget_show (eventBox);
	gtk_container_add (GTK_CONTAINER(eventBox), scrolledWindow);
	
	gtk_signal_connect (GTK_OBJECT(list), "select-row", GTK_SIGNAL_FUNC (rowSelected), data);
	gtk_signal_connect (GTK_OBJECT(list), "unselect-row", GTK_SIGNAL_FUNC (rowDeselected), data);

        /* added for Bug #4685270
	   Call signal handler so that when return is pressed we can send an action event to Java. */
	gtk_signal_connect (GTK_OBJECT(list), "key-press-event", GTK_SIGNAL_FUNC (keyPressed), data);

	/* Initialize GComponentPeerData. */

	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, eventBox, eventBox, list, JNI_TRUE);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT jintArray JNICALL
Java_sun_awt_gtk_GListPeer_getSelectedIndexes (JNIEnv *env, jobject this)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	jintArray selected;
	jint rowNum = 0;
	jint selectedIndex = 0;
	GList *item;
	GList *rows;
	int numSelected = 0;
	
	if (data == NULL)
		return NULL;

	awt_gtk_threadsEnter();

	rows = GTK_CLIST (data->listWidget)->row_list;
	item = g_list_first (rows);

	while (item != NULL)
	{
		GtkCListRow *row = GTK_CLIST_ROW (item);

		if ((row->state & GTK_STATE_SELECTED) == GTK_STATE_SELECTED)
			numSelected++;

		item = g_list_next (item);
	}

	/* Create an int array to store the selected indexes. */

	selected = (*env)->NewIntArray (env, numSelected);

	if (selected == NULL)
		return NULL;

	/* Go through the array and fill in the indexes of the selected rows. */

	item = g_list_first (rows);

	while (item != NULL)
	{
		GtkCListRow *row = GTK_CLIST_ROW (item);

		if ((row->state & GTK_STATE_SELECTED) == GTK_STATE_SELECTED)
			(*env)->SetIntArrayRegion (env, selected, selectedIndex++, 1, &rowNum);

		item = g_list_next (item);
		rowNum++;
	}

	awt_gtk_threadsLeave();

	return selected;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_addNative (JNIEnv *env, jobject this, jbyteArray itemText, jint index)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	gchar *gitemText;
	GList *items;
	GtkWidget *item;
	
	if (data == NULL)
		return;
	
	gitemText = (gchar *)(*env)->GetByteArrayElements (env, itemText, NULL);
	awt_gtk_threadsEnter();
	item = gtk_list_item_new_with_label(gitemText);
	gtk_widget_show (item);
	items = g_list_append (NULL, item);
	gtk_clist_insert (GTK_CLIST(data->listWidget), (gint)index, &gitemText);
	/*gtk_list_insert_items (GTK_LIST(data->listWidget), items, (gint)index);*/
	awt_gtk_threadsLeave();
	(*env)->ReleaseByteArrayElements (env, itemText, gitemText, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_delItems (JNIEnv *env, jobject this, jint start, jint end)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	jint i;
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	
	gtk_clist_freeze (GTK_CLIST(data->listWidget));
	
	for (i = start; i <= end; i++)
		gtk_clist_remove (GTK_CLIST(data->listWidget), (gint)start);
		
	gtk_clist_thaw (GTK_CLIST(data->listWidget));
		
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_removeAll (JNIEnv *env, jobject this)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_clist_clear (GTK_CLIST(data->listWidget));
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_select (JNIEnv *env, jobject this, jint index)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_clist_select_row (GTK_CLIST(data->listWidget), (gint)index, 0);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_deselect (JNIEnv *env, jobject this, jint index)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_clist_unselect_row (GTK_CLIST(data->listWidget), (gint)index, 0);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_makeVisible (JNIEnv *env, jobject this, jint index)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	
	if ((gtk_clist_row_is_visible (GTK_CLIST(data->listWidget), (gint)index) == GTK_VISIBILITY_NONE) ||
	    (gtk_clist_row_is_visible (GTK_CLIST(data->listWidget), (gint)index) == GTK_VISIBILITY_PARTIAL))  // added for Bug #4700138
		gtk_clist_moveto (GTK_CLIST(data->listWidget), (gint)index, 0, (gfloat)0.5, 0);
		
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_setMultipleMode (JNIEnv *env, jobject this, jboolean multipleMode)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_clist_set_selection_mode (GTK_CLIST(data->listWidget), (multipleMode == JNI_TRUE) ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);
	awt_gtk_threadsLeave();
}



JNIEXPORT void JNICALL
Java_sun_awt_gtk_GListPeer_setRowHeight (JNIEnv *env, jobject this, jint rowHeight)
{
	GListPeerData *data = (GListPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;

	awt_gtk_threadsEnter();
	gtk_clist_set_row_height(GTK_CLIST(data->listWidget), rowHeight);
	awt_gtk_threadsLeave();
}

