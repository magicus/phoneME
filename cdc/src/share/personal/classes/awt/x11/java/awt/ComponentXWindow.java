/*
 * @(#)ComponentXWindow.java	1.19 06/10/10
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

import java.awt.event.*;

/** The X window used by a component to get mouse events from the X server. This winow represents an InputOnly
 window in X (an invisible window that can be used to select and receive events).
 @version 1.15, 08/19/02
 @author Nicholas Allen*/

class ComponentXWindow {
    static {
        // Make sure the toolkit has been loaded. This is necessary to ensure the
        // libx11awt is loaded and initialized.
		
        Toolkit.getDefaultToolkit();
    }
    ComponentXWindow (Component component) {
        this.component = component;
        create();
        setBackground(component.getBackground());
        setCursor(component.cursor);
    }
	
    /** Sets the background for this x window. As this is an InputOnly X window we don't do anything
     but sub classes need to override this if the X window is an InputOutput X window. */
	
    void setBackground(Color c) {}
	
    /** Creates the component's X window. */
	
    void create() {
        isDelegateWindow = component.delegateSource != null || component.parent.xwindow.isDelegateWindow;
        // If this is the top level delegate's x window (eg a JButton's) then create it as a child
        // of the delegate source's (ie the Button's) x window
		
        this.windowID = create((component.delegateSource != null) ? component.delegateSource.xwindow.windowID : component.parent.xwindow.windowID);
    }
	
    /** Creates the window given its parent. The window should be initially unmapped and the
     size can be anything as this is set later by the setBounds method. */
	
    private native int create(int parent);
	
    /** Destroys the X window. */
	
    native void dispose();
	
    /** Gets the location of the X window on the screen. */
	
    native Point getLocationOnScreen();

    void setCursor(Cursor cursor) {
        setCursorNative(cursor != null ? cursor.type : Cursor.DEFAULT_CURSOR);
    }

    native void setCursorNative(int type);
	
    void setBounds(int x, int y, int width, int height) {
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
                ComponentXWindow parent = component.parent.xwindow;
                if (parent instanceof WindowXWindow) {
                    x -= ((WindowXWindow) parent).leftBorder;
                    y -= ((WindowXWindow) parent).topBorder;
                }
            }
            setBoundsNative(x, y, width, height);
            zeroSize = false;
            // Because the X window could have been unmapped by setting the size to 0
            // we remap it again if the component is showing.
			
            if (component.visible)
                map();
        }
    }
	
    /** Sets the bounds for this X window using XMoveResizeWindow xlib function. Note that
     this function should not be called with a width or height of zero as X will generate errors. */
	
    native void setBoundsNative(int x, int y, int width, int height);
	
    /** Sets the event mask for the X window using the supplied event mask (see AWTEvent).
     This lets the X server know what kinds of events we are interested in. */
	
    native void setMouseEventMask(long mask);
	
    /** Maps the X window. This will only map the window if the size is non zero for
     both the width and height. */
	
    void map() {
        if (!zeroSize)
            mapNative();
    }
	
    /** Maps the X window using XMapWindow xlib function. */
	
    native void mapNative();
	
    /** Unmaps the X window using the XUnmapWindow xlib function. */
	
    native void unmap();
	
    /** Called when a mouse button event is received from the X server for this X window.
     Posts required AWTEvents into the Java event queue for processing. This method will
     determine the click count. */
	
    void postMouseButtonEvent(int id, long when, int modifiers, int x, int y) {
        if (id == MouseEvent.MOUSE_PRESSED) {
            if (x == lastClickX && y == lastClickY && when - lastClickTime <= 200)
                clickCount++;
            else clickCount = 1;
            lastClickX = x;
            lastClickY = y;
            lastClickModifiers = modifiers;
        }
        boolean popupTrigger = (id == MouseEvent.MOUSE_PRESSED && (modifiers & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK);
        postMouseEvent(new MouseEvent (component, id, when, modifiers, x, y, clickCount, popupTrigger));
        if (id == MouseEvent.MOUSE_RELEASED &&
            lastClickX == x &&
            lastClickY == y &&
            lastClickModifiers == modifiers) {
            postMouseEvent(new MouseEvent (component, MouseEvent.MOUSE_CLICKED, when, modifiers, x, y, clickCount, popupTrigger));
            lastClickTime = when;
        }
    }
	
    /** Called when a mouse motion event is received from the X server for this X window.
     Posts required AWTEvents into the Java event queue for processing. */
	
    void postMouseEvent(int id, long when, int modifiers, int x, int y) {
        postMouseEvent(new MouseEvent (component, id, when, modifiers, x, y, 0, false));
    }
	
    private void postMouseEvent(MouseEvent e) {
        // Check to see if the mouse event should be propogated to the delegate source if this
        // is a delegate x window. For example, as the events will come in directly to a JButton
        // through its X window it is necessary to send the event to the Button as well.
		
        if (isDelegateWindow) {
            int x = e.getX(), y = e.getY();
            MouseEvent e2;
            Component c = component;
            // Find delegate source and translate to its coordinates
			
            while (c.delegateSource == null) {
                x += c.x;
                y += c.y;
                c = c.parent;
            }
            Toolkit.getEventQueue().postEvent(new MouseEvent (c.delegateSource, e.id, e.getWhen(), e.getModifiers(), x, y, e.getClickCount(), e.isPopupTrigger()));
        }
        Toolkit.getEventQueue().postEvent(e);
    }
    /** The component this X window is for. */
	
    Component component;
    /** The native X Window id. */
	
    int windowID;
    /** true if size is set to zero in either width or height. This is necessary
     as X doesn't allow 0 size windows. */
	
    boolean zeroSize = true;
    /** The time the last mouse click event ocurred at. */
	
    private long lastClickTime;
    /** The click count used for mouse events. */
	
    private int clickCount = 1;
    /** Position of last mouse click. */
	
    private int lastClickX, lastClickY;
    /** Modifiers used the last time the mouse was clicked. */
	
    private int lastClickModifiers;
    /** true if this x window is an x window for a delegate component (eg a JButton) or one of
     it's child windows. This is used to determine if we need to propogate mouse events to
     the delegate source (eg the Button). */
	
    private boolean isDelegateWindow;
}
