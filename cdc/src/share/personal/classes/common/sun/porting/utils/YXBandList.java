/*
 * @(#)YXBandList.java	1.13 06/10/10
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
 * Implementation of Y-X bounded rectangles.  The band lists are
 * circular and doubly-linked, with a dummy element separating the
 * head from the tail; this makes insertion and deletion extremely cheap.
 *
 * @version 1.9, 08/19/02
 * @author Richard Berlin
 */
class YXBandList {
    YXBand bandList;
    // static {
    // Coverage.cover(17, "(removed)");
    // }

    // static {
    // mark these covered, (a) so that they don't show as gaps, and
    // (b) so we get an error message if they ever get called.
    // // Coverage.cover(44, "(error case)");
    // // Coverage.cover(47, "(error case)");
    // // Coverage.cover(132, "(error case)");
    // // Coverage.cover(135, "(error case)");
    // }

    public YXBandList() {
        // // Coverage.cover(43, "YXBandList default constructor");
        bandList = null;
    }

    public YXBandList(int x, int y, int w, int h) {
        // // Coverage.cover(44, "YXBandList(x,y,w,h)");
        if ((w > 0) && (h > 0)) {
            init(x, y, w, h);
        }
    }

    public void init(int x, int y, int w, int h) {
        if ((w < 0) || (h < 0)) {
            throw new IllegalArgumentException("Width or height is < 0");
        }
        // // Coverage.cover(45, "init(x,y,w,h)");
        bandList = new YXBand(0, -1, null); // dummy element
        if ((w > 0) && (h > 0)) {
            bandList.next = new YXBand(x, y, w, h, bandList, bandList);
            bandList.prev = bandList.next;
        } else {
            bandList.next = bandList.prev = bandList;
        }
    }

    /*
     * Circular, doubly-linked lists are central to the implementation.
     *
     * The idiom for creating a new list is to start by building the
     * following structure, in which a dummy element and the real
     * first element have their prev and next pointers all pointing at
     * each other.  In this example we're making an initial X band
     * which goes from 10 to 18.  Dummy elements are usually set with
     * their start as 0 and their span as negative.
     * 
     *                -------- 
     *               |x = 0   |
     *      head --->|w = -1  |
     *               |next = -+---+
     *           +---+- = prev|   |
     *           |   |        |   |
     *           |    --------    |
     *           |     ^   ^      |
     *           |     |   |      |
     *           | +---+   +----+ |
     *           | |            | |
     *           | |  --------  | |
     *           | | |x = 10  | | |
     *           | | |w =  8  | | |
     *           | | |next = -+-+ |
     *           | +-+- = prev|   |
     *           +-->|        |<--+
     *                -------- 
     *      
     * Once this structure is set up, appending to the list is a general
     * and very simple insertion before the dummy head element and after
     * the last real element.  The code for appending a new element which
     * extends from 21 to 24 is
     *      
     *      head.prev.next = head.prev = new XSpan(21, 3, head.prev, head);
     *
     * In the first stage of execution (calling the constructor)
     * a new element is created and its links correctly set:
     * 
     *                -------- 
     *               |x = 0   |
     *      head --->|w = -1  |<--------------------+
     *               |next = -+---+                 |
     *           +---+- = prev|   |                 |
     *           |   |        |   |                 |
     *           |    --------    |                 |
     *           |     ^   ^      |                 |
     *           |     |   |      |                 |
     *           | +---+   +----+ |                 |
     *           | |            | |                 |
     *           | |  --------  | |      --------   |
     *           | | |x = 10  | | |     |x = 21  |  |
     *           | | |w =  8  | | |     |w = 3   |  |
     *           | | |next = -+-+ |     |next = -+--+
     *           | +-+- = prev|   |   +-+- = prev|
     *           +-->|        |<--+   | |        |
     *                --------        |  -------- 
     *                  ^             |
     *                  +-------------+
     *
     * and in the second phase (the assigments) the pointers are adjusted:
     *
     *                -------- 
     *               |x = 0   |
     *      head --->|w = -1  |<--------------------+
     *           +---+- = next|                     |
     *           |   |prev = -+----------------+    |
     *           |   |        |                |    |
     *           |    --------                 |    |
     *           |     ^                       |    |
     *           |     |                       |    |
     *           | +---+                       |    |
     *           | |                           V    |
     *           | |  --------           --------   |
     *           | | |x = 10  |         |x = 21  |  |
     *           | | |w =  8  |         |w = 3   |  |
     *           | | |next = -+-------->|next = -+--+
     *           | +-+- = prev|<--------+- = prev|
     *           +-->|        |         |        |
     *                --------           -------- 
     *
     * Analogous code will work for inserting between any two
     * elements of the list.  For example, to prepend a band
     * running from 3 to 5 we insert after the head and before the
     * first real element:
     *
     *      head.next.prev = head.next = new XSpan(3, 2, head, head.next); 
     */
     
