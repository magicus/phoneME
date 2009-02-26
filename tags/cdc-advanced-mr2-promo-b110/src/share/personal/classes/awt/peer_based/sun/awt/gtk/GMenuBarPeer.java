/*
 * @(#)GMenuBarPeer.java	1.12 06/10/10
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
 *
 *
 * @author Nicholas Allen
 */

class GMenuBarPeer implements MenuBarPeer, GMenuContainer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new GMenuBarPeer. */

    public GMenuBarPeer (MenuBar target) {
        this.target = target;
        create();
    }
	
    public GMenuBarPeer getMenuBar() {
        return this;
    }
	
    protected native void create();

    public native void dispose();

    public native void add(GMenuItemPeer item);
	
    /** This method does not need to do anything because the menuitem peer will add itself to a
     GMenuContainer when it is created. */
	
    public void addMenu(Menu m) {}

    public void delMenu(int index) {}

    public void addHelpMenu(Menu m) {}
    
    public void setFont(Font f) {}
    private MenuBar target;
    private int data;
}
