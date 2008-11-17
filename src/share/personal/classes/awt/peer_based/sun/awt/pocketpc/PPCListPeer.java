/*
 * @(#)PPCListPeer.java	1.8 06/10/10
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
 *
 *
 * @author Nicholas Allen
 */

class PPCListPeer extends PPCComponentPeer implements ListPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    // ComponentPeer overrides

    public Dimension minimumSize() {
	return minimumSize(4);
    }
    public boolean isFocusTraversable() {
	return true;
    }
    
    // ListPeer implementation

    public int[] getSelectedIndexes() {
	List l = (List)target;
	int len = l.countItems();
	int sel[] = new int[len];
	int nsel = 0;
	for (int i = 0 ; i < len ; i++) {
	    if (isSelected(i)) {
		sel[nsel++] = i;
	    }
	}
	int selected[] = new int[nsel];	
	System.arraycopy(sel, 0, selected, 0, nsel);
	return selected;
    }

    /* New method name for 1.1 */
    public void add(String item, int index) {
	addItem(item, index);
    }

    /* New method name for 1.1 */
    public void removeAll() {
	clear();
    }

    /* New method name for 1.1 */
    public void setMultipleMode (boolean b) {
	setMultipleSelections(b);
    }

    /* New method name for 1.1 */
    public Dimension getPreferredSize(int rows) {
	return preferredSize(rows);
    }

    /* New method name for 1.1 */
    public Dimension getMinimumSize(int rows) {
	return minimumSize(rows);
    }


    private FontMetrics   fm;
    public void addItem(String item, int index) {
        addItemNative(item, index, fm.stringWidth(item));
    }
    native void addItemNative(String item, int index, int width);

    public native void delItems(int start, int end);
    public void clear() {
	List l = (List)target;
	delItems(0, l.countItems()-1);
    }
    public native void select(int index);
    public native void deselect(int index);
    public native void makeVisible(int index);
    public native void setMultipleSelections(boolean v);
    public native int  getMaxWidth();

    public Dimension preferredSize(int v) {
	Dimension d = minimumSize(v);
        d.width = Math.max(d.width, getMaxWidth() + 20);
        return d;
    }
    public Dimension minimumSize(int v) {
	return new Dimension(20 + fm.stringWidth("0123456789abcde"), 
                         (fm.getHeight() * v) + 4); // include borders
    }

    // Toolkit & peer internals
    
    PPCListPeer(List target) {
	super(target);
    }

    void clearRectBeforePaint(Graphics g, Rectangle r) {
        // Overload to do nothing for native components
    }

    native void create(PPCComponentPeer parent);

    void initialize() {
	List li = (List)target;

	fm = getFontMetrics( li.getFont() );

	// add any items that were already inserted in the target.
	int  nitems = li.countItems();
	for (int i = 0; i < nitems; i++) {
	    addItem(li.getItem(i), -1);
	}

	// set whether this list should allow multiple selections.
	setMultipleSelections(li.allowsMultipleSelections());

	// make the visible position visible.
	int index = li.getVisibleIndex();
	if (index >= 0) {
	    makeVisible(index);
	}

	// select the item if necessary.
	int sel[] = li.getSelectedIndexes();
	for (int i = 0 ; i < sel.length ; i++) {
	    select(sel[i]);
	}

	super.initialize();
    }

    private native void updateMaxItemWidth();

    /*public*/ native boolean isSelected(int index);

    // update the fontmetrics when the font changes
    public synchronized void setFont(Font f)
    {
        super.setFont( f );
	    fm = getFontMetrics( ((List)target).getFont() );
        updateMaxItemWidth();
    }


    // native callbacks

    void handleAction(int index) {
	List l = (List)target;
	l.select(index);
        PPCToolkit.postEvent(new ActionEvent(target, 
                                             ActionEvent.ACTION_PERFORMED,
                                             l.getItem(index)));
    }

    void handleListChanged(int index) {
	List l = (List)target;
        PPCToolkit.postEvent(new ItemEvent(
            l, ItemEvent.ITEM_STATE_CHANGED,
            new Integer(index),
            isSelected(index)? ItemEvent.SELECTED : 
            ItemEvent.DESELECTED));
        
    }
}
