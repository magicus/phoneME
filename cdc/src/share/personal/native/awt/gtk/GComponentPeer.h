/*
 * @(#)GComponentPeer.h	1.15 06/10/10
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

#ifndef _GCOMPONENT_PEER_H_
#define _GCOMPONENT_PEER_H_

#include "awt.h"

/* Data that is stored in the data field of a GComponentPeer. Sub classes of
   ComponentPeer can "sub class" this data by putting a field of this type as the
   first field in their data structure. Any data structure that needs to be sub classed
   in this way should provide a awt_gtk_<PeerType>Data_init function to initialize the
   data. This allows the "sub classes" to call the super initialization function.
   In effect, this immitates constructors in OO languages. */

typedef struct _GComponentPeerData
{
	/* The main GtkWidget this GComponentPeer manages. This widget is used for
	   mouse signals which are then sent to the EventDispatchThread for
	   processing. */

	GtkWidget *widget;
	
	/* This is the widget that is used for drawing via a GdkGraphics object. */
	
	GtkWidget *drawWidget;
	
	/* A global reference to the peer. This can be used by callbacks. */
	
	jobject peerGlobalRef;
	
	/* The last time the widget was clicked on. This is used for determining the click
	   count for mouse click events. */
	
	guint32 lastClickTime;
	
	/* The number of times the widget was clicked on. */
	
	int clickCount;
	
	/* The mouse cursor currently set on this component. */
	
	GdkCursor *cursor;
	
	/* When a component peer is created we check to see if a foregriund/background/font have
	   be explicitly set for the component. If one hasn't then we set one which resembles the
	   peer's background/foreground/font. This ensures that when the component is asked for
	   any of these attributes Java will return something sensible that closely resembles
	   the native peer's. We need to remember the ones thgat we set so we can remove them again
	   when the peer is destroyed. We also use the installed background to decide if it is possible
	   to use a pixmap to render the background or not (if a background has been set which is not the same
	   as the installed background then we can't have a pixmap background). */
	   
	jobject installedBackground, installedForeground, installedFont;
	
} GComponentPeerData;

/* Initializes the ComponentData. This sets up the global reference to the peer and attaches the data to the peer.
   It also connects signals to the widgets necessary so that the appropriate callbacks are called.
   These signals handle events that are common to all objects that are instances of the java.awt.Component class.
   Returns JNI_TRUE if initialization was successful or JNI_FALSE if an exception has been thrown. */

extern jboolean awt_gtk_GComponentPeerData_init (JNIEnv *env,
												 jobject this,
												 GComponentPeerData *data,
												 GtkWidget *widget,
												 GtkWidget *drawWidget,
												 GtkWidget *styleWidget,
												 jboolean usesBaseForBackground);
extern GComponentPeerData *awt_gtk_getData (JNIEnv *env, jobject peer);
extern GComponentPeerData *awt_gtk_getDataFromWidget (GtkWidget *widget);

#endif
