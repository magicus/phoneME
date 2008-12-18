/*
 * @(#)GMenuItemPeer.h	1.10 06/10/10
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

#ifndef _GMENU_ITEM_PEER_H_
#define _GMENU_ITEM_PEER_H_

#include "awt.h"

typedef struct
{
	jobject peerGlobalRef;

	/* The GtkMenuItem widget that represents this menu item. */
	
	GtkWidget *menuItemWidget;
	
	/* The GtkLabel widget which is inside this menu item. */
	
	GtkWidget *labelWidget;

	GdkFont *font;
	
	jboolean hasShortcutBeenSet;
	GtkAccelGroup *accelGroup;
	guint accelKey, accelMods;
	
} GMenuItemPeerData;

/* Identifies the type of menu item to be created. This is used by the awt_gtk_GMenuItemPeerData_init
   function. */

enum MenuItemType
{
	MENU_ITEM_TYPE,
	CHECK_MENU_ITEM_TYPE,
	SEPARATOR_TYPE,
	SUB_MENU
};

extern void awt_gtk_GMenuItemPeerData_init (JNIEnv *env, jobject this, GMenuItemPeerData *data, enum MenuItemType type);

#endif
