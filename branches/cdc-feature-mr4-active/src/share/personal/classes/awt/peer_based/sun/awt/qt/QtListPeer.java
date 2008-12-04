/*
 * @(#)QtListPeer.java	1.14 06/10/10
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
 *
 *
 * @author Nicholas Allen
 */

class QtListPeer extends QtComponentPeer implements ListPeer {
	
    private static native void initIDs();
	
    static
    {
	initIDs();
    }
	
    /** Creates a new QtListPeer. */

    public QtListPeer (QtToolkit toolkit, List target)
    {
	super (toolkit, target);
		
	int itemCount = target.getItemCount();
		
	for (int i = 0; i < itemCount; i++)
	    add (target.getItem(i), i);
		
	setMultipleMode(target.isMultipleMode());
		
	int [] selected = target.getSelectedIndexes();
		
	if (selected != null)
	    {
		for (int i = 0; i < selected.length; i++)
		    select (selected[i]);
	    }
    }
	
    protected native void create(QtComponentPeer parentPeer);
	
    public boolean isFocusTraversable() {return true;}
    public native int[] getSelectedIndexes();

    public void add(String item, int index)
    {
        if(item!=null)
	    addNative(item, index);
    }
    protected native void addNative(String item, int index);

    public native void delItems(int start, int end);
    public native void removeAll();
    public native void select(int index);
    public native void deselect(int index);
    public native void makeVisible(int index);
    public native void setMultipleMode(boolean b);

    private native int preferredWidth(int rows);
    private native int preferredHeight(int rows);

    public Dimension getPreferredSize(int rows) {
	return new Dimension(preferredWidth(rows),
			     preferredHeight(rows));
    }

    public Dimension getMinimumSize(int rows) {
	return getPreferredSize(rows);
    }

    /**
     * DEPRECATED:  Replaced by add(String, int).
     */
    public void addItem(String item, int index) {add (item, index);}

    /**
     * DEPRECATED:  Replaced by removeAll().
     */
    public void clear() {removeAll();}

    /**
     * DEPRECATED:  Replaced by setMultipleMode(boolean).
     */
    public void setMultipleSelections(boolean v) {setMultipleMode(v);}

    /**
     * DEPRECATED:  Replaced by getPreferredSize(int).
     */
    public Dimension preferredSize(int v) {return getPreferredSize(v);}

    /**
     * DEPRECATED:  Replaced by getMinimumSize(int).
     */
    public Dimension minimumSize(int v) {return getMinimumSize(v);}
	
    private void postItemEvent (int row, boolean selected, 
                                boolean postActionEvent)
    {
	List list = (List)target;
        if(list.isMultipleMode() || selected) {
            QtToolkit.postEvent (new ItemEvent (list, ItemEvent.ITEM_STATE_CHANGED, new Integer(row), selected ? ItemEvent.SELECTED : ItemEvent.DESELECTED));
            if (postActionEvent)
                QtToolkit.postEvent(new ActionEvent (list, ActionEvent.ACTION_PERFORMED, list.getItem(row)));
        }
    }

    private void postActionEvent(int row) {
        List list = (List) target;
        
        QtToolkit.postEvent(new ActionEvent (list, ActionEvent.ACTION_PERFORMED, list.getItem(row)));
    }
 
    public boolean isFocusable() {
        return true;
    }
}

