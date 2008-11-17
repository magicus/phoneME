/*
 * @(#)ItemEvent.java	1.23 06/10/10
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

package java.awt.event;

import java.awt.Component;
import java.awt.AWTEvent;
import java.awt.ItemSelectable;

/**
 * The item event emitted by ItemSelectable objects.
 * This event is generated when an item is selected or de-selected.
 * @see java.awt.ItemSelectable
 * @see ItemListener
 *
 * @version 1.19 08/19/02
 * @author Carl Quinn
 */
public class ItemEvent extends AWTEvent {
    /**
     * Marks the first integer id for the range of item event ids.
     */
    public static final int ITEM_FIRST = 701;
    /**
     * Marks the last integer id for the range of item event ids.
     */
    public static final int ITEM_LAST = 701;
    /** 
     * The item state changed event type.
     */
    public static final int ITEM_STATE_CHANGED = ITEM_FIRST; //Event.LIST_SELECT 
    /**
     * The item selected state change type.
     */
    public static final int SELECTED = 1;
    /** 
     * The item de-selected state change type.
     */
    public static final int DESELECTED = 2;
    Object item;
    int stateChange;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = -608708132447206933L;
    /**
     * Constructs a ItemSelectEvent object with the specified ItemSelectable source,
     * type, item, and item select state.
     * @param source the ItemSelectable object where the event originated
     * @id the event type
     * @item the item where the event occurred
     * @stateChange the state change type which caused the event
     */
    public ItemEvent(ItemSelectable source, int id, Object item, int stateChange) {
        super(source, id);
        this.item = item;
        this.stateChange = stateChange;
    }

    /**
     * Returns the ItemSelectable object where this event originated.
     */
    public ItemSelectable getItemSelectable() {
        return (ItemSelectable) source;
    }

    /**
     * Returns the item where the event occurred.
     */
    public Object getItem() {
        return item;
    }

    /**
     * Returns the state change type which generated the event.
     * @see #SELECTED
     * @see #DESELECTED
     */
    public int getStateChange() {
        return stateChange;
    }

    public String paramString() {
        String typeStr;
        switch (id) {
        case ITEM_STATE_CHANGED:
            typeStr = "ITEM_STATE_CHANGED";
            break;

        default:
            typeStr = "unknown type";
        }
        String stateStr;
        switch (stateChange) {
        case SELECTED:
            stateStr = "SELECTED";
            break;

        case DESELECTED:
            stateStr = "DESELECTED";
            break;

        default:
            stateStr = "unknown type";
        }
        return typeStr + ",item=" + item + ",stateChange=" + stateStr;
    }
}
