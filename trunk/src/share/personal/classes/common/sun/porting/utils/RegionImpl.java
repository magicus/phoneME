/*
 * @(#)RegionImpl.java	1.13 06/10/10
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
package sun.porting.utils;

import sun.porting.graphicssystem.Region;
import java.util.Enumeration;

/**
 * Region manipulation code.  This code attempts to avoid using full-fledged
 * Y-X bands if one or both of the regions being manipulated are single
 * rectangles.
 *
 * @version 1.9, 08/19/02
 * @author Richard Berlin
 */
public class RegionImpl extends YXBandList implements Region {
    // static {
    // Coverage.cover(10, "(case deleted)");
    // }

    /*
     * To avoid Y-X bands when possible, and to help speed up overlap
     * testing, we maintain the bounding box of the region.
     */
    protected Rectangle boundingBox;
    public RegionImpl() {
        // // Coverage.cover(1, "Default Constructor Region()");
        boundingBox = new Rectangle(0, 0, 0, 0);
    }

    public RegionImpl(int x, int y, int w, int h) {
        // // Coverage.cover(2, "Constructor Region(x,y,w,h)");
        boundingBox = new Rectangle(x, y, w, h);
    }

    // deep copy of a Region
    public Region copy() {
        // // Coverage.cover(3, "Region copy()");
        RegionImpl ret = new RegionImpl(boundingBox.x, boundingBox.y,
                boundingBox.width, boundingBox.height);
        if (bandList != null) {
            // // Coverage.cover(4, "bandList != null");
            ret.copyBands(this);
        }
        return ret;
    }

    public java.awt.Rectangle getBounds() {
        if (bandList == null) {
            return new Rectangle(boundingBox);
        } else {
            return super.getBounds();
        }
    }

    public boolean equals(Region reg) {
        RegionImpl r = (RegionImpl) reg;
        if (!boundingBox.equals(r.boundingBox)) {
            return false;
        }
        if ((bandList == null) && (r.bandList == null)) {
            return true;
        }
        if (bandList == null) {
            YXBand first = r.bandList.next;
            YXBand last = r.bandList.prev;
            // Implies also that first == last
            return first.children.next == last.children.prev;
        }
        if (r.bandList == null) {
            YXBand first = bandList.next;
            YXBand last = bandList.prev;
            // Implies also that first == last
            return first.children.next == last.children.prev;
        }
        return super.equals(r);
    }

    // simple overlap tests on bounding boxes, hence only tells us
    // whether regions *may* intersect
    public boolean mayIntersect(Region r) {
        // // Coverage.cover(5, "mayIntersect(Region)");
        return (r != null) 
            && boundingBox.intersects(((RegionImpl) r).boundingBox);
    }

    // simple overlap tests on bounding boxes, hence only tells us
    // whether regions *may* intersect
    public boolean mayIntersect(java.awt.Rectangle r) {
        // // Coverage.cover(6, "mayIntersect(Rectangle)");
        return (r != null) && boundingBox.intersects(r);
    }

    // simple overlap tests on bounding boxes, hence only tells us
    // whether regions *may* intersect
    public boolean mayIntersect(int x, int y, int w, int h) {
        // // Coverage.cover(7, "mayIntersect(x,y,w,h)");
        return boundingBox.intersects(x, y, w, h);
    }

    public boolean isRectangle() {
        if (bandList == null) {
            return true;
        }
        return super.isRectangle();
    }

    public void defragment(boolean isClip) {
        if (bandList == null) {
            if (isClip) {
                // force a band list to be created.
                init(boundingBox.x, boundingBox.y, 
                    boundingBox.width, boundingBox.height);
                // now make sure it's legal
                if (bandList.next == bandList) {
                    bandList = null;
                }
            }
        } else {
            super.deFragment();
        }
    }

    // destructively union two regions
    public void union(Region reg) {
        RegionImpl r = (RegionImpl) reg;
        if (r.isEmpty()) {
            // Coverage.cover(1, "union with empty region");
            return;
        } else if (r.bandList == null) {
            // Coverage.cover(2, "union with region that has null bandlist");
            union(r.boundingBox);
            return;
        } 
        if (bandList == null) {
            if (boundingBox.contains(r.boundingBox)) {
                // Coverage.cover(9, "region is rectangle which subsumes r");
                return;
            }
            // Coverage.cover(3, "initialize bandList");
            super.init(boundingBox.x, boundingBox.y,
                boundingBox.width, boundingBox.height);
        }
        boundingBox.add(r.boundingBox);
        super.union(r.bandList);
    }

    public void union(int x, int y, int w, int h) {
        if ((w <= 0) || (h <= 0)) {
            // Coverage.cover(4, "union with empty area");
            return;
        }
        union(new Rectangle(x, y, w, h));
    }