    /*
     * Simple copy of the elements of a doubly-linked list.
     */
    protected void copyBands(YXBandList src) {
        // // Coverage.cover(46, "copyBands(YXBandList)");
        if (src.bandList == null) {
            // // Coverage.cover(47, "source bandlist is null");
            bandList = null;
            return;
        } else {
            // // Coverage.cover(48, "create dummy head");
            bandList = new YXBand(0, -1, null);
        }
        YXBand q = bandList;    // dummy element
        // // Coverage.cover(49, "Copy first element");
        // copy first real element
        YXBand p = src.bandList.next;
        q.next = new YXBand(p.y, p.h, XSpan.copy(p.children), q, q);
        q.prev = q.next;
        // copy remainder of list
        for (p = p.next; p != src.bandList; p = p.next) {
            // // Coverage.cover(50, "Copy remainder");
            q.prev = q.prev.next = 
                            new YXBand(p.y, p.h, XSpan.copy(p.children), q.prev, q);
        }
    }

    public boolean isRectangle() {
        if (bandList.next == bandList.prev) {
            XSpan kids = bandList.next.children;
            return (kids.next == kids.prev);
        }
        return false;
    }

    /*
     * Subtract a rectangle from a region.  This is done in 3 stages:
     *   1) If the first y band in the range overlaps the edge
     *      of the rectangle, and some of its x spans extend outside
     *      the rectangle, split the first band; if y overlaps but
     *      x does not, just shorten the band.
     *   2) clear out the x range of any band wholly within the y range.
     *      if all spans are within the rectangle for a given band, delete
     *      the entire band.
     *   3) If the last y band overlaps the edge, perform an operation
     *      analogous to step 1.
     */
    protected void subtract(int x, int y, int w, int h) {
        if ((w < 0) || (h < 0)) {
            throw new IllegalArgumentException("Width or height is < 0");
        }
        // // Coverage.cover(51, "subtract(x,y,w,h)");
        int x2 = x + w;
        int y2 = y + h;
        YXBand yb = YXBand.findBand(y, y2, bandList);
        if (yb == null) {
            // // Coverage.cover(52, "no pertinent bands found");
            return;
        }
        // yb now points to the first relevant band
        if (yb.y < y) {
            // // Coverage.cover(53, "yb < y");
            int yEnd = yb.y + yb.h;
            // first band crosses the rectangle--we may have to split bands
            XSpan oldKids = yb.children;
            XSpan newKids = XSpan.copyOutside(x, x2, yb.children);
            if (newKids == null) {
                // // Coverage.cover(54, "newKids == null (shorten band)");
                // the newly split-off band would have no children, so
                // we can just shorten this band
                yb.h = y - yb.y;
                if (yEnd <= y2) {
                    yb = yb.next;
                }
            } else {
                // // Coverage.cover(55, "newKids != null (split band)");
                // do the split.
                // --create a new band from y..(yb.y + yb.h)
                int newEnd = yEnd;
                if (yEnd > y2) {
                    // // Coverage.cover(56, "decrease newEnd");
                    newEnd = y2;
                }
                yb.next = yb.next.prev =
                                new YXBand(y, newEnd - y, newKids, yb, yb.next);
                yb.h = y - yb.y;
                yb = yb.next;   
            }
            if (yEnd > y2) {
                // // Coverage.cover(57, "yEnd > y2");
                yb.next = yb.next.prev =
                                new YXBand(y2, yEnd - y2, XSpan.copy(oldKids),
                                    yb, yb.next);
                return;
            }
        }
        while (yb != bandList) {
            if (yb.y >= y2) {
                // // Coverage.cover(58, "yb.y > y2 (exit)");
                return;
            }
            int yEnd = yb.y + yb.h;
            XSpan prev = yb.children.prev;
            if ((yb.children.next.x >= x) && ((prev.x + prev.w) <= x2)) {
                if (yEnd > y2) {
                    // // Coverage.cover(59, "yEnd > y2 (shorten last band)");
                    // we can just shorten the last band to y2..(yb.y + yb.h)
                    yb.y = y2;
                    yb.h = yEnd - y2;
                    return;
                } else {
                    // // Coverage.cover(60, "delete last band");
                    // delete this band
                    yb.next.prev = yb.prev;
                    yb.prev.next = yb.next;
                    yb = yb.next;
                }
            } else {
                if (yEnd > y2) {
                    // // Coverage.cover(61, "yEnd > y2 (split bands)");
                    // last band must be split
                    XSpan newKids = XSpan.copyOutside(x, x2, yb.children);
                    if (newKids != null) {
                        yb.prev = yb.prev.next = 
                                        new YXBand(yb.y, y2 - yb.y, newKids, yb.prev, yb);
                    }
                    yb.y = y2;
                    yb.h = yEnd - y2;
                    return;
                } else {
                    // // Coverage.cover(62, "call deleteInside()");
                    XSpan.deleteInside(x, x2, yb.children);
                    yb = yb.next;
                }
            }
        }
    }

