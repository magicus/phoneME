/*
 * @(#)GeometryProvider.java	1.12 06/10/10
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

package sun.porting.graphicssystem;

/**
 * The <code>GeometryProvider</code> interface is what allows a Graphics
 * object to clip to a window.  Each time a <code>Graphics</code> object is used for
 * a draw, it is required to consult its associated <code>GeometryProvider</code> in
 * order to first lock the drawing surface, and then obtain the 
 * current transformation and clipping information for the operation.
 * If the drawing surface is an offscreen image, the implementation of
 * the <code>GeometryProvider</code> interface is trivial; if it is a window, the
 * window system lock, window's onscreen location, current clip area 
 * and background are made available via this interface.
 *
 * @version 1.7, 08/19/02
 */
public interface GeometryProvider extends sun.awt.DrawingSurfaceInfo {
    /**
     * Get the lock object associated with the
     * <code>GeometryProvider</code>.  <em>All operations must acquire
     * this lock <strong>as the first thing they do</strong>, and hold
     * the lock until the draw has been completed.</em> If the object
     * being drawn is a window, this can prevent the windows from
     * being moved during the draw; in any case it also prevents more
     * than one thread from drawing to the same object at the same
     * time.
     * @return An Object to be used as the lock for the current draw 
     * operation.
     */
    Object getLock();
    /**
     * Get the validation identifier.  This is the same number that is
     * returned by lock(); if it has not changed since the previous
     * acquisition of the lock, the clipping information can be assumed
     * not to have changed.
     * @return The change identifier.
     */
    int getValidationID();
    /**
     * Get the background color, if any, that this object is
     * supposed to have.  This allows surfaces to stack in either 
     * an opaque or a transparent manner (a return value of null
     * indicates that the object is transparent).
     * @return A <code>java.awt.Color</code> object describing the 
     * background color for the drawing surface.
     */
    java.awt.Color getBackgroundColor();
}
