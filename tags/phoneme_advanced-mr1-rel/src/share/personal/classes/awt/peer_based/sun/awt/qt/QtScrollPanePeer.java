/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
package sun.awt.qt;


import java.awt.*;
import java.awt.event.AdjustmentEvent;
import sun.awt.peer.*;


/**
 *
 *
 */
class QtScrollPanePeer extends QtContainerPeer implements ScrollPanePeer
{
    private static native void initIDs();
    private static int hScrollbarHeight = calculateHScrollbarHeight();
    private static int vScrollbarWidth = calculateVScrollbarWidth();
	
    private static native int calculateHScrollbarHeight();
    private static native int calculateVScrollbarWidth();
    private native void enableScrollbarsNative(boolean hBarOn, boolean vBarOn);

    private boolean ignoreSetValue;
    
    private int scrollbarDisplayPolicy;
    
    public Dimension getPreferredSize() {
        return target.getSize();
    }
	
    static
    {
	initIDs();
    }
	
    /** Creates a new QtScrollPanePeer. */

    QtScrollPanePeer (QtToolkit toolkit, ScrollPane target)
    {
	super (toolkit, target);
        scrollbarDisplayPolicy = target.getScrollbarDisplayPolicy();
    }
	
    protected native void create(QtComponentPeer parentPeer);

    native void add (QtComponentPeer peer);

    void remove(QtComponentPeer peer) {
        updateScrollBarsNative();   //6249842
    }
	
    /** Overridden to calculate insets. This includes any optional
	scrollbars so we need to dynamically work this out. */
	
    public Insets getInsets() {
       
        // adjust predicted view size to account for scrollbars
        
        // This accounts for the 2 pixels from the edge of the viewport
        // to the edge of the scrollview on qt-emb-2.3.2.
	Insets inset = new Insets(2, 2, 2, 2);

        if (scrollbarVisible(Adjustable.VERTICAL))
            inset.right = vScrollbarWidth;
      
        if (scrollbarVisible(Adjustable.HORIZONTAL))
            inset.bottom = hScrollbarHeight;
 
	return inset;
    }
	
    public int getHScrollbarHeight() 
    {
	return hScrollbarHeight;
    }
    
    public int getVScrollbarWidth() 
    {
	return vScrollbarWidth;
    }
    
    public void childResized(int w, int h) {

        // Compensates forthe setBounds() call in ScrollPane.layout() because in
        // the native layer, the child component is reparented to the viewport
        // rather than to the scrollview widget.
        Component c = ((Container)target).getComponent(0);
        c.setLocation(c.getX() - 2, c.getY() - 2);

        if(scrollbarDisplayPolicy == ScrollPane.SCROLLBARS_AS_NEEDED) {
            childResizedNative(w, h); // 6228838
        }

        // Removed the scrollbar workaround for gtk.
    }

    public void setUnitIncrement(Adjustable adj, int u)	
    {
	setUnitIncrementNative(adj.getOrientation(), u);
    }

    private native void setUnitIncrementNative(int orientation, int increment);

    //6255265
    public int setValue(Adjustable adj, int v) {
        if(ignoreSetValue) return -1;   //6255265

        int orientation = adj.getOrientation();
        int max = adj.getMaximum();
        int pageSize = adj.getVisibleAmount();

        int rval = setAdjusterNative(orientation, v, max, pageSize);   //6255265
        return rval;
    }
    
    class QtScrollPaneAdjustableEvent extends AWTEvent implements ActiveEvent {
        QtScrollPanePeer peer;
        Adjustable adjuster;
        int value;
    
        QtScrollPaneAdjustableEvent(QtScrollPanePeer src, Adjustable adj, int val) {
            super(src, 0);
            peer = src;
            adjuster = adj;
            value = val;
        }

        public void dispatch() {
            // Fixed 6260600.
            // Qt can emit the valueChanged signals for horizontal or vertical
            // scroll bars after scroll child removal as a result of the call
            // made to updateScrollBarsNative.  If there is no scroll child,
            // it is not necessary to update the Adjustable for the current
            // value.
            // See also 6249842.
            if (getScrollChild() == null) {
                return;
            }

            peer.ignoreSetValue = true;
            adjuster.setValue(value);
            peer.ignoreSetValue = false;
        }
    }

    // Fixed 6260600.
    private Component getScrollChild() {
        ScrollPane sp = (ScrollPane)target;
        Component child = null;
        try {
            child = sp.getComponent(0);
        } catch (ArrayIndexOutOfBoundsException e) {
            // do nothing.  in this case we return null
        }
        return child;
    }
    // Fixed 6260600.

    private void postAdjustableEvent(int orientation, int value) {
        if (orientation == Adjustable.HORIZONTAL) {
            QtToolkit.postEvent(new QtScrollPaneAdjustableEvent(this, 
                           ((ScrollPane)target).getHAdjustable(), value));
        } else if (orientation == Adjustable.VERTICAL) {
            QtToolkit.postEvent(new QtScrollPaneAdjustableEvent(this, 
                           ((ScrollPane)target).getVAdjustable(), value));
        } 
    }

    private native int setAdjusterNative(int orientation, int value, int max, int pageSize);   //6255265
    private native boolean scrollbarVisible(int orientation);
    private native void updateScrollBarsNative();   //6249842
    private native void childResizedNative(int width, int height); // 6228838
}
