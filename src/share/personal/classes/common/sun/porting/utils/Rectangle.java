/*
 * @(#)Rectangle.java	1.11 06/10/10
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

/**
 * An extension of java.awt.Rectangle.  There is a change to the behavior
 * of addition: we treat all rectangles with width <= 0 or height <= 0 as
 * empty.  For some reason, java.awt.Rectangle does not.
 *
 * @version 1.7, 08/19/02
 * @author Richard Berlin
 */
public class Rectangle extends java.awt.Rectangle {
    public Rectangle() {}

    public Rectangle(int x, int y, int width, int height) {
        setRectangle(x, y, width, height);
    }

    public Rectangle(java.awt.Rectangle r) {
        setRectangle(r);
    }

    public void add(java.awt.Rectangle r) {
        if (r.width <= 0 || r.height <= 0) {
            return;
        }
        if ((this.width <= 0) || (this.height <= 0)) {
            setRectangle(r);
            return;
        }
        super.add(r);
    }

    public void add(int x, int y, int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        if (this.width <= 0 || this.height <= 0) {
            this.x = x;
            this.y = y;
            this.width = width;
            this.height = height;
        } else {
            int x1 = Math.min(this.x, x);
            int y1 = Math.min(this.y, y);
            int x2 = Math.max(this.x + this.width, x + width);
            int y2 = Math.max(this.y + this.height, y + height);
            this.x = x1;
            this.y = y1;
            this.width = x2 - x1;
            this.height = y2 - y1;
        }
    }

    public boolean intersects(int rx, int ry, int rwidth, int rheight) {
        return !((rx + rwidth <= x) ||
                (ry + rheight <= y) ||
                (rx >= x + width) ||
                (ry >= y + height));
    }

    public boolean contains(int rx, int ry, int rw, int rh) {
        return ((rx >= x) && (ry >= y)
                && ((x + width) >= (rx + rw)) && ((y + height) >= (ry + rh)));
    }

    public boolean contains(java.awt.Rectangle r) {
        return contains(r.x, r.y, r.width, r.height);
    }

    // Destructively intersect with the current rectangle.
    public void intersect(int rx, int ry, int rwidth, int rheight) {
        if (isEmpty()) {
            return;
        }
        int x2 = Math.min(x + width, rx + rwidth);
        int y2 = Math.min(y + height, ry + rheight);
        x = Math.max(x, rx);
        y = Math.max(y, ry);
        width = x2 - x;
        height = y2 - y;
    }

    // Destructively intersect r with the current rectangle.
    public void intersect(java.awt.Rectangle r) {
        if (r.isEmpty()) {
            setEmpty();
            return;
        }
        intersect(r.x, r.y, r.width, r.height);
    }

    public void subtract(int rx, int ry, int rw, int rh) {
        if (isEmpty() || (rw == 0) || (rh == 0)) {
            return;
        }
        if ((rx <= x) && ((rx + rw) >= (x + width))) {
            // completely covers top or bottom edge
            if (ry <= y) {
                height += y;
                y = ry + rh;
                height -= y;
            } else if ((ry + rh) >= (y + height)) {
                height = ry - y;
            }
        }
        if (height <= 0) {
            setEmpty();
            return;
        }
        if ((ry <= y) && ((ry + rh) >= (y + height))) {
            // completely covers left or right edge
            if (rx <= x) {
                width += x;
                x = rx + rw;
                width -= x;
            } else if ((rx + rw) >= (x + width)) {
                width = rx - x;
            }
        }
        if (width <= 0) {
            setEmpty();
        }
    }

    // destructively subtract r from the current rectangle. As with
    // the add operation, the result is the bounding box of the true answer,
    // i.e. the smallest rectangle that encloses (this - r).
    public void subtract(java.awt.Rectangle r) {
        subtract(r.x, r.y, r.width, r.height);
    }

    public void setEmpty() {
        width = 0;
        height = 0;
    }

    public void setRectangle(int x, int y, int width, int height) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }

    public void setRectangle(java.awt.Rectangle r) {
        x = r.x;
        y = r.y;
        width = r.width;
        height = r.height;
    }
}
