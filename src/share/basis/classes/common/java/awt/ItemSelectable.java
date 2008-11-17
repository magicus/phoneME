/*
 * @(#)ItemSelectable.java	1.13 06/10/10
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

package java.awt;

import java.awt.event.*;

/**
 * The interface for objects which contain a set of items for
 * which zero or more can be selected.
 *
 * @version 1.9 08/19/02
 * @author Amy Fowler
 */

public interface ItemSelectable {
    /**
     * Returns the selected items or null if no items are selected.
     */
    public Object[] getSelectedObjects();
    /**
     * Add a listener to recieve item events when the state of
     * an item changes.
     * @param l the listener to recieve events
     * @see ItemEvent
     */    
    public void addItemListener(ItemListener l);
    /**
     * Removes an item listener.
     * @param l the listener being removed
     * @see ItemEvent
     */ 
    public void removeItemListener(ItemListener l);
}
