/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.automation;

/**
 * Represents pen event.
 */
public interface AutoPenEvent extends AutoEvent {
    /**
     * Gets pen state.
     *
     * @return AutoPenState representing pen state
     */
    public AutoPenState getPenState();

    /**
     * Gets X coord of pen tip.
     *
     * @return x coord of pen tip
     */
    public int getX();

    /**
     * Gets Y coord of pen tip.
     *
     * @return y coord of pen tip
     */
    public int getY();

    /**
     * Gets string representation of event. The format is following:
     *  pen x: x_value, y: y_value, state: state_value
     * where x_value, y_value and state_value are string representation 
     * of x coord, y coord and pen state.
     * For example:
     *  pen x: 10, y: 10, state: pressed
     */
    public String toString();
}