    /*
     * Destructively intersect a rectangle from a region.  
     * This is done in several steps:
     *   1) Delete y bands that end above the rectangle.
     *   2) If the first y band in the range overlaps the edge
     *      of the rectangle, shorten it.
     *   3) For each band, remove spans outside the rectangle.  If this
     *      is all spans, delete the band.
     *   4) If the last y band overlaps the edge, shorten it.
     *   5) Delete bands that begin below the rectangle.
     */
    protected void intersect(int x, int y, int w, int h) {
        if ((w < 0) || (h < 0)) {
            throw new IllegalArgumentException("Width or height is < 0");
        }
        // // Coverage.cover(63, "intersect(x,y,w,h)");
        int x2 = x + w;
        int y2 = y + h;
        YXBand yb = YXBand.findBand(y, y2, bandList);
        if (yb == null) {
            // // Coverage.cover(64, "no pertinent bands (null out list and return)");
            bandList.next = bandList;
            bandList.prev = bandList;
            return;
        }
        // yb now points to the first relevant band
        // delete anything in front of it
        yb.prev = bandList;
        bandList.next = yb;
        // shorten the first band if necessary
        int yEnd = yb.y + yb.h;
        if (y2 < yEnd) {
            // // Coverage.cover(65, "decrease yEnd");
            yEnd = y2;
        }
        if (y > yb.y) {
            // // Coverage.cover(66, "increase yb.y");
            yb.y = y;
        }
        yb.h = yEnd - yb.y;
        // trim the x spans of each relevant band
        while ((yb != bandList) && (yb.y < y2)) {
            XSpan prv = yb.children.prev;
            if ((yb.children.next.x < x) || ((prv.x + prv.w) > x2)) {
                // // Coverage.cover(67, "call deleteOutside()");
                XSpan.deleteOutside(x, x2, yb.children);
                if (yb.children.next == yb.children) {
                    // // Coverage.cover(68, "delete empty band");
                    // delete empty band
                    yb.prev.next = yb.next;
                    yb.next.prev = yb.prev;
                }
            }
            yb = yb.next;
        }
        // yb points past the last relevant band; back it up one.
        // then shorten the last band if necessary.
        yb = yb.prev;
        yEnd = y2 - yb.y;
        if (yEnd < yb.h) {
            // // Coverage.cover(69, "decrease yb.h");
            yb.h = yEnd;
        }
        // Delete anything beyond the last relevant band.
        yb.next = bandList;
        bandList.prev = yb;
    }

