/*
 * @(#)GFileDialogPeer.c	1.14 06/10/10
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "jni.h"
#include "sun_awt_gtk_GFileDialogPeer.h"
#include "GFileDialogPeer.h"

/* Called when the Ok button is pressed in the file dialog. We need to set the selected file or directory
   on the target FileDialog for this peer. */

static void
okClicked (GtkButton *button, GComponentPeerData *data)
{
	jobject this = data->peerGlobalRef;
	jobject target;
	JNIEnv *env;
	gchar *gfile;
	jstring dir = NULL;
	jstring file = NULL;
	struct stat fileStats;
	jboolean isDirectory = JNI_FALSE;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
	
	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
	
	/* Get the selected file from the dialog and create java Strings for the directory
	   name and the file name. */
	
	gfile = gtk_file_selection_get_filename (GTK_FILE_SELECTION(data->widget));
		
	/* Check if the selected file represents a file or a directory. */
	
	if (stat (gfile, &fileStats) == 0)
		isDirectory = (fileStats.st_mode & S_IFDIR) != 0;
		
	isDirectory = (gfile[strlen(gfile) - 1] == '/');
	
	if (isDirectory)
	{
		dir = (*env)->NewStringUTF (env, gfile);
		
		if (dir == NULL)
			(*env)->ExceptionDescribe (env);
		
		file = NULL;
	}
		
	else
	{
		/* Extract file and directory components. */
		
		const gchar *fileName = strrchr (gfile, '/');
		gchar *dirName = ".";
		jboolean freeDirName = JNI_FALSE;
		
		if (fileName == NULL)
			fileName = gfile;

		else
		{
			int dirLength;
			
			fileName++;
			dirLength = fileName - gfile;
			dirName = malloc (dirLength + 1);
			strncpy (dirName, gfile, dirLength);
			dirName[dirLength] = '\0';
			freeDirName = JNI_TRUE;
		}
		
		file = (*env)->NewStringUTF (env, fileName);
		
		if (file == NULL)
			(*env)->ExceptionDescribe (env);
			
		dir = (*env)->NewStringUTF (env, dirName);
		
		if (dir == NULL)
			(*env)->ExceptionDescribe (env);
			
		if (freeDirName)
			free (dirName);
	}
	
	(*env)->SetObjectField (env, target, GCachedIDs.java_awt_FileDialog_fileFID, file);
	(*env)->SetObjectField (env, target, GCachedIDs.java_awt_FileDialog_dirFID, dir);
	
	/* Now hide the dialog. */
	
	(*env)->CallVoidMethod (env, target, GCachedIDs.java_awt_Component_hideMID);
	
	awt_gtk_callbackLeave();
}

static void
cancelClicked (GtkButton *button, GComponentPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	jobject target;
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
	
	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
	
	(*env)->SetObjectField (env, target, GCachedIDs.java_awt_FileDialog_fileFID, NULL);
	(*env)->SetObjectField (env, target, GCachedIDs.java_awt_FileDialog_dirFID, NULL);
	
	/* Now hide the dialog. */
	
	(*env)->CallVoidMethod (env, target, GCachedIDs.java_awt_Component_hideMID);
	
	awt_gtk_callbackLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFileDialogPeer_initIDs (JNIEnv *env, jclass cls)
{
	cls = (*env)->FindClass (env, "java/awt/FileDialog");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_FileDialog_fileFID, "file", "Ljava/lang/String;");
	GET_FIELD_ID (java_awt_FileDialog_dirFID, "dir", "Ljava/lang/String;");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFileDialogPeer_create (JNIEnv *env, jobject this)
{
	GFileDialogPeerData *data = calloc (1, sizeof (GFileDialogPeerData));
	GtkWidget *fileDialog;
	GtkFileSelection *fileSelection;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	fileDialog = gtk_file_selection_new ("");
	
	/* Connect signals to listen to the ok and cancel buttons. */
	
	fileSelection = GTK_FILE_SELECTION (fileDialog);
	gtk_signal_connect (GTK_OBJECT(fileSelection->ok_button), "clicked", GTK_SIGNAL_FUNC (okClicked), data);
	gtk_signal_connect (GTK_OBJECT(fileSelection->cancel_button), "clicked", GTK_SIGNAL_FUNC (cancelClicked), data);
	
	/* Initialize the Dialog peer's data. We pass JNI_FALSE here as we are unable to add components to
	   the file dialog. */
	
	awt_gtk_GDialogPeerData_init (env, this, (GDialogPeerData *)data, fileDialog, JNI_FALSE);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFileDialogPeer_setFileNative (JNIEnv *env, jobject this, jbyteArray file)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	const gchar *gfile = NULL;
	
	if (data == NULL)
		return;
		
	if (file != NULL)
		gfile = (const gchar *)(*env)->GetByteArrayElements (env, file, NULL);
			
	awt_gtk_threadsEnter();
	gtk_file_selection_set_filename (GTK_FILE_SELECTION(data->widget), gfile);
	awt_gtk_threadsLeave();
	
	if (file != NULL)
		(*env)->ReleaseByteArrayElements (env, file, (jbyte *)gfile, JNI_ABORT);
}



