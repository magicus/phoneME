/*
 * @(#)QtScrollPanePeer.java	1.22 06/10/20
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
package sun.awt.qt;


import java.awt.*;
import java.awt.event.AdjustmentEvent;
import sun.awt.peer.*;


/**
 *
 *
 * @author Nicholas Allen
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
	Insets insets = new Insets(2, 2, 2, 2);

        // 6347067.
        // TCK: ScrollPane test failed when waiting time is added after the validate().
        // Fixed 6228838: Resizing the panel cause wrong scroll bar range
        // on zaurus, which, in turn, fixed the viewport size problem in
        // CDC 1.1 linux-x86.  But zaurus still has a problem which is shown when
        // running the PP-TCK interactive ComponetTests where two tests will have
        // both scroll bars on and in these two tests, you can see that the
        // bottom of the "Yes" "No" buttons are chopped off.
        //
        // The getInsets() call is modified to calculate whether scrollbars are on
        // in order to return the correct insets.  In particular,
        //
        // Given that the hScrollbarHeight and vScrollbarWidth are known, which
        // is true in the Qt port case:
        //
        // hScrollbarOn is a function of scrollpane dim, child dim, as well as
        // vScrollbarOn in the boundary case where the horizontal scrollbar could
        // be needed if the vertical scrollbar needs to be present and the extra
        // width due to the vertical scrollbar just makes the horizontal
        // scrollbar necessary!

        ScrollPane sp = (ScrollPane)target;
        Dimension d = sp.size();
        Component c = getScrollChild();
        Dimension cd;
        if (c != null) {
            cd = c.size();
        } else {
            cd = new Dimension(0, 0);
        }

        if (scrollbarDisplayPolicy == ScrollPane.SCROLLBARS_ALWAYS) {
            insets.right += vScrollbarWidth;
            insets.bottom += hScrollbarHeight;
        } else if (scrollbarDisplayPolicy == ScrollPane.SCROLLBARS_AS_NEEDED) {
            if (d.width - insets.left*2 < cd.width) {
                // Hbar is necessary.
                insets.bottom += hScrollbarHeight;
                if (d.height - insets.top - insets.bottom < cd.height) {
                    insets.right += vScrollbarWidth;
                }
            } else if (d.width - insets.left*2 - cd.width >= vScrollbarWidth) {
                // We're very sure that hbar will not be on.
                if (d.height - insets.top*2 < cd.height) {
                    insets.right += vScrollbarWidth;
                }
            } else {
                // Borderline case so we need to check vbar first.
                if (d.height - insets.top*2 < cd.height) {
                    insets.right += vScrollbarWidth;
                    if (d.width - insets.left - insets.right < cd.width) {
                        // Hbar is needed after all!
                        insets.bottom += hScrollbarHeight;
                    }
                }
            }
        } 
        // 6347067.

	return insets;
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