    // destructively union a rectangle with a region
    public void union(java.awt.Rectangle r) {
        if (r.isEmpty()) {
            // Coverage.cover(5, "union with empty rectangle");
            return;
        } else if (boundingBox.isEmpty()) {
            boundingBox = new Rectangle(r);
            return;
        }
        if (r.contains(boundingBox.x, boundingBox.y)
            && r.contains(boundingBox.x + boundingBox.width, 
                boundingBox.y + boundingBox.height)) {
            // Coverage.cover(6, "union with rectangle that subsumes all");
            bandList = null;
            if (r instanceof Rectangle) {
                boundingBox = (Rectangle) r;
            } else {
                boundingBox = new Rectangle(r);
            }
            return;
        }
        if (bandList == null) {
            if (boundingBox.contains(r)) { // r inside boundingBox
                // Coverage.cover(7, "region is rectangle which subsumes r");
                return;
            }
            // Coverage.cover(8, "initialize bandList");
            super.init(boundingBox.x, boundingBox.y,
                boundingBox.width, boundingBox.height);
        }
        boundingBox.add(r);
        super.union(r.x, r.y, r.width, r.height);
    }

    // destructively subtract two regions
    public void subtract(Region reg) {
        RegionImpl r = (RegionImpl) reg;
        // // Coverage.cover(8, "subtract(Region)");
        if (boundingBox.isEmpty()
            || (r == null) || r.boundingBox.isEmpty() 
            || !boundingBox.intersects(r.boundingBox)) {
            // // Coverage.cover(9, "empty or nonintersecting bounding box");
            return;
        }
        if (bandList == null) {
            // // Coverage.cover(10, "bandList == null");
            super.init(boundingBox.x, boundingBox.y,
                boundingBox.width, boundingBox.height);
        }
        if (r.bandList == null) {
            // // Coverage.cover(11, "r.bandList == null");
            super.subtract(r.boundingBox.x, r.boundingBox.y,
                r.boundingBox.width, r.boundingBox.height);
        } else {
            // // Coverage.cover(12, "subtract band lists");
            super.subtract(r.bandList);
        }
        boundingBox = (Rectangle) getBounds();
        if ((boundingBox.width | boundingBox.height) == 0) {
            // // Coverage.cover(13, "result has empty bounding box");
            bandList = null;
        }
    }

    public void subtract(java.awt.Rectangle r) {
        subtract(r.x, r.y, r.width, r.height);
    }

    // destructively subtract a rectangle from a region
    public void subtract(int x, int y, int w, int h) {
        // // Coverage.cover(14, "subtract(x,y,w,h)");
        if (((w <= 0) || (h <= 0)) 
            || !boundingBox.intersects(x, y, w, h)) {
            // // Coverage.cover(15, "Null size or no intersection");
            return;
        }
        boundingBox.subtract(x, y, w, h);
        if ((boundingBox.width | boundingBox.height) == 0) {
            // // Coverage.cover(16, "Bounding box subtraction yields empty rectangle");
            bandList = null;
            return;
        } 
        if (bandList == null) {
            // // Coverage.cover(17, "bandList == null");
            super.init(boundingBox.x, boundingBox.y,
                boundingBox.width, boundingBox.height);
        }
        super.subtract(x, y, w, h);
        boundingBox = (Rectangle) getBounds();
        if ((boundingBox.width | boundingBox.height) == 0) {
            // // Coverage.cover(18, "boundingBox is empty");
            bandList = null;
        }
    }

    // destructively intersect two regions
    public void intersect(Region reg) {
        RegionImpl r = (RegionImpl) reg;
        // // Coverage.cover(19, "intersect(Region)");
        if (boundingBox.isEmpty()
            || (r == null) || r.boundingBox.isEmpty()
            || !boundingBox.intersects(r.boundingBox)) {
            // // Coverage.cover(20, "empty or nonintersecting bounding box");
            setEmpty();
            return;
        }
        if (bandList == null) {
            if (r.bandList == null) {
                // // Coverage.cover(21, "Both band lists empty");
                boundingBox.intersect(r.boundingBox);
            } else {
                // // Coverage.cover(22, "bandList empty");
                super.init(boundingBox.x, boundingBox.y,
                    boundingBox.width, boundingBox.height);
                int x1 = Math.max(boundingBox.x, r.boundingBox.x);
                int x2 = Math.min(boundingBox.x + boundingBox.width,
                        r.boundingBox.x + r.boundingBox.width);
                super.intersect(r.bandList, x1, x2);
                boundingBox = (Rectangle) getBounds();
            }
        } else {
            if (r.bandList == null) {
                // // Coverage.cover(23, "r.bandList empty");
                super.intersect(r.boundingBox.x, r.boundingBox.y,
                    r.boundingBox.width, r.boundingBox.height);
            } else {
                // // Coverage.cover(24, "intersecting 2 band lists");
                int x1 = Math.max(boundingBox.x, r.boundingBox.x);
                int x2 = Math.min(boundingBox.x + boundingBox.width,
                        r.boundingBox.x + r.boundingBox.width);
                super.intersect(r.bandList, x1, x2);
            }
            boundingBox = (Rectangle) getBounds();
        }
        if ((boundingBox.width | boundingBox.height) == 0) {
            // // Coverage.cover(25, "bounding box empty");
            bandList = null;
        }
    }

