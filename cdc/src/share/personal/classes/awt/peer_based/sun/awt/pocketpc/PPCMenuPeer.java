/*
 * @(#)PPCMenuPeer.java	1.8 06/10/10
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

class PPCMenuPeer extends PPCMenuItemPeer implements MenuPeer {

  // MenuPeer implementation

    public native void addSeparator();
    public void addItem(MenuItem item) {
	PPCMenuItemPeer itemPeer = (PPCMenuItemPeer) PPCToolkit.getMenuComponentPeer((MenuComponent)item);
    }
    public native void delItem(int index);

    // Toolkit & peer internals

    PPCMenuPeer() {}   // used by subclasses.

	//Netscape: Menus require a 2 step construction in the event that
	//the menu is displayed before any items are added to it.

    PPCMenuPeer(Menu target) {
	this.target = target;
    }

    void create() {
      MenuContainer parent = ((Menu) target).getParent();

      if (parent instanceof MenuBar) {
	PPCMenuBarPeer mbPeer = (PPCMenuBarPeer) PPCToolkit.getMenuComponentPeer((MenuComponent)parent);
	createMenu(mbPeer);
      }
      else if (parent instanceof Menu) {
	PPCMenuPeer mPeer = (PPCMenuPeer) PPCToolkit.getMenuComponentPeer((MenuComponent)parent);
	createSubMenu(mPeer);
      }
      else {
	throw new IllegalArgumentException("unknown menu container class");
      }
    }

    native void createMenu(PPCMenuBarPeer parent);
    native void createSubMenu(PPCMenuPeer parent);
}
