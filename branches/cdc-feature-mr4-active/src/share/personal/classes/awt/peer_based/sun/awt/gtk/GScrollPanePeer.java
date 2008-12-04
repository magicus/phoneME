/*
 * @(#)GScrollPanePeer.java	1.20 06/10/10
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

import java.awt.*;
import java.awt.event.AdjustmentEvent;
import sun.awt.peer.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class GScrollPanePeer extends GContainerPeer implements ScrollPanePeer {
    private static native void initIDs();
    private static int hScrollbarHeight = calculateHScrollbarHeight();
    private static int vScrollbarWidth = calculateVScrollbarWidth();
    private static native int calculateHScrollbarHeight();

    private static native int calculateVScrollbarWidth();
    private native void enableScrollbarsNative(boolean hBarOn, boolean vBarOn);

    private boolean ignoreSetValue;
    private int scrollbarDisplayPolicy;
    // added for bug #'s 4714727, 4687092, 4689327, 4699570
    private boolean ignoreDispatchH = false, ignoreDispatchV = false;
    static {
        initIDs();
    }
    /** Creates a new GScrollPanePeer. */

    GScrollPanePeer (GToolkit toolkit, ScrollPane target) {
        super(toolkit, target);
        connectAdjuster(target.getHAdjustable(), Adjustable.HORIZONTAL);
        connectAdjuster(target.getVAdjustable(), Adjustable.VERTICAL);
        scrollbarDisplayPolicy = target.getScrollbarDisplayPolicy();
    }

    protected native void create();

    native void add(GComponentPeer peer);
	
    void remove(GComponentPeer peer) {}
	
    /** Overidden to calculate insets. This includes any optional scrollbars so
     we need to dynamically work this out. */
	
    public Insets getInsets() {
        // adjust predicted view size to account for scrollbars
        
        Insets inset = new Insets(0, 0, 0, 0);
        if (scrollbarVisible(Adjustable.VERTICAL))
            inset.right = vScrollbarWidth;
        if (scrollbarVisible(Adjustable.HORIZONTAL))
            inset.bottom = hScrollbarHeight;
        return inset;
    }
	
    public int getHScrollbarHeight() {
        return hScrollbarHeight;
    }

    public int getVScrollbarWidth() {
        return vScrollbarWidth;
    }

    public void childResized(int w, int h) {
	boolean hBarOn=false;
	boolean vBarOn=false;
	int sw, sh;
	int hAdj=0;
	int wAdj=0;

	sw = target.getWidth();
	sh = target.getHeight();

	/* GTK peer doesn't do scrollbars right */
	/* so we enable and disable the scrollbars explictly */
	if (scrollbarDisplayPolicy == ScrollPane.SCROLLBARS_AS_NEEDED) {
	    if (w > sw) {
		hBarOn = true;
		hAdj = hScrollbarHeight;
	    }
	    if (h > sh - hAdj) {
		vBarOn = true;
		wAdj = vScrollbarWidth;
		/* revisit need for horiz. scrollbar */
		if ((!hBarOn) && (w > sw - wAdj)) {
		    hBarOn = true;
		}

	    }
	    enableScrollbarsNative(hBarOn, vBarOn);
	}
    }

    public void setUnitIncrement(Adjustable adj, int u) {
        setUnitIncrementNative(adj.getOrientation(), u);
    }

    private native void setUnitIncrementNative(int orientation, int increment);

    public void setValue(Adjustable adj, int v) {
        if (ignoreSetValue) return;
        int orientation = adj.getOrientation();
        int max = adj.getMaximum();
        int pageSize = adj.getVisibleAmount();
        // if-block added for bug #'s 4714727, 4687092, 4689327, 4699570
        /*
         For some reason, when the scroll position is set beyond the
         maximum value, gtk generates a value-changed signal
         that sends back a value that's one pixel less than the
         maximum value.  This is one reason that the comparison
         between get and set sometimes fail.  This modification
         attempts to correct the bug by anticipating and inhibiting
         the value-changed signal from modifying the position value.
         If gtk does not consistently exhibit this behavior, there
         will be a problem with missed scroll position value updates.
         */
        if (v >= (max - pageSize)) {
            if (orientation == Adjustable.HORIZONTAL)
                ignoreDispatchH = true;
            else
                ignoreDispatchV = true;
        }
        setAdjusterNative(orientation, v, max, pageSize);
    }
    class GScrollPaneAdjustableEvent extends AWTEvent implements ActiveEvent {
        GScrollPanePeer peer;
        Adjustable adjuster;
        int value;
        GScrollPaneAdjustableEvent(GScrollPanePeer src, Adjustable adj, int val) {
            super(src, 0);
            peer = src;
            adjuster = adj;
            value = val;
        }

        public void dispatch() {
            if (adjuster.getOrientation() == Adjustable.HORIZONTAL) {
                if (ignoreDispatchH) {
                    ignoreDispatchH = false;
                    return;
                }
            } else {
                if (ignoreDispatchV) {
                    ignoreDispatchV = false;
                    return;
                }
            }
            peer.ignoreSetValue = true;
            adjuster.setValue(value);
            peer.ignoreSetValue = false;
        }
    }
    private void postAdjustableEvent(Adjustable adjuster, int value) {
        GToolkit.postEvent(new GScrollPaneAdjustableEvent(this, adjuster, value));
    }

    public Dimension getPreferredSize() {        
        return target.getSize();
    }

    private native void setAdjusterNative(int orientation, int value, int max, int pageSize);
    private native void connectAdjuster(Adjustable adj, int orientation);
    
    private native boolean scrollbarVisible(int orientation);
}
