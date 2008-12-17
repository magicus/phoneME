/*
 * @(#)GListPeer.java	1.20 06/10/10
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

package sun.awt.gtk;

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class GListPeer extends GComponentPeer implements ListPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new GListPeer. */

    public GListPeer (GToolkit toolkit, List target) {
        super(toolkit, target);
        int itemCount = target.getItemCount();
        for (int i = 0; i < itemCount; i++)
            add(target.getItem(i), i);
        setMultipleMode(target.isMultipleMode());
        int[] selected = target.getSelectedIndexes();
        if (selected != null) {
            for (int i = 0; i < selected.length; i++)
                select(selected[i]);
        }
        makeVisible(target.getVisibleIndex());
    }
	
    private native void createNative(int rowHeight);

    protected void create() {
        FontMetrics fm = getFontMetrics(((List) target).getFont());
        createNative(fm.getHeight());
    }
	
    public boolean isFocusTraversable() {
        return true;
    }

    public native int[] getSelectedIndexes();

    public void add(String item, int index) {
        if (item != null)
            addNative(stringToNulMultiByte(item), index);
    }

    private native void setRowHeight(int rowHeight);

    public void setFont(Font f) {
        super.setFont(f);
        FontMetrics fm = getFontMetrics(f);
        setRowHeight(fm.getHeight());
    }

    protected native void addNative(byte[] item, int index);

    public native void delItems(int start, int end);

    public native void removeAll();

    public native void select(int index);

    public native void deselect(int index);

    public native void makeVisible(int index);

    public native void setMultipleMode(boolean b);
    final static int 	MARGIN = 2;
    final static int 	SPACE = 1;
    final static int 	SCROLLBAR = 20;
    public Dimension getPreferredSize(int rows) {
        FontMetrics fm = getFontMetrics(((List) target).getFont());
        return new Dimension(SCROLLBAR + 2 * MARGIN +
                fm.stringWidth("0123456789abcde"),
                ((fm.getHeight() + 2 * SPACE) * rows) +
                2 * MARGIN);
    }

    public Dimension getMinimumSize(int rows) {
        FontMetrics fm = getFontMetrics(((List) target).getFont());
        return new Dimension(SCROLLBAR + 2 * MARGIN +
                fm.stringWidth("0123456789"),
                ((fm.getHeight() + 2 * SPACE) * rows) +
                2 * MARGIN);
    }
	
    private void postItemEvent(int row, boolean selected, boolean postActionEvent) {
        List list = (List) target;
        if (list.isMultipleMode() || selected) {
            GToolkit.postEvent(new ItemEvent (list, ItemEvent.ITEM_STATE_CHANGED, new Integer(row), selected ? ItemEvent.SELECTED : ItemEvent.DESELECTED));
            if (postActionEvent)
                GToolkit.postEvent(new ActionEvent (list, ActionEvent.ACTION_PERFORMED, list.getItem(row)));
        }
    }

    // added method for Bug #4685270
    private void postActionEvent(int row) {
        List list = (List) target;
        GToolkit.postEvent(new ActionEvent (list, ActionEvent.ACTION_PERFORMED, list.getItem(row)));
    }
}
