/*
 * @(#)PopupMenu.java	1.29 06/10/10
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

package java.awt;

import sun.awt.peer.PopupMenuPeer;
import sun.awt.PeerBasedToolkit;

/**
 * A class that implements a menu which can be dynamically popped up
 * at a specified position within a component.<p>
 * As the inheritance hierarchy implies, a PopupMenu can be used anywhere
 * a Menu can be used. However, if you use a PopupMenu like a Menu (e.g.,
 * you add it to a MenuBar), then you <b>cannot</b> call <code>show</code>
 * on that PopupMenu.
 *
 * @version	1.25 08/19/02
 * @author 	Amy Fowler
 */
public class PopupMenu extends Menu {
    private static final String base = "popup";
    static int nameCounter = 0;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -4620452533522760060L;
    /**
     * Creates a new popup menu.
     */
    public PopupMenu() {
        this("");
    }

    /**
     * Creates a new popup menu with the specified name.
     *
     * @param label a non-null string specifying the popup menu's label 
     */
    public PopupMenu(String label) {
        super(label);
    }

    /**
     * Construct a name for this MenuComponent.  Called by getName() when
     * the name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }

    /**
     * Creates the popup menu's peer.  The peer allows us to change the
     * appearance of the popup menu without changing any of the popup menu's
     * functionality.
     */
    public void addNotify() {
        synchronized (getTreeLock()) {
            if (peer == null) {
                peer = ((PeerBasedToolkit) Toolkit.getDefaultToolkit()).createPopupMenu(this);
            }
            int nitems = getItemCount();
            for (int i = 0; i < nitems; i++) {
                MenuItem mi = getItem(i);
                mi.parent = this;
                mi.addNotify();
            }
        }
    }

    /**
     * Shows the popup menu at the x, y position relative to an origin component.
     * The origin component must be contained within the component
     * hierarchy of the popup menu's parent.  Both the origin and the parent 
     * must be showing on the screen for this method to be valid.<p>
     * If this PopupMenu is being used as a Menu (i.e., it has a non-Component
     * parent), then you cannot call this method on the PopupMenu.
     * 
     * @param origin the component which defines the coordinate space
     * @param x the x coordinate position to popup the menu
     * @param y the y coordinate position to popup the menu
     * @exception IllegalArgumentException  if this PopupMenu has a non-
     *            Component parent
     */
    public void show(Component origin, int x, int y) {
        // added for Bug #4698597
        if (!(parent instanceof Component)) {
            throw new IllegalArgumentException(
                    "PopupMenus with non-Component parents cannot be shown");
        }
        Component p = (Component) parent;
        if (p == null) {
            throw new NullPointerException("parent is null");
        }
        if (p != origin &&
            p instanceof Container && !((Container) p).isAncestorOf(origin)) {
            throw new IllegalArgumentException("origin not in parent's hierarchy");
        }
        if (p.peer == null || !p.isShowing()) {
            throw new RuntimeException("parent not showing on screen");
        }
        if (peer == null) {
            addNotify();
        }
        synchronized (getTreeLock()) {
            if (peer != null) {
                ((PopupMenuPeer) peer).show(origin, x, y);
            }
        }
    }
}
