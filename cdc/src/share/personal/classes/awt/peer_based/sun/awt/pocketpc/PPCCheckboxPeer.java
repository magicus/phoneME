/*
 * @(#)PPCCheckboxPeer.java	1.3 02/12/09
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
import java.awt.event.*;
import sun.awt.peer.*;

class PPCCheckboxPeer extends PPCComponentPeer implements CheckboxPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new PPCCheckboxPeer. */

    PPCCheckboxPeer (Checkbox target) {
        super(target);
        //perhaps not necessary?
        //setState(target.getState());
        //setLabel(target.getLabel());
    }
	
    native void create(PPCComponentPeer parent);
	
    public boolean isFocusTraversable() {
        return true;
    }
	
    public native void setState(boolean state);
    public native void setCheckboxGroup(CheckboxGroup g); 
    private native void setLabelNative(String label);
	
    public void setLabel(String label) {
        setLabelNative(label);
    }

    // not in pjava - may not be necessary
    private void postItemEvent(boolean state) {
        Checkbox cb = (Checkbox) target;
        CheckboxGroup cbg = cb.getCheckboxGroup();
        /* Bugid 4039594. If this is the current Checkbox in a CheckboxGroup,
         * then return to prevent deselection. Otherwise, it's logical state
         * will be turned off, but it will appear on.
         */
        if ((cbg != null) && (cbg.getSelectedCheckbox() == cb) &&
            cb.getState()) {
            cb.setState(true);
            return;
        }
        cb.setState(state);
        // For some reason Java 2 only sends selected events is the checkbox 
        // is in a chekbox group. I don't know why this is but implemented 
        // this anyway to be consistent.
		
        if (cbg == null || state)
            PPCToolkit.postEvent(new ItemEvent(cb, 
                         ItemEvent.ITEM_STATE_CHANGED,
                         cb.getLabel(),
                         state ? ItemEvent.SELECTED : ItemEvent.DESELECTED));
    }

    // pjava code
    public Dimension getMinimumSize() {
       String lbl = ((Checkbox)target).getLabel();
       if (lbl != null) {
          FontMetrics fm = getFontMetrics(((Checkbox)target).getFont());
          return new Dimension(30 + fm.stringWidth(lbl), 
                               Math.max(fm.getHeight() + 8, 15));
       }
       return new Dimension(20, 15);
    }

    // pjava code
    void initialize() {
        Checkbox t = (Checkbox)target;
        setState(t.getState());
        setCheckboxGroup(t.getCheckboxGroup());

        Color bg = ((Component)target).getBackground();
        if (bg != null) {
            setBackground(bg);
        }

        super.initialize();
    }

    // pjava code
    void clearRectBeforePaint(Graphics g, Rectangle r) {
       // Overload to do nothing for native components
    }

    // pjava native callbacks
    void handleAction(boolean state) {
        Checkbox cb = (Checkbox)this.target;
        cb.setState(state);
        PPCToolkit.postEvent(new ItemEvent(cb, ItemEvent.ITEM_STATE_CHANGED,
                         cb.getLabel(),
                         state? ItemEvent.SELECTED : ItemEvent.DESELECTED));

    }

    // pjava code
    public Dimension minimumSize() {
            return getMinimumSize();
    }
}