    /*
     * Destructively union a rectangle into a region.  This is a relatively
     * simple operation requiring creation of new bands wherever a band does
     * not exist in the range y..y+h.  Where bands do exist, an XSpan must be
     * merged in to cover the range x..x+w.  Only two possible split bands are
     * required: if the first overlapping band starts before y, and if the
     * last overlapping band extends beyond y+h;
     */
    protected void union(int x, int y, int w, int h) {
        if ((w < 0) || (h < 0)) {
            throw new IllegalArgumentException("Width or height is < 0");
        }
        int yEnd = y + h;
        // find first overlapping band
        YXBand yb;
        for (yb = bandList.next; yb != bandList; yb = yb.next) {
            if ((yb.y + yb.h) > y) {
                break;
            }
        }
        if ((yb != bandList) && (y > yb.y)) {
            // split bands, and operate (below) on the second one
            // Coverage.cover(11, "split bands");
            yb.next = yb.next.prev =
                            new YXBand(y, yb.y + yb.h - y,
                                XSpan.copy(yb.children), yb, yb.next);
            yb.h = y - yb.y;
            yb = yb.next;
        }
        while ((yb != bandList) && (yb.y < yEnd)) {
            if (yb.y > y) {
                // Coverage.cover(12, "insert band");
                // insert a band from x,y to x+h,yb.y
                yb.prev = yb.prev.next =
                                new YXBand(x, y, w, yb.y - y, yb.prev, yb);
            }
            y = yb.y + yb.h;
            if (y > yEnd) {
                // Coverage.cover(13, "split again");
                // split bands, and operate (below) on the first one
                yb.next = yb.next.prev =
                                new YXBand(yEnd, y - yEnd, 
                                    XSpan.copy(yb.children), yb, yb.next);
                yb.h = yEnd - yb.y;
            }
            // now insert XSpan to cover x,yb.y to x+h,yb.y+yb.h
            XSpan.cover(yb.children, x, w);
            yb = yb.next;
        };
        if (y < yEnd) {
            // Coverage.cover(14, "handle last band");
            // yb is the first band that comes after yEnd
            // insert a band (in front) from x,y to x+h,yEnd
            yb.prev = yb.prev.next =
                            new YXBand(x, y, w, yEnd - y, yb.prev, yb);
        }
    }

    /*
     * Operations on a pair of regions are done by case analysis.  There
     * are eleven cases:
     *
     *       1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10  |  11
     * A:  --       -- ---   ----  ----- --     ---  ----   ---   ---    ---  
     * B:     -- --      ---   --   ---  ----   ---  ---   ---   ----   -----
     *
     * The first two cases (no overlap) are typically handled specially--
     * bands are either skipped or deleted.
     *
     * Cases 3-5 are reduced to cases 6-8 by shortening or splitting bands.
     * Case 8 is further reducible to case 7 and case 9 reducible to case
     * 10 by shortening or splitting.
     *
     * For subtraction and intersection, the only difference in handling
     * cases 6, 7, 10, 11 is whether band b has to be re-examined.
     */


    /*
     * destructively subtract two band structures
     */
    protected void subtract(YXBand l) {
        // // Coverage.cover(70, "subtract(YXBand)");
        YXBand a = bandList.next;
        YXBand b = l.next;
        while ((a != bandList) && (b != l)) {
            int aEnd = a.y + a.h;
            if (aEnd <= b.y) {
                // // Coverage.cover(71, "no overlap (skip a)");
                // no overlap -- skip
                a = a.next;
                continue;
            }
            int bEnd = b.y + b.h;
            if (bEnd <= a.y) {
                // // Coverage.cover(72, "no overlap (skip b)");
                // no overlap -- skip
                b = b.next;
                continue;
            } 
            if ((a.y == b.y) && (aEnd == bEnd)) {
                // // Coverage.cover(73, "y bands are equal - subtract kids");
                // special case this because it's so common
                XSpan.subtract(a.children, b.children);
                if (a.children.next == a.children) {
                    // // Coverage.cover(74, "delete empty band");
                    // band is empty--delete it
                    a.prev.next = a.next;
                    a.next.prev = a.prev;
                }
                a = a.next;
                b = b.next;
                continue;
            }
            XSpan newKids;
            if (a.y < b.y) {
                // // Coverage.cover(75, "a.y < b.y (trim and split bands)");
                // keep the non-overlap area preceding the overlap
                newKids = XSpan.copy(a.children);
                a.next = a.next.prev =
                                new YXBand(b.y, aEnd - b.y, newKids, a, a.next);
                a.h = b.y - a.y;
                a = a.next;
            }
            if (aEnd > bEnd) {
                // // Coverage.cover(76, "aEnd > bEnd (trim and split bands)");
                // keep the non-overlap area following the overlap
                newKids = XSpan.copy(a.children);
                a.next = a.next.prev =
                                new YXBand(bEnd, aEnd - bEnd, newKids, a, a.next);
                a.h = bEnd - a.y;
            }
            XSpan.subtract(a.children, b.children);
            if (a.children.next == a.children) {
                // // Coverage.cover(77, "delete empty band");
                // band is empty--delete it
                a.prev.next = a.next;
                a.next.prev = a.prev;
            }
            a = a.next;
        }
    }

