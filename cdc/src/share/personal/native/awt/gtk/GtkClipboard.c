/*
 * @(#)GtkClipboard.c	1.8 06/10/10
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

#include <string.h>

#include <gtk/gtk.h>
#include <gtk/gtkinvisible.h>

#include "jni.h"
#include "sun_awt_gtk_GtkClipboard.h"
#include "awt.h"


static GdkAtom GDK_SELECTION_CLIPBOARD;


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GtkClipboard_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GtkClipboard_getStringContentsMID, "getStringContents", "()[B");
        GET_METHOD_ID (GtkClipboard_lostOwnershipMID, "lostOwnership", "(Ljava/awt/datatransfer/Clipboard;Ljava/awt/datatransfer/Transferable;)V"); 
	GET_METHOD_ID (GtkClipboard_updateContentsMID, "updateContents", "([B)V");

        GDK_SELECTION_CLIPBOARD = gdk_atom_intern("CLIPBOARD", FALSE);
}



/* We have lost the clipboard, someone else copied something to it */

static void
selectionClearEvent(GtkWidget *widget, GdkEventSelection *event, gpointer udata)
{
    jobject this = (jobject)udata;
    JNIEnv *env;

    if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
            return;

    awt_gtk_callbackEnter();

    (*env)->CallVoidMethod(env, this, GCachedIDs.GtkClipboard_lostOwnershipMID, NULL, NULL);

    awt_gtk_callbackLeave();
}


/* Callback which contains the contents of the Clipboard, this is called in response
 * gtk_selection_convert(), then copies the string back to java
 * We did a paste in Java.
 */

static void
selectionReceivedEvent(GtkWidget *widget, GtkSelectionData *data, guint time, gpointer udata)
{
    jobject this = (jobject)udata;
    JNIEnv *env;
    jbyteArray string;

    if ((data->length <= 0) || (data->type != GDK_SELECTION_TYPE_STRING))
        return;
    
    if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
        return;

    awt_gtk_callbackEnter();

    if((string = (*env)->NewByteArray(env, data->length)) != NULL)
    {
        (*env)->SetByteArrayRegion(env, string, 0, data->length, (jbyte *)data->data);

        (*env)->CallVoidMethod(env, this, GCachedIDs.GtkClipboard_updateContentsMID, string);
    }

    awt_gtk_callbackLeave();
}


/* Provides the string to the requestor. Copies from the Java Clipboard.
 * We did a copy in Java
 */

static void
selectionGetEvent(GtkWidget *widget, GtkSelectionData *data, guint info, guint time, gpointer udata)
{
    jobject this = (jobject)udata;
    JNIEnv *env;
    jbyteArray strContents;
    jbyte *text;

    if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
            return;

    awt_gtk_callbackEnter();

    strContents = (*env)->CallObjectMethod(env, this, GCachedIDs.GtkClipboard_getStringContentsMID);

    if(strContents != NULL)
    {
        jint textLen = (*env)->GetArrayLength(env, strContents);

        text = (*env)->GetByteArrayElements(env, strContents, NULL);

        gtk_selection_data_set(data, GDK_SELECTION_TYPE_STRING, 8, text, textLen);

        (*env)->ReleaseByteArrayElements(env, strContents, text, JNI_ABORT);
    }
    else
        gtk_selection_data_set(data, GDK_NONE, 8, NULL, 0);


    awt_gtk_callbackLeave();
}


JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GtkClipboard_createWidget(JNIEnv *env, jobject this)
{
    GtkWidget *clipboard;

    awt_gtk_threadsEnter();

    clipboard = gtk_invisible_new();

    this = (*env)->NewGlobalRef(env, this);

    gtk_selection_add_target(clipboard, GDK_SELECTION_CLIPBOARD, GDK_SELECTION_TYPE_STRING, (guint)this);

    gtk_signal_connect (GTK_OBJECT(clipboard), "selection-clear-event", GTK_SIGNAL_FUNC (selectionClearEvent), (gpointer)this);   
    gtk_signal_connect (GTK_OBJECT(clipboard), "selection-get", GTK_SIGNAL_FUNC (selectionGetEvent), (gpointer)this);
    gtk_signal_connect (GTK_OBJECT(clipboard), "selection-received", GTK_SIGNAL_FUNC (selectionReceivedEvent), (gpointer)this);

    awt_gtk_threadsLeave();

    return (jint)clipboard;
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GtkClipboard_destroyWidget(JNIEnv *env, jobject this, jint data)
{

    awt_gtk_threadsEnter();

    if(data != 0)
        gtk_widget_destroy((GtkWidget *)data);

    awt_gtk_threadsLeave();
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GtkClipboard_setNativeClipboard(JNIEnv *env, jobject this, jint data)
{
    awt_gtk_threadsEnter();
    gtk_selection_owner_set((GtkWidget *)data, GDK_SELECTION_CLIPBOARD, GDK_CURRENT_TIME);
    awt_gtk_threadsLeave();
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GtkClipboard_getNativeClipboard(JNIEnv *env, jobject this, jint data)
{
    awt_gtk_threadsEnter();
    gtk_selection_convert((GtkWidget *)data, GDK_SELECTION_CLIPBOARD, GDK_SELECTION_TYPE_STRING, GDK_CURRENT_TIME);
    awt_gtk_threadsLeave();
}




