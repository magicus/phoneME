/*
 * @(#)QtScrollbarPeer.java	1.13 06/10/10
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
/**
 * QtScrollbarPeer.java
 *
 * @author Created by Omnicore CodeGuide
 */

package sun.awt.qt;

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;

class QtScrollbarPeer extends QtComponentPeer implements ScrollbarPeer
{
    private static native void initIDs ();
	
    static
    {
	initIDs();
    }
	
    QtScrollbarPeer (QtToolkit toolkit, Scrollbar target)
    {
	super (toolkit, target);
		
	setValues (target.getValue(), target.getVisibleAmount(), target.getMinimum(), target.getMaximum());
	setLineIncrement (target.getLineIncrement());
	setPageIncrement (target.getPageIncrement());
    }
	
    protected void create (QtComponentPeer parentPeer)
    {
	createScrollbar (parentPeer, ((Scrollbar)target).getOrientation());
    }
	
    private native void createScrollbar (QtComponentPeer
					 parentPeer, int orientation);

    public boolean isFocusTraversable()
    {
        return isFocusTraversableNative();
    }

    private native boolean isFocusTraversableNative();

    public native void setValues(int value, int visible, int minimum, int maximum);
    public native void setLineIncrement(int l);
    public native void setPageIncrement(int l);
    
    public Dimension getPreferredSize() {
        if (((Scrollbar)target).getOrientation() == Scrollbar.HORIZONTAL) 
            return new Dimension(60, 16);
        else 
            return new Dimension(16, 60);
    }
	
    private void postAdjustmentEvent(int type, int value) {
        QtToolkit.postEvent(new AdjustmentEvent((Adjustable) target, AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED, type, value));
    }

    protected boolean shouldFocusOnClick() {
        return true;
    }
}

