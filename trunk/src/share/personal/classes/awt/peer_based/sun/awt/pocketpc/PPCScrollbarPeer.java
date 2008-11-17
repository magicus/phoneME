/*
 * @(#)PPCScrollbarPeer.java	1.8 06/10/10
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

/**
 * PPCScrollbarPeer.java
 *
 * @author Created by Omnicore CodeGuide
 */

package sun.awt.pocketpc;

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;

class PPCScrollbarPeer extends PPCComponentPeer implements ScrollbarPeer {
    private static native void initIDs();
    static {
        initIDs();
    }

    boolean ignore;

    // ComponentPeer overrides
    public Dimension getMinimumSize() {
	if (((Scrollbar)target).getOrientation() == Scrollbar.VERTICAL) {
	    return new Dimension(18, 50);
	}
	else {
	    return new Dimension(50, 18);
	}
    }

    // ScrollbarPeer implementation
    public void setValues(int value, int visible, int minimum, int maximum) {
        if (!ignore) {
            setValuesNative(value, visible, minimum, maximum);
        }
    }
    native void setValuesNative(int value, int visible, int minimum, int maximum); 
    public native void setLineIncrement(int l);
    public native void setPageIncrement(int l);

    // Toolkit & peer internals
    
    PPCScrollbarPeer(Scrollbar target) {
	super(target);
        ignore = false;
    }

    native void create(PPCComponentPeer parent);
    
    void initialize() {
	Scrollbar sb = (Scrollbar)target;
	setValues(sb.getValue(), sb.getVisibleAmount(), 
		  sb.getMinimum(), sb.getMaximum());
	super.initialize();
    }

    void clearRectBeforePaint(Graphics g, Rectangle r) {
        // Overload to do nothing for native components
    }
    
    // native callbacks

    void lineUp(int value) {
        PPCToolkit.postEvent(new Adjustor((Scrollbar)target, value, this,
                                        AdjustmentEvent.UNIT_DECREMENT));
    }

    void lineDown(int value) {
        PPCToolkit.postEvent(new Adjustor((Scrollbar)target, value, this,
                                        AdjustmentEvent.UNIT_INCREMENT));
    }

    void pageUp(int value) {
        PPCToolkit.postEvent(new Adjustor((Scrollbar)target, value, this,
                                        AdjustmentEvent.BLOCK_DECREMENT));
    }

    void pageDown(int value) {
        PPCToolkit.postEvent(new Adjustor((Scrollbar)target, value, this,
                                        AdjustmentEvent.BLOCK_INCREMENT));
    }

    void dragBegin(int value) {
// New event model event -- not supported yet.
//	((Scrollbar)target).setValue(value);
//	WToolkit.postEvent(new Event(target, Event.SCROLL_BEGIN, new Integer(value)));
    }

    void dragAbsolute(int value) {
        PPCToolkit.postEvent(new Adjustor((Scrollbar)target, value, this,
                                        AdjustmentEvent.TRACK));
    }

    void dragEnd(int value) {
// New event model event -- not supported yet.
//	((Scrollbar)target).setValue(value);
//	WToolkit.postEvent(new Event(target, Event.SCROLL_END, new Integer(value)));
    }

    /**
     * DEPRECATED
     */
    public Dimension minimumSize() {
	return getMinimumSize();
    }

    /**
     * This is used to change the Scrollbar on another thread without
     * deadlocking.
     */
    class Adjustor extends AWTEvent implements java.awt.ActiveEvent {

	Adjustor(Scrollbar sb, int value, PPCScrollbarPeer peer, int type) {
	    super(peer, 0);
	    this.sb = sb;
	    this.value = value;
	    this.peer = peer;
            this.type = type;
            consumed = true;
	}

        public void dispatch() {
	    sb.setValue(value);
	    peer.ignore = true;
            PPCToolkit.postEvent(new AdjustmentEvent(sb, 
                                     AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED,
                                     type, value));
	    peer.ignore = false;
	}

	int value;
	Scrollbar sb;
	PPCScrollbarPeer peer;
        int type;
    }
}
