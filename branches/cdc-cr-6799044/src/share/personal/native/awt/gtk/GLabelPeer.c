/*
 * @(#)GLabelPeer.c	1.14 06/10/10
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
#include "java_awt_Label.h"
#include "sun_awt_gtk_GLabelPeer.h"
#include "GLabelPeer.h"

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GLabelPeer_create (JNIEnv *env, jobject this)
{
	GLabelPeerData *data = (GLabelPeerData *)calloc (1, sizeof (GLabelPeerData));
	GtkWidget *drawingArea;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	/* Labels are currently drawn in Java as GtkLabel doesn't have support for horizontal positioning of text
	   as Java does.  */
	
	drawingArea = gtk_drawing_area_new();
	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, drawingArea, drawingArea, NULL, JNI_FALSE);
	
	awt_gtk_threadsLeave();
}