    /*
     * destructively intersect two band structures
     */
    protected void intersect(YXBand l, int x1, int x2) {
        // // Coverage.cover(78, "intersect(YXBand, x1, x2)");
        YXBand a = bandList.next;
        YXBand b = l.next;
        while ((a != bandList) && (b != l)) {
            int aEnd = a.y + a.h;
            if (aEnd <= b.y) {
                // // Coverage.cover(79, "no overlap (delete a)");
                // no overlap -- delete
                a.prev.next = a.next;
                a.next.prev = a.prev;
                a = a.next;
                continue;
            }
            int bEnd = b.y + b.h;
            if (bEnd <= a.y) {
                // // Coverage.cover(80, "no overlap (skip b)");
                // no overlap -- skip
                b = b.next;
                continue;
            }
            if (a.y <= b.y) {
                // // Coverage.cover(81, "a.y <= b.y (trim band)");
                // we're not interested in the non-overlap.
                // begin by shortening the band.
                a.y = b.y;
                a.h = aEnd - a.y;
            }       
            if (aEnd > bEnd) {
                // // Coverage.cover(82, "aEnd > bEnd");
                if ((b.next != l) && (b.next.y < aEnd)) {
                    // may need to split
                    XSpan newKids = XSpan.copyInside(x1, x2, a.children);
                    if (newKids == null) {
                        // // Coverage.cover(83, "newKids == null (delete band)");
                        // we've discovered that this band can't contribute
                        // to overlap at all.  Delete it and don't bother
                        // with the split.
                        a.prev.next = a.next;
                        a.next.prev = a.prev;
                        a = a.next;
                        b = b.next;
                        continue;
                    } else {
                        // // Coverage.cover(84, "split band");
                        a.next = a.next.prev =
                                        new YXBand(b.next.y, aEnd - b.next.y, newKids,
                                            a, a.next);
                    }
                }
                // now shorten the band
                a.h = bEnd - a.y;
            }
            // intersect the x spans
            XSpan.intersect(a.children, b.children);
            if (a.children.next == a.children) {
                // // Coverage.cover(85, "delete empty band");
                // band is empty--delete it
                a.prev.next = a.next;
                a.next.prev = a.prev;
            }
            a = a.next;
        }
        // delete any remaining a elements which don't overlap.
        a.prev.next = bandList;
        bandList.prev = a.prev;
    }