    public void intersect(java.awt.Rectangle r) {
        intersect(r.x, r.y, r.width, r.height);
    }

    // destructively intersect a rectangle with a region
    public void intersect(int x, int y, int w, int h) {
        // // Coverage.cover(26, "intersect(x,y,w,h)");
        if (((w <= 0) || (h <= 0)) 
            || !boundingBox.intersects(x, y, w, h)) {
            // // Coverage.cover(27, "Null size or no intersection");
            setEmpty();
            return;
        }
        if (bandList != null) {
            // // Coverage.cover(28, "intersect with a band list");
            super.intersect(x, y, w, h);
            boundingBox = (Rectangle) getBounds();
        } else {
            // // Coverage.cover(29, "intersect bounding boxes");
            boundingBox.intersect(x, y, w, h);
        }
        if ((boundingBox.width | boundingBox.height) == 0) {
            // // Coverage.cover(30, "bounding box empty");
            bandList = null;
        }
    }

    // return true if the point is in the region
    public boolean contains(int x, int y) {
        // // Coverage.cover(31, "contains(x,y)");
        if (boundingBox.contains(x, y, 1, 1)) {
            // // Coverage.cover(32, "bounding box check");
            if (bandList == null) {
                // // Coverage.cover(33, "null bandList");
                return true;    // region is rectangular
            } else {
                // // Coverage.cover(34, "checking band list");
                return super.contains(x, y, 1, 1);
            }
        }
        return false;
    }

    // return true if the rectangle is in the region
    public boolean contains(int x, int y, int w, int h) {
        // // Coverage.cover(35, "contains(x,y,w,h)");
        if (boundingBox.contains(x, y, w, h)) {
            // // Coverage.cover(36, "bounding box check");
            if (bandList == null) {
                // // Coverage.cover(37, "null bandList");
                return true;
            } else {
                // // Coverage.cover(38, "checking band list");
                return super.contains(x, y, w, h);
            }
        }
        return false;
    }

    // translate the region by dx,dy
    public void translate(int dx, int dy) {
        // // Coverage.cover(39, "translate(dx,dy)");
        boundingBox.translate(dx, dy);
        if ((bandList != null) && ((dx | dy) != 0)) {
            // // Coverage.cover(40, "translating bands");
            super.translate(dx, dy);
        }
    }

    public void setEmpty() {
        // // Coverage.cover(41, "setEmpty()");
        boundingBox.width = 0;
        boundingBox.height = 0;
        bandList = null;
    }

    public boolean isEmpty() {
        // // Coverage.cover(42, "isEmpty()");
        return ((boundingBox.width | boundingBox.height) == 0);
    }

    // We don't want test coverage
    public String toString() {
        if (bandList == null) {
            if (boundingBox.isEmpty()) {
                return "[Region: (empty)]";
            } else {
                return boundingBox.toString();
            }
        } else {
            return ("[Region: " + super.toString() + "]");
        }
    }

    public Enumeration getRectangles() {
        return new RectangleEnumerator(this);
    }
}

class RectangleEnumerator implements Enumeration {
    YXBand yPtr, yHead;
    XSpan  xPtr, xHead;
    java.awt.Rectangle bbox;
    RectangleEnumerator(Region r) {
        yHead = ((RegionImpl) r).bandList;
        if (yHead == null) {
            bbox = r.getBounds();
        } else {
            yPtr = yHead.next;
            xHead = yPtr.children;
            xPtr = xHead;
        }
    }

    public boolean hasMoreElements() {
        if (yHead == null) {
            return (bbox != null);
        } else {
            return (yPtr.next != yHead) || (xPtr.next != xHead);
        }
    }

    public Object nextElement() {
        if (yHead == null) {
            java.awt.Rectangle r = bbox;
            bbox = null;
            return r;
        }
        xPtr = xPtr.next;
        if (xPtr == xHead) {
            yPtr = yPtr.next;
            if (yPtr == yHead) {
                return null;
            }
            xHead = yPtr.children;
            xPtr = xHead.next;
        }
        return new Rectangle(xPtr.x, yPtr.y, xPtr.w, yPtr.h);
    }
}
