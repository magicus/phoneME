/*
 * @(#)GMenuItemPeer.java	1.13 06/10/10
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
import java.awt.event.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class GMenuItemPeer implements MenuItemPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new GMenuItemPeer. */

    GMenuItemPeer (MenuItem target) {
        this.target = target;
        create();
        try {
            GMenuContainer container = (GMenuContainer) GToolkit.getMenuComponentPeer((MenuComponent) target.getParent());
            menuBar = container.getMenuBar();
            /* Add the item to its container. */
		
            container.add(this);
        } catch (ClassCastException e) {}
        setLabel(target.getLabel());
        setEnabled(target.isEnabled());
        setFont(target.getFont());
    }
	
    protected void create() {
        boolean isSeparator = target.getLabel().equals("-");
        create(isSeparator);
        if (!isSeparator) {
            setLabel(target.getLabel());
            setFont(target.getFont());
        }
    }
	
    protected native void create(boolean isSeparator);
	
    public native void dispose();
	
    public void setLabel(String label) {
        if (label != null) {
            byte[] encString = label.getBytes();
            byte[] encStringNul = new byte[encString.length + 1];
            System.arraycopy(encString, 0, encStringNul, 0, encString.length);
            setLabelNative(encStringNul);
        }
    }

    protected native void setLabelNative(byte[] label);

    public native void setEnabled(boolean b);

    public native void setFont(Font f);
    
    public GMenuBarPeer getMenuBar() {
        return menuBar;
    }
	
    private void postActionEvent() {
        GToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED,
                target.getActionCommand()));
    }
    private int data;
    protected MenuItem target;
    /** The menu bar that this menu item is attatched to if it is attatcyed to a menu bar.
     This is needed for the accelerator group that is used to implement shortcuts in Gtk.
     The menu bar holds the accelerator group. */
	
    private GMenuBarPeer menuBar;
}