    /*
     * destructively union two band structures
     */
    protected void union(YXBand l) {
        YXBand a = bandList.next;
        YXBand b = l.next;
        while ((a != bandList) && (b != l)) {
            int aEnd = a.y + a.h;
            if (aEnd <= b.y) {
                // Coverage.cover(15, "no overlap -- skip");
                // no overlap -- skip
                a = a.next;
                continue;
            }
            int bEnd = b.y + b.h;
            if (bEnd <= a.y) {
                // Coverage.cover(16, "no overlap -- insert part of b");
                // no overlap -- simple insertion of (part of) b
                int start = (a.prev == bandList) ? b.y : (a.prev.y + a.prev.h);
                if (b.y > start) {
                    start = b.y;
                }
                a.prev = a.prev.next =
                                new YXBand(start, bEnd - start, XSpan.copy(b.children),
                                    a.prev, a);
                b = b.next;
                continue;
            }
            if (b.y < a.y) {
                // Coverage.cover(40, "add non-overlap which precedes overlap");

                int start = (a.prev == bandList) ? b.y : (a.prev.y + a.prev.h);
                if (b.y > start) {
                    start = b.y;
                }
                if (a.y > start) {
                    a.prev = a.prev.next =
                                    new YXBand(start, a.y - start, XSpan.copy(b.children),
                                        a.prev, a);
                }
            }
            if ((a.y < b.y) && (aEnd > b.y)) {
                // Coverage.cover(18, "keep non-overlap which precedes overlap");
                // keep the non-overlap area preceding the overlap
                a.next = a.next.prev = 
                                new YXBand(b.y, aEnd - b.y,
                                    XSpan.copy(a.children), a, a.next);
                a.h = b.y - a.y;
                a = a.next;
            }
            if (aEnd > bEnd) {
                // Coverage.cover(19, "keep non-overlap which follows overlap");
                // keep the non-overlap area following the overlap
                a.next = a.next.prev =
                                new YXBand(bEnd, aEnd - bEnd, 
                                    XSpan.copy(a.children), a, a.next);
                a.h = bEnd - a.y;
            }
            XSpan.merge(a.children, b.children);
            a = a.next;
            if (aEnd >= bEnd) {
                // Coverage.cover(20, "aEnd >= bEnd");
                b = b.next;
            }
        }
        if (b != l) {
            int start = (a.prev == bandList) ? b.y : (a.prev.y + a.prev.h);
            if (b.y > start) {
                start = b.y;
            }
            a.prev = a.prev.next =
                            new YXBand(start, b.y + b.h - start, XSpan.copy(b.children),
                                a.prev, a);
            b = b.next;
            while (b != l) {
                a.prev = a.prev.next =
                                new YXBand(b.y, b.h, XSpan.copy(b.children), a.prev, a);
                b = b.next;
            }
        }
    }

    /* Scan the band list.  If two adjacent y bands have identical
     * lists of children, coalesce the bands.
     */
    public void deFragment() {
        // // Coverage.cover(86, "deFragment");
        for (YXBand yb = bandList.next; yb != bandList.prev; yb = yb.next) {
            while ((yb.h == 0) && (yb != bandList)) {
                yb.prev.next = yb.next;
                yb.next.prev = yb.prev;
                yb = yb.next;
            }
            if ((yb.y + yb.h) != yb.next.y) {
                // // Coverage.cover(87, "non-adjacent");
                continue;       // not adjacent
            }
            XSpan aHead = yb.children;
            XSpan bHead = yb.next.children;
            XSpan a = aHead.next;
            XSpan b = bHead.next;
            while ((a != aHead) && (b != bHead)) {
                if ((a.x == b.x) && (a.w == b.w)) {
                    // // Coverage.cover(88, "spans are equal");
                    a = a.next;
                    b = b.next;
                } else {
                    // // Coverage.cover(89, "spans not equal");
                    break;
                }
            }
            if ((a == aHead) && (b == bHead)) {
                // // Coverage.cover(90, "coalesce");
                // children match--coalesce
                yb.h += yb.next.h;
                yb.next = yb.next.next;
                yb.next.prev = yb;
                yb = yb.prev;
            }
        }
    }

