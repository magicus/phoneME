/*
 * @(#)Polygon.java	1.32 06/10/10
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

/**
 * The <code>Polygon</code> class encapsulates a description of a 
 * closed, two-dimensional region within a coordinate space. This 
 * region is bounded by an arbitrary number of line segments, each of 
 * which is one side of the polygon. Internally, a polygon 
 * comprises of a list of (<i>x</i>,&nbsp;<i>y</i>) coordinate pairs, 
 * where each pair defines a <i>vertex</i> of the polygon, and two 
 * successive pairs are the endpoints of a line that is a side of the 
 * polygon. The first and final pairs of (<i>x</i>,&nbsp;<i>y</i>) 
 * points are joined by a line segment that closes the polygon.
 *
 * @version 	1.26, 08/19/02
 * @author 	Sami Shaio
 * @author      Herb Jellinek
 * @since       JDK1.0
 */
public class Polygon implements Shape, java.io.Serializable {
    /**
     * The total number of points.
     * @since JDK1.0
     */
    public int npoints = 0;
    /**
     * The array of <i>x</i> coordinates. 
     * @since   JDK1.0
     */
    public int xpoints[] = new int[4];
    /**
     * The array of <i>y</i> coordinates. 
     * @since   JDK1.0
     */
    public int ypoints[] = new int[4];
    /*
     * Bounds of the polygon.
     */
    protected Rectangle bounds = null;
    /* 
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = -6460061437900069969L;
    /**
     * Creates an empty polygon.
     * @since JDK1.0
     */
    public Polygon() {}

    /**
     * Constructs and initializes a polygon from the specified 
     * parameters. 
     * @param      xpoints   an array of <i>x</i> coordinates.
     * @param      ypoints   an array of <i>y</i> coordinates.
     * @param      npoints   the total number of points in the polygon.
     * @exception  NegativeArraySizeException if the value of
     *                       <code>npoints</code> is negative.
     * @since      JDK1.0
     */
    public Polygon(int xpoints[], int ypoints[], int npoints) {        
        // J2SDK 1.4 Fix 4489009: should throw IndexOutofBoundsException instead
        // of OutofMemoryException if npoints is huge and > {x,y}points.length
        // Compliant with Personal Basis Profile Specification.
        if (npoints > xpoints.length || npoints > ypoints.length) {
            throw new IndexOutOfBoundsException("npoints > xpoints.length || npoints > ypoints.length");
        }        
        this.npoints = npoints;
        this.xpoints = new int[npoints];
        this.ypoints = new int[npoints];
        System.arraycopy(xpoints, 0, this.xpoints, 0, npoints);
        System.arraycopy(ypoints, 0, this.ypoints, 0, npoints);	
    }
    
    /**
     * Resets this <code>Polygon</code> object to an empty polygon.
     * The coordinate arrays and the data in them are left untouched
     * but the number of points is reset to zero to mark the old
     * vertex data as invalid and to start accumulating new vertex
     * data at the beginning.
     * All internally-cached data relating to the old vertices
     * are discarded.
     * Note that since the coordinate arrays from before the reset
     * are reused, creating a new empty <code>Polygon</code> might
     * be more memory efficient than resetting the current one if
     * the number of vertices in the new polygon data is significantly
     * smaller than the number of vertices in the data from before the
     * reset.
     * @see         java.awt.Polygon#invalidate
     * @since 1.4
     */
    public void reset() {
        npoints = 0;
        bounds = null;
    }
    /**
     * Invalidates or flushes any internally-cached data that depends
     * on the vertex coordinates of this <code>Polygon</code>.
     * This method should be called after any direct manipulation
     * of the coordinates in the <code>xpoints</code> or
     * <code>ypoints</code> arrays to avoid inconsistent results
     * from methods such as <code>getBounds</code> or <code>contains</code>
     * that might cache data from earlier computations relating to
     * the vertex coordinates.
     * @see         java.awt.Polygon#getBounds
     * @since 1.4
     */
    public void invalidate() {
        bounds = null;
    }
    /**
     * Translates the vertices by <code>deltaX</code> along the 
     * <i>x</i> axis and by <code>deltaY</code> along the 
     * <i>y</i> axis.
     * @param deltaX the amount to translate along the <i>x</i> axis
     * @param deltaY the amount to translate along the <i>y</i> axis
     * @since JDK1.1
     */
    public void translate(int deltaX, int deltaY) {
        for (int i = 0; i < npoints; i++) {
            xpoints[i] += deltaX;
            ypoints[i] += deltaY;
        }
        if (bounds != null) {
            bounds.translate(deltaX, deltaY);
        }
    }

