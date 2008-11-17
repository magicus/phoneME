/*
 * @(#)Panel.java	1.32 06/10/10
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

import sun.awt.peer.PanelPeer;
import sun.awt.PeerBasedToolkit;

/**
 * <code>Panel</code> is the simplest container class. A panel
 * provides space in which an application can attach any other
 * component, including other panels.
 * <p>
 * The default layout manager for a panel is the
 * <code>FlowLayout</code> layout manager.
 *
 * @version 	1.28, 08/19/02
 * @author 	Sami Shaio
 * @see     java.awt.FlowLayout
 * @since   JDK1.0
 */
public class Panel extends Container {
    final static LayoutManager panelLayout = new FlowLayout();
    private static final String base = "panel";
    private static int nameCounter = 0;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -2728009084054400034L;
    /**
     * Creates a new panel using the default layout manager.
     * The default layout manager for all panels is the
     * <code>FlowLayout</code> class.
     * @since       JDK1.0
     */
    public Panel() {
        this(panelLayout);
    }

    /**
     * Creates a new panel with the specified layout manager.
     * @param layout the layout manager for this panel.
     * @since JDK1.1
     */
    public Panel(LayoutManager layout) {
        setLayout(layout);
    }

    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }

    /**
     * Creates the Panel's peer.  The peer allows you to modify the
     * appearance of the panel without changing its functionality.
     */

    public void addNotify() {
        synchronized (getTreeLock()) {
            if (peer == null)
                peer = ((PeerBasedToolkit) getToolkit()).createPanel(this);
            super.addNotify();
        }
    }
}
