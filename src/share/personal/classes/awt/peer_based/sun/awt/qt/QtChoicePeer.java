/*
 * @(#)QtChoicePeer.java	1.13 06/10/10
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

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;

/**
 * Peer object for Qt+ choice widget.
 *
 */

class QtChoicePeer extends QtComponentPeer implements ChoicePeer
{
    private static native void initIDs();
	
    static
    {
	initIDs();
    }
	
    QtChoicePeer (QtToolkit toolkit, Choice target)
    {
	super (toolkit, target);
		
	int itemCount = target.getItemCount();
		
	for (int i = 0; i < itemCount; i++)
	    add (target.getItem(i), i);
		
	int selectedIndex = target.getSelectedIndex();
		
	if (selectedIndex >= 0 && itemCount > 0)
	    select(selectedIndex);
    }
	
    protected native void create( QtComponentPeer parentPeer );
	
    public boolean isFocusTraversable()
    {
        return true;
    }
	
    protected native void addNative( String item, int index );

    public void add( String item, int index )
    {
        if (item != null)
            addNative(item, index );
        target.invalidate();
    }
	
    private native void removeNative( int index );
  
    public void remove( int index )
    {
        removeNative(index );
        target.invalidate();
    }

    public native void select(int index);

    public boolean isFocusable() {
        return true;
    } 
  
    private void postItemEvent (int index)
    {
	Choice c = (Choice)target;
	QtToolkit.postEvent(new ItemEvent(c, ItemEvent.ITEM_STATE_CHANGED,
	 				  c.getItem(index),
                                          ItemEvent.SELECTED));
    }

}