    /*
     * Calculate the bounding box of the points passed to the constructor.
     * Sets 'bounds' to the result.
     */
    void calculateBounds(int xpoints[], int ypoints[], int npoints) {
        int boundsMinX = Integer.MAX_VALUE;
        int boundsMinY = Integer.MAX_VALUE;
        int boundsMaxX = Integer.MIN_VALUE;
        int boundsMaxY = Integer.MIN_VALUE;
        for (int i = 0; i < npoints; i++) {
            int x = xpoints[i];
            boundsMinX = Math.min(boundsMinX, x);
            boundsMaxX = Math.max(boundsMaxX, x);
            int y = ypoints[i];
            boundsMinY = Math.min(boundsMinY, y);
            boundsMaxY = Math.max(boundsMaxY, y);
        }
        bounds = new Rectangle(boundsMinX, boundsMinY,
                    boundsMaxX - boundsMinX,
                    boundsMaxY - boundsMinY);
    }

    /*
     * Update the bounding box to fit the point x, y.
     */
    void updateBounds(int x, int y) {
        if (x < bounds.x) {
            bounds.width = bounds.width + (bounds.x - x);
            bounds.x = x;
        } else {
            bounds.width = Math.max(bounds.width, x - bounds.x);
            // bounds.x = bounds.x;
        }
        if (y < bounds.y) {
            bounds.height = bounds.height + (bounds.y - y);
            bounds.y = y;
        } else {
            bounds.height = Math.max(bounds.height, y - bounds.y);
            // bounds.y = bounds.y;
        }
    }	

    /**
     * Appends a point to this polygon. 
     * <p>
     * If an operation that calculates the bounding box of this polygon
     * has already been performed, such as <code>getBounds</code> 
     * or <code>contains</code>, then this method updates the bounding box. 
     * @param       x   the <i>x</i> coordinate of the point.
     * @param       y   the <i>y</i> coordinate of the point.
     * @see         java.awt.Polygon#getBounds
     * @see         java.awt.Polygon#contains
     * @since       JDK1.0
     */
    public void addPoint(int x, int y) {
        if (npoints == xpoints.length) {
            int tmp[];
            tmp = new int[npoints * 2];
            System.arraycopy(xpoints, 0, tmp, 0, npoints);
            xpoints = tmp;
            tmp = new int[npoints * 2];
            System.arraycopy(ypoints, 0, tmp, 0, npoints);
            ypoints = tmp;
        }
        xpoints[npoints] = x;
        ypoints[npoints] = y;
        npoints++;
        if (bounds != null) {
            updateBounds(x, y);
        }
    }

    /**
     * Gets the bounding box of this polygon. The bounding box is the
     * smallest rectangle whose sides are parallel to the <i>x</i> and
     * <i>y</i> axes of the coordinate space, and that can completely
     * contain the polygon.
     * @return      a rectangle that defines the bounds of this polygon.
     * @since       JDK1.1
     */
    public Rectangle getBounds() {
        if (npoints == 0) {
            return new Rectangle();
        }        
        if (bounds == null) {
            calculateBounds(xpoints, ypoints, npoints);
        }
        return bounds;
    }

    /**
     * Determines whether the specified point is inside the Polygon.
     * Uses an even-odd insideness rule (also known as an alternating
     * rule).
     * @param p the point to be tested
     */
    public boolean contains(Point p) {
        return contains(p.x, p.y);
    }

    /**
     * Determines whether the specified point is contained by this polygon.   
     * @param      x  the <i>x</i> coordinate of the point to be tested.
     * @param      y  the <i>y</i> coordinate of the point to be tested.
     * @return     <code>true</code> if the point (<i>x</i>,&nbsp;<i>y</i>) 
     *                       is contained by this polygon; 
     *                       <code>false</code> otherwise.
     * @since      JDK1.1
     */
    public boolean contains(int x, int y) {
        return contains((double) x, (double) y);    
    }
    
    private boolean contains(double x, double y) {
        if (npoints <= 2 || !getBounds().contains(x, y)) {
            return false;
        }
        int hits = 0;
        int lastx = xpoints[npoints - 1];
        int lasty = ypoints[npoints - 1];
        int curx, cury;
        // Walk the edges of the polygon
        for (int i = 0; i < npoints; lastx = curx, lasty = cury, i++) {
            curx = xpoints[i];
            cury = ypoints[i];
            if (cury == lasty) {
                continue;
            }
            int leftx;
            if (curx < lastx) {
                if (x >= lastx) {
                    continue;
                }
                leftx = curx;
            } else {
                if (x >= curx) {
                    continue;
                }
                leftx = lastx;
            }
            double test1, test2;
            if (cury < lasty) {
                if (y < cury || y >= lasty) {
                    continue;
                }
                if (x < leftx) {
                    hits++;
                    continue;
                }
                test1 = x - curx;
                test2 = y - cury;
            } else {
                if (y < lasty || y >= cury) {
                    continue;
                }
                if (x < leftx) {
                    hits++;
                    continue;
                }
                test1 = x - lastx;
                test2 = y - lasty;
            }
            if (test1 < (test2 / (lasty - cury) * (lastx - curx))) {
                hits++;
            }
        }
        return ((hits & 1) != 0);
    }    
}
