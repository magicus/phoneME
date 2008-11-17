/*
 * @(#)HeavyweightComponentXWindow.java	1.15 06/10/10
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

import java.awt.event.PaintEvent;
import java.awt.event.ComponentListener;
import java.awt.event.ComponentEvent;
import java.awt.event.ContainerListener;
import java.awt.event.ContainerEvent;
import java.util.ArrayList;
import java.util.Iterator;

/** The X window used by a component, that is traditionally heavyweight, to get mouse events from the X server.
 This winow represents an InputOutput
 window in X (an opaque window that can be used to select and receive events). This is required for maximum
 compatiability with applications/applets that assume AWT components are heavyweight. Heavyweight components
 generate their own paint events and also appear in front of lightweights. Some programs may require these
 assumptions in order to work properly so by providing a heavyweight window we can maintain maximum
 compatiability.
 @version 1.1, 10/22/01
 @author Nicholas Allen*/

class HeavyweightComponentXWindow extends ComponentXWindow implements ComponentListener, ContainerListener {
    HeavyweightComponentXWindow(Component component) {
        super(component);
    }
	
    void create() {
        // Find parent whose X window is also a heavyweight X window. This is necessary because
        // X does not allow an InputOutput window to be a child of an InputOnly window. We need
        // to listen for lightweight parent moves/hides because we are not a child of these but
        // as far as Java is concerned we are.
		
        Container parent = component.parent;
        // If the parent is a scrollpane then we want to create our X window as a child of the
        // viewport and not the scrollpane itself. We must only do this if the component is not
        // the viewport or the scroll bars for the scrollpane as they should be made children of
        // the scrollpane itself and not the viewport.
		
        if (parent instanceof ScrollPane) {
            ScrollPane scrollPane = (ScrollPane) parent;
            if (component != scrollPane.viewport &&
                component != scrollPane.horizScrollbar &&
                component != scrollPane.vertScrollbar) {
                // Create the viewport X window that we will be added to instead of the scrollpane's.
				
                scrollPane.viewport.parent = scrollPane;
                scrollPane.viewport.addNotify();
                parent = scrollPane.viewport;
            }
        } else {
            while (parent.isLightweight()) {
                parent.addComponentListener(this);
                parent.addContainerListener(this);
                parent = parent.parent;
            }
            parent.addContainerListener(this);
        }
        heavyweightParent = parent;
        this.windowID = create(parent.xwindow.windowID);
    }
	
    private native int create(int parent);
	
    void dispose() {
        removeReferences();
        super.dispose();
    }
	
    native void setMouseEventMask(long mask);

    final native void setBackground(Color color);
	
    /** Called by the native code when an area of the X Window is exposed.
     This posts a paint event into Java's event queue for processing by Java
     so that the exposed area is redrawn. */
	
    void postPaintEvent(int x, int y, int width, int height) {
        Toolkit.getEventQueue().postEvent(new PaintEvent(component, PaintEvent.PAINT, new Rectangle(x, y, width, height)));
    }
	
    void setBounds(int x, int y, int width, int height) {
        // Translate into heavyweight parent's coordinates.
		
        Container parent = component.parent;
        while (parent != null) {
            if (parent.xwindow instanceof HeavyweightComponentXWindow) {
                // X does not allow window of width or height of 0 so if the component has these
                // dimensions then we unmap the XWindow.
				
                if (width == 0 || height == 0) {
                    unmap();
                    zeroSize = true;
                } else {
                    // If this is a top level delegate's x window then it is a child of the delegate
                    // source's x window and so we always position it a 0,0.
					
                    if (component.delegateSource != null) {
                        x = 0;
                        y = 0;
                    } else {
                        if (parent.xwindow instanceof WindowXWindow) {
                            x -= ((WindowXWindow) parent.xwindow).leftBorder;
                            y -= ((WindowXWindow) parent.xwindow).topBorder;
                        }
                    }
                    setBoundsNative(x, y, width, height);
                    zeroSize = false;
                    // Because the X window could have been unmapped by setting the size to 0
                    // we remap it again if the component is showing.
					
                    if (component.visible)
                        map();
                }
                return;
            }
            x += parent.x;
            y += parent.y;
            parent = parent.parent;
        }
    }
	
    /**
     * Invoked when the component's size changes.
     */
    public void componentResized(ComponentEvent e) {}

    /**
     * Invoked when the component's position changes.
     */
    public void componentMoved(ComponentEvent e) {
        setBounds(component.x, component.y, component.width, component.height);
    }

    /**
     * Invoked when the component has been made visible.
     */
    public void componentShown(ComponentEvent e) {
        if (component.isShowing())
            map();
    }

    /**
     * Invoked when the component has been made invisible.
     */
    public void componentHidden(ComponentEvent e) {
        unmap();
    }
	
    /**
     * Invoked when a component has been added to the container.
     */
    public void componentAdded(ContainerEvent e) {}

    /**
     * Invoked when a component has been removed from the container.
     */
    public void componentRemoved(ContainerEvent e) {
        Component c = e.getChild();
        if (c == component)
            removeReferences();
        else if (lightweightParents.contains(c))
            removeReferences();
    }
	
    private void removeReferences() {
        Iterator i = lightweightParents.iterator();
        while (i.hasNext()) {
            Container c = (Container) i.next();
            c.removeComponentListener(this);
            c.removeContainerListener(this);
        }
        if (heavyweightParent != null)
            heavyweightParent.removeContainerListener(this);
    }
    private java.util.List lightweightParents = new ArrayList(4);
    private Container heavyweightParent;
}
