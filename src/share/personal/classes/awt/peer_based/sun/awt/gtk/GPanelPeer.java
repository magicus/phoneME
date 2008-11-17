/*
 * @(#)GPanelPeer.java	1.15 06/10/10
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

package sun.awt.gtk;

import sun.awt.peer.*;
import java.awt.*;

/**
 * GContainerPeer.java
 *
 * @author Nicholas Allen
 */

class GPanelPeer extends GContainerPeer implements PanelPeer {
    private int numChildren = 0;
    GPanelPeer (GToolkit toolkit, Container target) {
        super(toolkit, target);
    }
	
    protected native void create();

    final void add(GComponentPeer peer) {
        // If the component whose peer is being created is a direct child of this container
        // (ie it is not a heavyweight inside a lightweight) then we need to determine the
        // position to insert at.
		
        if (peer.target.getParent() == target) {
            // Determine the Z order position of the component being added.
			
            Container container = (Container) target;
            int numComponents = container.getComponentCount();
            for (int i = 0; i < numComponents; i++) {
                Component component = container.getComponent(i);
                if (component == peer.target) {
                    insert(peer, numChildren - i);
                    numChildren++;
                    return;
                }
            }
        } // The peer is not a direct child so it must be in a lightweight hierachy. We add the
        // peer to the end of the list.
        else {
            insert(peer, -1);
        }
    }
	
    final void remove(GComponentPeer peer) {
        if (peer.target.getParent() == target) {
			delete(peer);
            numChildren--;
        }
    }
	
    /** Inserts the component's peer into this container at the specified index. If the index is
     negative then the component will be added at the end of the list. Gtk does not have support
     for inserting widgets at an arditrary index so we have had to write a custom widget to
     support this. */
	
    private native void insert(GComponentPeer peer, int index);
	
    private native void delete(GComponentPeer peer);
	
    protected boolean canHavePixmapBackground() {
        return false;
    }
}
