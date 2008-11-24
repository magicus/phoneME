/*
 * @(#)GScrollPanePeer.h	1.12 06/10/10
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

#ifndef _GSCROLL_PANE_PEER_H_
#define _GSCROLL_PANE_PEER_H_

#include "GComponentPeer.h"

typedef struct
{
	GComponentPeerData componentPeerData;
	
	/** The GtkScrolledWindow widget. This is needed because we have to put the
	    scrolled window indside a GtkEventBox in order to enable all the events on
	    it. Thus we can't use GComponentPeerData->widget as this will be an event box
	    and not the scrolled window. */
	    
	GtkWidget *scrolledWindowWidget;
	
	GtkWidget *viewportWidget;
	
}  GScrollPanePeerData;


/** This is not really peer data, but a structure to hold the connection between the
    scroll pane and its associated adjusters (scroll bars).
    The scroll pane is a two part widget. When an event occurs on a scroll pane this
    needs to be relayed to the peer. */


typedef struct
{
        GScrollPanePeerData scrollPaneData;

        jobject adjustable;

} GAdjustablePeerData;

#endif
