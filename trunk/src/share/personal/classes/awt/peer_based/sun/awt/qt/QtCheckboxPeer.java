/*
 * @(#)QtCheckboxPeer.java	1.12 06/10/10
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
import java.awt.event.*;
import sun.awt.peer.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class QtCheckboxPeer extends QtComponentPeer implements CheckboxPeer
{
	private static native void initIDs();
	
	static
	{
		initIDs();
	}
	
	/** Creates a new QtCheckboxPeer. */

	QtCheckboxPeer (QtToolkit toolkit, Checkbox target)
	{
		super (toolkit, target);
		setState (target.getState());
		setLabel (target.getLabel());
	}
	
	protected void create(QtComponentPeer parentPeer)
	{
		CheckboxGroup group = ((Checkbox)target).getCheckboxGroup();
		
		create (parentPeer, group != null);
	}
	
	private native void create (QtComponentPeer parentPeer, boolean radio);
	
	public boolean isFocusTraversable() {return true;}
	
	public native void setState(boolean state);
	
	/* TODO: Changing of checkbox group after widget created. */
	
    public void setCheckboxGroup(CheckboxGroup g) { }
	
    public void setLabel(String label)
    {
        setLabelNative(label);
    }
    
    public boolean isFocusable() {
        return true;
    }

    private native void setLabelNative(String label);
	
	private void postItemEvent (boolean state)
	{
		Checkbox cb = (Checkbox)target;
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

        // For some reason Java 2 only sends selected events is the 
        // checkbox is in a chekbox group.
        // I don't know why this is but implemented this anyway 
        // to be consistent.
        if (cbg == null || state)    
            QtToolkit.postEvent(new ItemEvent(cb, 
                                              ItemEvent.ITEM_STATE_CHANGED,
                                              cb.getLabel(),
                                              state? ItemEvent.SELECTED : 
                                              ItemEvent.DESELECTED));
        }
}

