/*
 * @(#)GWindowPeer.java	1.24 06/10/10
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

import sun.awt.peer.WindowPeer;
import java.awt.*;
import java.awt.event.*;

/**
 * GWindowPeer.java
 *
 * @author Nicholas Allen
 */

class GWindowPeer extends GPanelPeer implements WindowPeer {
    private static native void initIDs();

    private static int defaultLeftBorder = 0;
    private static int defaultRightBorder = 0;
    private static int defaultTopBorder = 0;
    private static int defaultBottomBorder = 0;

    static {
        initIDs();
    }

    GWindowPeer (GToolkit toolkit, Window target) {
        super(toolkit, target);

        Window owner = (Window) target.getParent();

        if (owner != null) {
            GWindowPeer ownerPeer = (GWindowPeer) GToolkit.getComponentPeer(owner);

            if (ownerPeer != null)
                setOwner(ownerPeer);
        }

        if (!initializedBorders) {
            initBorders();
            initializedBorders = true;
        }

        insets = calculateInsets();
		currentFocusComponent = target;
    }

    protected native void create();

    public native void toFront();

    public native void toBack();

    public native void setResizable(boolean resizeable);

    /** This is overridden to set the bounds on the window manager's frame. */

    public void setBounds(int x, int y, int w, int h) {
        if (!initializedBorders) {
            initBorders();
            initializedBorders = true;
        }

        setBoundsNative(x, y, Math.max(w - leftBorder - rightBorder, 0), Math.max(h - topBorder - bottomBorder, 0));
    }

    /** Sets the bounds for the X window for this widget. */

    protected native void setBoundsNative(int x, int y, int w, int h);

    public Graphics getGraphics() {
        Graphics g = super.getGraphics();

        if (g != null)
            g.translate(-leftBorder, -topBorder);

        return g;
    }

    /** This is necessary to make sure the window manager's X window is in the correct place
     after showing the window. It's not very elegant but it seems to be the only way to get
     the right size and position of the actual outer frame under Gtk with any window manager. */

    public void show() {
        super.show();
        Rectangle bounds = target.getBounds();
        setBounds(bounds.x, bounds.y, bounds.width, bounds.height);
    }

    public Point getLocationOnScreen() {
        Point point = super.getLocationOnScreen();
        point.x -= leftBorder;
        point.y -= topBorder;
        return point;
    }

    void postPaintEvent(int x, int y, int width, int height) {
        super.postPaintEvent(x + leftBorder, y + topBorder, width, height);
    }

    void postMouseEvent(int id, long when, int modifiers, int x, int y, int clickCount, boolean popupTrigger, int nativeEvent) {
        super.postMouseEvent(id, when, modifiers, x + leftBorder, y + topBorder, clickCount, popupTrigger, nativeEvent);
    }

    /**
     * Called to inform the Frame that its size has changed and it
     * should layout its children.
     */

    private void postResizedEvent() {
        GToolkit.postEvent(new ComponentEvent(target, ComponentEvent.COMPONENT_RESIZED));
    }

    private void postMovedEvent() {
        GToolkit.postEvent(new ComponentEvent(target, ComponentEvent.COMPONENT_MOVED));
    }

	/** Posts the required window event to the queue. */

    protected void postWindowEvent(int type) {

		// Gtk does not post focus events when the window becomes activated/deactivated so
		// it is necessary to do this here.

		if (type == WindowEvent.WINDOW_ACTIVATED) {
			if (!postedActivatedEvent) {
				GToolkit.postEvent(new WindowEvent((Window) target, type));
				GToolkit.postEvent(new FocusEvent (currentFocusComponent, FocusEvent.FOCUS_GAINED));
				postedActivatedEvent = true;
			}
		}

		else if (type == WindowEvent.WINDOW_DEACTIVATED) {
			if (postedActivatedEvent) {
				GToolkit.postEvent(new WindowEvent((Window) target, type));
				GToolkit.postEvent(new FocusEvent (currentFocusComponent, FocusEvent.FOCUS_LOST, true));
				postedActivatedEvent = false;
			}
		}

		else GToolkit.postEvent(new WindowEvent((Window) target, type));
    }

	/** This has been overridden because calling gtk_widget_grab_focus on a window does not take
	    focus away from a component. We have to explicity set the focus component to NULL using
	 gtk_window_set_focus. */

	public native void requestFocus();

	/** Called by the native code when the focus has moved from one component to another
	    inside this window. Note that this may be called even when the window is not activated
	 so we can't always post focus gained events to the current focus component. If no component
	 has the focus in this window then this method is called with this window as the new focus
	 component. */

    private void focusChanged (GComponentPeer newFocusPeer) {

		Component newFocusComponent = newFocusPeer.target;

        if (newFocusComponent != currentFocusComponent) {

			GToolkit.postEvent(new FocusEvent (currentFocusComponent, FocusEvent.FOCUS_LOST));

			if (postedActivatedEvent) {
            	GToolkit.postEvent(new FocusEvent (newFocusComponent, FocusEvent.FOCUS_GAINED));
			}

            currentFocusComponent = newFocusComponent;
        }
    }

    private void updateInsets() {
        insets = calculateInsets();
    }

    /** Calculates the insets using any values appropriate (such as borders). */

    Insets calculateInsets() {
        return new Insets (topBorder, leftBorder, bottomBorder, rightBorder);
    }

    /** Sets up border values to some sensible values for this window. */

    void initBorders() {
        topBorder = defaultTopBorder;
        leftBorder = defaultLeftBorder;
        bottomBorder = defaultBottomBorder;
        rightBorder = defaultRightBorder;
    }

    /** Sets the default border values to be used for future windows. */

    void setDefaultBorders(int top, int left, int bottom, int right) {
        defaultTopBorder = top;
        defaultLeftBorder = left;
        defaultBottomBorder = bottom;
        defaultRightBorder = right;
    }

    private native void setOwner(GWindowPeer owner);

    int getOriginX() {
        return -leftBorder;
    }

    int getOriginY() {
        return -topBorder;
    }

    int topBorder, leftBorder, bottomBorder, rightBorder;

	/** The currently focused component in this window. This will be the window itself if
	 no component inside the window has focus but it is never null. */

    private Component currentFocusComponent;

	/** Determines if a WINDOW_ACTIVATED event has been posted to the queue without a
	    WINDOW_DEACTIVATED. This is necessary because when the focus changes in the window
	    we need to post a focus gained permanent on the new component only if this window has
	 been activated.
	 @see #focusChanged */

	private boolean postedActivatedEvent;
    private boolean initializedBorders;
}
