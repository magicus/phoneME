/*
 * @(#)PPCButtonPeer.java	1.3 02/12/09
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

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;

/**
 * PPCButtonPeer.java
 *
 * @author Nicholas Allen
 */

class PPCButtonPeer extends PPCComponentPeer implements ButtonPeer {

    static
    {
        initIDs();
    }

    PPCButtonPeer (Button target) {
        super(target);
        //Perhaps not necessary?
        //setLabel(target.getLabel());
    }
	
    public boolean isFocusTraversable() {
        return true;
    }
	
    native void create(PPCComponentPeer peer);

    private native void setLabelNative(String label);
    private static native void initIDs();
	
    public void setLabel(String label) {
        setLabelNative(label);
        // may not be necessary?
        //((Component)target).invalidate();
    }


    public Dimension getMinimumSize() {
        FontMetrics fm = getFontMetrics(((Button)target).getFont());
        return new Dimension(fm.stringWidth(((Button)target).getLabel()) + 14, 
                             fm.getHeight() + 8);
    }

    public Dimension minimumSize() {
        return getMinimumSize();
    }

    public void handleAction() {
        PPCToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED,
                                  ((Button)target).getActionCommand()));
    }

    void clearRectBeforePaint(Graphics g, Rectangle r) {
        // Overload to do nothing for native components
    }
}
