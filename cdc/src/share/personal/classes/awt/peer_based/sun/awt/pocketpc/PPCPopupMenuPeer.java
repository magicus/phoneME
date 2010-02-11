/*
 * @(#)PPCPopupMenuPeer.java	1.9 06/10/10
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
 */

package sun.awt.pocketpc;

import java.awt.*;
import sun.awt.peer.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class PPCPopupMenuPeer extends PPCMenuPeer implements PopupMenuPeer {

  public PPCPopupMenuPeer(PopupMenu target) {
    this.target = target;

    if (target.getParent() instanceof Component) {
      Component parent = (Component)target.getParent();	    
      ComponentPeer peer = PPCToolkit.getComponentPeer(parent);
      PPCComponentPeer parentPeer = null;
      if (peer instanceof PPCComponentPeer) {
         parentPeer = (PPCComponentPeer)peer;
      } else {
        // because the menu isn't a component (sigh) we first have to wait
	// for a failure to map the peer which should only happen for a 
	// lightweight container, then find the actual native parent from
	// that component.
	parent = PPCToolkit.getNativeContainer(parent);
	parentPeer = (PPCComponentPeer) PPCToolkit.getComponentPeer(parent);
      }
      createMenu(parentPeer);
    } else {
	throw new IllegalArgumentException(
                  "illegal popup menu container class");
      }
    }

    native void createMenu(PPCComponentPeer parent);

    public void show (Component origin, int x, int y) {

      Event e = new Event(origin, 0, 0, x, y, 0, 0);

      //PPCComponentPeer peer = (PPCComponentPeer) PPCToolkit.getComponentPeer(origin);
      if (! (PPCToolkit.getComponentPeer(origin) instanceof PPCComponentPeer)) {
	// A failure to map the peer should only happen for a 
	// lightweight component, then find the actual native parent from
	// that component.  The event coorinates are going to have to be
	// remapped as well.
	    Component nativeOrigin = PPCToolkit.getNativeContainer(origin);
	    e.target = nativeOrigin;

	// remove the event coordinates 
	    for (Component c = origin; c != nativeOrigin; c = c.getParent()) {
	      Point p = c.getLocation();
	      e.x += p.x;
	      e.y += p.y;
	     }
      }

      showPopup(e);
    }

    public native void showPopup(Event e); 


}
