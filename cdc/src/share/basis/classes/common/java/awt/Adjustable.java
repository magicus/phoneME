/*
 * @(#)Adjustable.java	1.15 06/10/10
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
 * The interface for objects which have an adjustable numeric value
 * contained within a bounded range of values.
 *
 * @version 1.10 08/19/02
 * @author Amy Fowler
 * @author Tim Prinzing
 */

public interface Adjustable {
    /**
     * The horizontal orientation.  
     */
    public static final int HORIZONTAL = 0; 
    /**
     * The vertical orientation.  
     */
    public static final int VERTICAL = 1;    

    /**
     * Indicates that the <code>Adjustable</code> has no orientation.
     */
    public static final int NO_ORIENTATION = 2;

    /**
     * Gets the orientation of the adjustable object.
     */
    int getOrientation();
    /**
     * Sets the minimum value of the adjustable object.
     * @param min the minimum value
     */
    void setMinimum(int min);
    /**
     * Gets the minimum value of the adjustable object.
     */
    int getMinimum();
    /**
     * Sets the maximum value of the adjustable object.
     * @param max the maximum value
     */
    void setMaximum(int max);
    /**
     * Gets the maximum value of the adjustable object.
     */
    int getMaximum();
    /**
     * Sets the unit value increment for the adjustable object.
     * @param u the unit increment
     */
    void setUnitIncrement(int u);
    /**
     * Gets the unit value increment for the adjustable object.
     */
    int getUnitIncrement();
    /**
     * Sets the block value increment for the adjustable object.
     * @param b the block increment
     */
    void setBlockIncrement(int b);
    /**
     * Gets the block value increment for the adjustable object.
     */
    int getBlockIncrement();
    /**
     * Sets the length of the proportionl indicator of the
     * adjustable object.
     * @param v the length of the indicator
     */
    void setVisibleAmount(int v);
    /**
     * Gets the length of the propertional indicator.
     */
    int getVisibleAmount();
    /**
     * Sets the current value of the adjustable object. This
     * value must be within the range defined by the minimum and
     * maximum values for this object.
     * @param v the current value 
     */
    void setValue(int v);
    /**
     * Gets the current value of the adjustable object.
     */
    int getValue();
    /**
     * Add a listener to recieve adjustment events when the value of
     * the adjustable object changes.
     * @param l the listener to recieve events
     * @see AdjustmentEvent
     */    
    void addAdjustmentListener(AdjustmentListener l);
    /**
     * Removes an adjustment listener.
     * @param l the listener being removed
     * @see AdjustmentEvent
     */ 
    void removeAdjustmentListener(AdjustmentListener l);
}    
