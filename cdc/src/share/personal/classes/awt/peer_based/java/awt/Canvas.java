/*
 * @(#)Canvas.java	1.12 06/10/10
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

import sun.awt.peer.CanvasPeer;
import sun.awt.PeerBasedToolkit;

/**
 * A <code>Canvas</code> component represents a blank rectangular
 * area of the screen onto which the application can draw or from
 * which the application can trap input events from the user.
 * <p>
 * An application must subclass the <code>Canvas</code> class in
 * order to get useful functionality such as creating a custom
 * component. The <code>paint</code> method must be overridden
 * in order to perform custom graphics on the canvas.
 *
 * @version 	1.19 08/10/01
 * @author 	Sami Shaio
 * @since       JDK1.0
 */
public class Canvas extends Component {
    private static final String base = "canvas";
    private static int nameCounter = 0;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -2284879212465893870L;
    /**
     * Constructs a new Canvas.
     */
    public Canvas() {}

    /*
     * 6222500.
     * TCK: Component.getGraphicsConfiguration() doesn't return the correct GraphicsConfiguration
     */
    public Canvas(GraphicsConfiguration config) {
        this();
        graphicsConfig = config;
    }

    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }

    /**
     * Creates the peer of the canvas.  This peer allows you to change the
     * user interface of the canvas without changing its functionality.
     * @see     java.awt.Toolkit#createCanvas(java.awt.Canvas)
     * @see     java.awt.Component#getToolkit()
     * @since   JDK1.0
     */
    public void addNotify() {
        synchronized (getTreeLock()) {
            if (peer == null)
                peer = ((PeerBasedToolkit) getToolkit()).createCanvas(this);
            super.addNotify();
        }
    }

    /**
     * This method is called to repaint this canvas. Most applications
     * that subclass <code>Canvas</code> should override this method in
     * order to perform some useful operation.
     * <p>
     * The <code>paint</code> method provided by <code>Canvas</code>
     * redraws this canvas's rectangle in the background color.
     * <p>
     * The graphics context's origin (0,&nbsp;0) is the top-left corner
     * of this canvas. Its clipping region is the area of the context.
     * @param      g   the graphics context.
     * @see        java.awt.Graphics
     * @since      JDK1.0
     */
    public void paint(Graphics g) {
        g.setColor(getBackground());
        g.fillRect(0, 0, width, height);
    }

    boolean postsOldMouseEvents() {
        return true;
    }
}
