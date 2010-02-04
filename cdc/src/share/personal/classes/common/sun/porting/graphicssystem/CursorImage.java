/*
 * @(#)CursorImage.java	1.13 06/10/10
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

import java.awt.Image;

/**
 * CursorImage is an object which encapsulates the bitmap and hotspot
 * data describing a cursor.  It is used simply to hold all of the
 * required information in one place.  It also enables the native
 * implementation to build a cursor from this image data and cache it.
 * 
 * @version 1.8, 08/19/02
 */
public abstract class CursorImage {
    /**
     * An image which contains the shape of the cursor.  Image transparency
     * is used to determine which pixels of the cursor are painted and which
     * are not.  Colors in the image should be retained whenever possible (e.g.
     * if the underlying graphics system supports colored cursors).  At a
     * minimum, three pixel values (black, white, transparent) should be 
     * significant.
     */
    protected java.awt.Image image;
    /**
     * The x coordinate of the cursor hot spot, i.e. the coordinate within
     * the image that corresponds to the cursor's location on screen.
     */
    protected int hotX;
    /**
     * The y coordinate of the cursor hot spot, i.e. the coordinate within
     * the image that corresponds to the cursor's location on screen.
     */
    protected int hotY;
    /**
     * Create a new CursorImage object from the given image data and hotspot.
     * It is the responsibility of the caller to make sure that the image data
     * contains no more colors than are supported by the graphics system, and
     * that the size of the cursor image is supported.
     * @param imageData An image, probably with transparency, which describes
     * the shape and coloration of the cursor.
     * @param x The x coordinate of the cursor hotspot.
     * @param y The y coordinate of the cursor hotspot.
     * @see GraphicsSystem#getMaximumCursorColors
     * @see GraphicsSystem#getMaximumCursorSize
     * @see GraphicsSystem#getBestCursorSize
     */
    public CursorImage(java.awt.Image imageData, int x, int y) {
        image = imageData;
        hotX = x;
        hotY = y;
    }
}
