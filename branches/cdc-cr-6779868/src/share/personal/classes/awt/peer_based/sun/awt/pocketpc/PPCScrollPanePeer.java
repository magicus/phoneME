/*
 * @(#)PPCScrollPanePeer.java	1.10 06/10/10 
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
import java.awt.event.AdjustmentEvent;
import sun.awt.peer.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class PPCScrollPanePeer extends PPCPanelPeer implements ScrollPanePeer {
    private static native void initIDs();
    static {
        initIDs();
    }

    int scrollbarWidth;
    int scrollbarHeight;
    boolean ignore;

    native void create(PPCComponentPeer parent);	    
    native int getOffset(int orient);

    PPCScrollPanePeer(Component target) {
	super(target);
        scrollbarWidth = getVScrollbarWidthNative();
        scrollbarHeight = getHScrollbarHeightNative();
	ignore = false;
    }

    void initialize() {
        super.initialize();
        setInsets();
    }

    public void setUnitIncrement(Adjustable adj, int p) {
        // The unitIncrement is grabbed from the target as needed.
    }

    public Insets insets() {
        return getInsets();
    }
    private native void setInsets();

    public native synchronized void setScrollPosition(int x, int y);

    public int getHScrollbarHeight() {
        return scrollbarHeight;
    }
    private native int getHScrollbarHeightNative();

    public int getVScrollbarWidth() {
        return scrollbarWidth;
    }
    private native int getVScrollbarWidthNative();

    public Point getScrollOffset() {
	int x, y;
	x = getOffset(Scrollbar.HORIZONTAL);
	y = getOffset(Scrollbar.VERTICAL);
	return new Point(x, y);
    }

    /**
     * The child component has been resized.  The scrollbars must be
     * updated with the new sizes.  At the native level the sizes of
     * the actual windows may not have changed yet, so the size
     * information from the java-level is passed down and used.
     */
    public void childResized(int width, int height) {
	ScrollPane sp = (ScrollPane)target;
	Dimension vs = sp.getSize();
	setSpans(vs.width, vs.height, width, height);
        setInsets();
    }

    native synchronized void setSpans(int viewWidth, int viewHeight, 
                                      int childWidth, int childHeight);

    /**
     * Called by ScrollPane's internal observer of the scrollpane's adjustables.
     * This is called whenever a scroll position is changed in one
     * of adjustables, whether it was modified externally or from the
     * native scrollbars themselves.  If the change was sourced by the
     * native scrollbars then ignore will be set to true.
     */
    public int setValue(Adjustable adj, int v) {
	if (! ignore) {
	    Component c = getScrollChild();
	    Point p = c.getLocation();
	    switch(adj.getOrientation()) {
	    case Adjustable.VERTICAL:
		setScrollPosition(-(p.x), v);
		break;
	    case Adjustable.HORIZONTAL:
		setScrollPosition(v, -(p.y));
		break;
	    }
	    return 0;
	}
	return -1;
    }
	    
    private native Component getScrollChild();

    /**
     * Callback from windows to indicate that the vertical scrollbar was
     * adjusted and a new value is desired.  If this is a valid
     * value the adjustable will change and setValue will be called.
     */
    void scrolledVertical(int value) {
	ScrollPane sp = (ScrollPane)target;
        Adjustable adj = sp.getVAdjustable();
        if (adj != null) {
	    Adjustor e = new Adjustor(adj, value, this);
            PPCToolkit.postEvent(e);
        }
    }

    /**
     * Callback from windows to indicate that the horizontal scrollbar was
     * adjusted and a new value is desired.  If this is a valid
     * value the adjustable will change and setValue will be called.
     */
    void scrolledHorizontal(int value) {
	ScrollPane sp = (ScrollPane)target;
        Adjustable adj = sp.getHAdjustable();
        if (adj != null) {
	    Adjustor e = new Adjustor(adj, value, this);
            PPCToolkit.postEvent(e);
        }
    }

    /**
     * This is used to change the adjustable on another thread 
     * to represent a change made in the native scrollbar.  Since
     * the change was reflected immediately at the native level,
     * notification from the adjustable is temporarily ignored.
     */
    class Adjustor extends AWTEvent implements java.awt.ActiveEvent {

	Adjustor(Adjustable adj, int value, PPCScrollPanePeer peer) {
	    super(adj, 0);
	    this.adj = adj;
	    this.value = value;
	    this.peer = peer;
	}

        public void dispatch() {
	    peer.ignore = true;
	    adj.setValue(value);
	    peer.ignore = false;
	}

	int value;
	Adjustable adj;
	PPCScrollPanePeer peer;
    }
}
