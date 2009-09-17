/*
 * @(#)Region.java	1.16 06/10/10
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

import java.util.Enumeration;
import java.awt.*;

/**
 * This is the interface for an object which represents rectilinear regions
 * (e.g. window visibility and clip regions).
 *
 * @version 1.11, 08/19/02
 */
public interface Region extends java.awt.Shape {
    /**
     * Create an independent copy of the region.
     */
    Region copy();
    /**
     * Test for equality against the given region.
     * @param r The region against which to test.
     * @return true if this region is equal to r, false otherwise.
     */
    boolean equals(Region r);
    /**
     * Does the region represent a simple rectangle?
     * @return true if this region contains only one rectangle, false otherwise.
     */
    boolean isRectangle();
    /**
     * Is the region empty?
     * @return true if this region contains no area, false otherwise.
     */
    boolean isEmpty();
    /**
     * Set the region to an empty region.
     */
    void setEmpty();
    /**
     * Translate the entire region by the vector (dx, dy).
     * @param dx The x component of the translation.
     * @param dy The y component of the translation.
     */
    void translate(int dx, int dy);
    /**
     * Return an <code>Enumeration</code> object which can be used to loop
     * through the elements of this region.  WARNING: the enumeration
     * is not responsible for making sure that the region is not changed
     * in the meantime.  If there is any risk of this, you should do
     * <code>region.copy().getRectangles()</code>
     * @return An <code>Enumeration</code> object associated with this region.
     */
    Enumeration getRectangles();
    /**
     * Defragment a region.  This call is a convenience for the <code>Region</code>
     * implementation; it indicates that a number of successive operations
     * have been done and now the region can be expected to stay static for
     * a period of time.
     * @param isClip Indicates that the region is going to be used directly
     * as a clip region.
     */
    void defragment(boolean isClip);
    /**
     * Get the bounding box of the region, <i>i.e.</i> the smallest rectangle
     * which completely encloses the region's current contents.  If the
     * region is empty, returns the rectangle (0,0,0,0).
     * @return A <code>Rectangle</code> describing the bounding box.
     */
    Rectangle getBounds();
    /**
     * Do a destructive union operation with the given region.
     * @param r The other <code>Region</code> with which this <code>Region</code> 
     * should be combined.
     */
    void union(Region r);
    /**
     * Do a destructive union operation with the rectangle (x,y,w,h).
     * @param x The x coordinate of the rectangle used for the union.
     * @param y The y coordinate of the rectangle used for the union.
     * @param w The width of the rectangle used for the union.
     * @param h The height of the rectangle used for the union.
     */
    void union(int x, int y, int w, int h);
    /**
     * Do a destructive union operation with the given <code>Rectangle</code>.
     * @param r The rectangle which should be added to this region.
     */
    void union(Rectangle r);
    /**
     * Do a destructive subtract operation with the given region.
     * @param r The region which should be subtracted out.
     */
    void subtract(Region r);
    /**
     * Do a destructive subtraction, removing the rectangle (x,y,w,h)
     * from the current region.
     * @param x The x coordinate of the rectangle to be subtracted.
     * @param y The y coordinate of the rectangle to be subtracted.
     * @param w The width of the rectangle to be subtracted.
     * @param h The height of the rectangle to be subtracted.
     */
    void subtract(int x, int y, int w, int h);
    /**
     * Do a destructive subtraction, removing the given <code>Rectangle</code>.
     * @param r The rectangle to be subtracted out.
     */
    void subtract(Rectangle r);
    /**
     * Do a destructive intersection operation with the given region.
     * @param r The region with which this region should be intersected.
     */
    void intersect(Region r);
    /**
     * Do a destructive intersection operation with a region described by
     * the rectangle (x,y,w,h).
     * @param x The x coordinate of the rectangle used for the intersection.
     * @param y The y coordinate of the rectangle used for the intersection.
     * @param w The width of the rectangle used for the intersection.
     * @param h The height of the rectangle used for the intersection.
     */
    void intersect(int x, int y, int w, int h);
    /**
     * Do a destructive intersection operation with a region described by
     * the given <code>Rectangle</code>.
     * @param r The rectangle with which this region should be intersected.
     */
    void intersect(Rectangle r);
    /**
     * Does the region contain the given point?
     * @param x The x coordinate of the test point
     * @param y The y coordinate of the test point
     * @return true if (x,y) is in the region, false otherwise.
     */
    boolean contains(int x, int y);
    /**
     * Does the region contain the given rectangle (in its entirety)?
     * @param x The x coordinate of the test rectangle
     * @param y The y coordinate of the test rectangle
     * @param w The width of the test rectangle
     * @param h The height of the test rectangle
     * @return true if (x,y,w,h) is entirely in the region, false otherwise.
     */
    boolean contains(int x, int y, int w, int h);
    /**
     * A quick, bounding box overlap test to see whether this region occupies 
     * the same general space as the given region r.
     * An overlap is possible only if the bounding box of this region 
     * overlaps the bounding box of r.  The bounding box test quickly eliminates
     * regions from consideration if they can't possibly intersect.
     * @param r The region which is to be tested for overlap.
     * @return true if an overlap is possible, false otherwise.
     */
    boolean mayIntersect(Region r);
    /**
     * A quick, bounding box overlap test to see whether this region occupies 
     * the same general space as a rectangle described by the given (x,y,w,h) tuple.
     * An overlap is possible only if the bounding box of this region 
     * overlaps the area of the rectangle.  The bounding box test quickly eliminates
     * regions from consideration if they can't possibly intersect.
     * @param x The x coordinate of the test rectangle
     * @param y The y coordinate of the test rectangle
     * @param w The width of the test rectangle
     * @param h The height of the test rectangle
     * @return true if an overlap is possible, false otherwise.
     */
    boolean mayIntersect(int x, int y, int w, int h);
    /**
     * A quick, bounding box overlap test to see whether this region occupies 
     * the same general space as the given rectangle r.
     * An overlap is possible only if the bounding box of this region 
     * overlaps the area of the rectangle.  The bounding box test quickly eliminates
     * regions from consideration if they can't possibly intersect.
     * @param r The rectangle which is to be tested for overlap.
     * @return true If an overlap is possible, false otherwise.
     */
    boolean mayIntersect(Rectangle r);
}
