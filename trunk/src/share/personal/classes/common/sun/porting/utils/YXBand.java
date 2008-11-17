/*
 * @(#)YXBand.java	1.10 06/10/10
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
 * A class which stores a single Y/X band.
 *
 * @version 1.5, 08/19/02
 */
class YXBand {
    int y, h;
    YXBand next, prev;
    XSpan children;
    public YXBand(int x, int y, int w, int h, YXBand prev, YXBand next) {
        // // Coverage.cover(108, "YXBand(x,y,w,h,prev,next)");
        this.y = y;
        this.h = h;
        this.children = new XSpan(0, -1);
        this.children.next = new XSpan(x, w, this.children, this.children);
        this.children.prev = this.children.next;
        this.prev = prev;
        this.next = next;
    }

    public YXBand(int y, int h, XSpan children) {
        this(y, h, children, null, null);
        // // Coverage.cover(109, "YXBand(y,h,children)");
    }

    public YXBand(int y, int h, XSpan children, YXBand prev, YXBand next) {
        // // Coverage.cover(110, "YXBand(y,h,children,prev,next)");
        this.y = y;
        this.h = h;
        this.children = children;
        this.prev = prev;
        this.next = next;
    }

    static YXBand findBand(int y, int y2, YXBand head) {
        // // Coverage.cover(111, "findBand(y,y2,head)");
        YXBand yb;
        for (yb = head.next; yb != head; yb = yb.next) {
            if (yb.y >= y2) {
                // // Coverage.cover(112, "ran off list");
                return null;
            } else if ((yb.y + yb.h) > y) {
                // // Coverage.cover(113, "found band");
                break;
            }
        }
        return (yb == head) ? null : yb;
    }

    // no test coverage for makeString
    public static String makeString(YXBand head) {
        String ret = "[";
        for (YXBand yb = head.next; yb != head; yb = yb.next) {
            if (yb != head.next) {
                ret += " ";
            }
            String s = "[(" + yb.y + "," + yb.h + "): " + 
                XSpan.makeString(yb.children) + "]";
            ret += s;
        }
        return ret + "]";
    }

    // no test coverage for toString
    public String toString() {
        return "(y = " + y + ", h = " + h + ")";
    }
}