    /*
     * See if the rectangle x,y,w,h is all within the band structure.
     */
    protected boolean contains(int x, int y, int w, int h) {
        if ((w < 0) || (h < 0)) {
            throw new IllegalArgumentException("Width or height is < 0");
        }
        // // Coverage.cover(91, "contains(x,y,w,h)");
        YXBand yb;
        XSpan xs;
        int x2 = x + w;
        int y2 = y + h;
        int xEnd, yEnd;
        yb = YXBand.findBand(y, y2, bandList);
        if (yb == null) {
            // // Coverage.cover(92, "no relevant bands");
            return false;
        }
        // before we loop, check first band
        if (yb.y > y) {
            // // Coverage.cover(93, "end uncovered");
            return false;       // end uncovered
        }
        // now scan all bands between y and y2, checking for
        // discontiguity and making sure x range is covered
        for (yEnd = yb.y; (yb != bandList) && (yEnd < y2); yb = yb.next) {
            if (yb.y > yEnd) {
                // // Coverage.cover(94, "y discontiguous");
                return false;   // discontigous
            }
            // before we loop, check last band
            XSpan xHead = yb.children;
            if ((xHead.prev.x + xHead.prev.w) < x2) {
                // // Coverage.cover(95, "end uncovered");
                return false;   // end uncovered
            }
            // search for first overlapping band
            for (xs = xHead.next; xs != xHead; xs = xs.next) {
                if (xs.x > x) {
                    // // Coverage.cover(96, "end uncovered");
                    return false; // end uncovered
                } else if (xs.x + xs.w > x) {
                    // // Coverage.cover(97, "found band");
                    break;
                }
            }
            // now scan all bands between x and x2 for discontiguity

            // NOTE: we really should be able to dispense with this loop,
            // because we coalesce XSpans at every step.  Either there is
            // a single XSpan which contains all of x..xEnd, or we can
            // return false.  But for now I'll let it stand because it's
            // only a minor ineffeciency in practice.
            for (xEnd = xs.x; (xs != xHead) && (xEnd < x2); xs = xs.next) {
                if (xs.x > xEnd) {
                    // // Coverage.cover(98, "discontiguous");
                    return false; // discontiguous
                }
                xEnd = xs.x + xs.w;
            }
            // // Coverage.cover(99, "update yEnd");
            yEnd = yb.y + yb.h;
        }
        return true;
    }

    public boolean equals(YXBandList head) {
        YXBand aBand = bandList.next;
        YXBand bBand = head.bandList.next;
        while ((aBand != bandList) || (bBand != head.bandList)) {
            if ((aBand.y != bBand.y) || (aBand.h != bBand.h)) {
                return false;
            }
            XSpan aHead = aBand.children;
            XSpan bHead = bBand.children;
            XSpan a = aHead.next;
            XSpan b = bHead.next;
            while ((a != aHead) || (b != bHead)) {
                if ((a.x != b.x) || (a.w != b.w)) {
                    return false;
                }
                a = a.next;
                b = b.next;
            }
            aBand = aBand.next; 
            bBand = bBand.next;
        }
        return true;
    }

    /*
     * find the smallest rectangle which contains the whole band structure.
     */
    public java.awt.Rectangle getBounds() {
        // // Coverage.cover(100, "getBounds");
        if ((bandList == null) || (bandList.next == bandList)) {
            // // Coverage.cover(101, "null bandList");
            bandList = null;
            return new Rectangle();
        }
        XSpan firstX = bandList.next.children;
        int x1 = firstX.next.x;
        int x2 = firstX.prev.x + firstX.prev.w;
        int tmp;
        for (YXBand l = bandList.next.next; l != bandList; l = l.next) {
            tmp = l.children.next.x;
            if (x1 > tmp) {
                // // Coverage.cover(102, "increase x1");
                x1 = tmp;
            }
            XSpan lastX = l.children.prev;
            tmp = lastX.x + lastX.w;
            if (x2 < tmp) {
                // // Coverage.cover(103, "decrease x2");
                x2 = tmp;
            }
            // // Coverage.cover(104, "(spans examined)");

        }
        YXBand prev = bandList.prev;
        return new Rectangle(x1, bandList.next.y,
                x2 - x1, prev.y + prev.h - bandList.next.y);
    }

    /*
     * translate all bands by dx, dy
     */
    protected void translate(int dx, int dy) {
        // // Coverage.cover(105, "translate(dx,dy)");
        for (YXBand yb = bandList.next; yb != bandList; yb = yb.next) {
            yb.y += dy;
            // // Coverage.cover(106, "bands processed");
            XSpan xHead = yb.children;
            for (XSpan xs = xHead.next; xs != xHead; xs = xs.next) {
                // // Coverage.cover(107, "spans processed");
                xs.x += dx;
            }
        }
    }

    // no test coverage for toString
    public String toString() {
        return YXBand.makeString(bandList);
    }
}
