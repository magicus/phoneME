/*
 * @(#)XSpan.java	1.11 06/10/10
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
 * A class which stores an X span.
 *
 * @version 1.6, 08/19/02
 */
class XSpan {
    int x, w;
    XSpan next, prev;
    public XSpan(int x, int w) {
        // // Coverage.cover(114, "XSpan(x,w)");
        this.x = x;
        this.w = w;
    }

    public XSpan(int x, int w, XSpan prev, XSpan next) {
        // // Coverage.cover(115, "XSpan(x,w,prev,next)");
        this.x = x;
        this.w = w;
        this.prev = prev;
        this.next = next;
    }

    /*
     static void validateList(String msg, XSpan head) {
     for (XSpan p = head.next; p != head.prev; p = p.next) {
     if ((p.x + p.w) >= p.next.x) {
     System.err.println(msg + ": Bands are out of order??");
     System.err.println("  " +p.next+ " is after " +p);
     System.err.println("Whole span list: ");
     p = head.next;
     while (p != head) {
     System.err.println("    " +p);
     p = p.next;
     }
     throw new RuntimeException("invalid XSpans");
     }
     }
     }
     */

    /*
     * Destructively add a span between x and x+w.  This
     * replaces (or is merged with) any spans currently in
     * that region.
     */
    static void cover(XSpan head, int x, int w) {
        // validateList("+cover",head);

        XSpan x1, x2;
        int xEnd = x + w;
        // one simple, special case, because append is not uncommon
        int endOfBand = head.prev.x + head.prev.w;
        if (x >= endOfBand) {
            // Coverage.cover(21, "x >= endOfBand");
            if (x == endOfBand) {
                // Coverage.cover(22, "x == endOfBand");
                head.prev.w += w;
            } else {
                // Coverage.cover(23, "x != endOfBand");
                head.prev = head.prev.next =
                                new XSpan(x, w, head.prev, head);
            }
            return;
        }
        // find first overlapping span
        for (x1 = head.next; x1 != head; x1 = x1.next) {
            if ((x1.x + x1.w) >= x) {
                break;
            }
        }
        if ((x1 != head) && (x1.x < x)) {
            // Coverage.cover(24, "x1.x < x");
            x = x1.x;
        }
        // and last overlapping span
        if (x1 == head) {
            // Coverage.cover(25, "x1 == head");
            x2 = x1.prev;
        } else {
            // Coverage.cover(26, "scan for x2");
            for (x2 = x1; x2.next != head; x2 = x2.next) {
                if ((x2.x + x2.w) >= xEnd) {
                    break;
                }
            }
        }
        if (x2.x > xEnd) {
            // Coverage.cover(27, "x2.x > xEnd");
            x2 = x2.prev;
        }
        if ((x2.x + x2.w) > xEnd) {
            // Coverage.cover(28, "(x2.x + x2.w) > xEnd");
            xEnd = x2.x + x2.w;
        }
        x2.next.prev = x1.prev.next = 
                        new XSpan(x, xEnd - x, x1.prev, x2.next);
        // validateList("-cover", head);
    }

    /*
     * Utility function which destructively deletes from the
     * list any span that is inside x..x2
     */
    static void deleteInside(int x, int x2, XSpan head) {
        // validateList("+deleteInside", head);
        // // Coverage.cover(116, "deleteInside(x,x2,head)");
        XSpan xs;
        for (xs = head.next; xs != head; xs = xs.next) {
            if (xs.x >= x2) {
                // // Coverage.cover(117, "no spans in range");
                return;
            } else if ((xs.x + xs.w) > x) {
                // // Coverage.cover(118, "found span");
                break;
            }
        }
        if (xs == head) {
            // // Coverage.cover(119, "ran off end");
            return;
        }
        while (xs != head) {
            int xEnd = (xs.x + xs.w);
            if (xs.x < x) {
                // // Coverage.cover(120, "shorten band");
                // shorten band
                xs.w = x - xs.x;
                if (xEnd > x2) {
                    // split
                    // // Coverage.cover(121, "xEnd > x2 (split)");
                    xs.next = xs.next.prev =
                                    new XSpan(x2, xEnd - x2, xs, xs.next);
                }
            } else if (xs.x >= x2) {
                // // Coverage.cover(122, "out of range (return)");
                return;
            } else if (xEnd > x2) {
                // // Coverage.cover(123, "xEnd > x2 (shorten)");
                // shorten band
                xs.x = x2;
                xs.w = xEnd - x2;
                return;
            } else {
                // // Coverage.cover(124, "delete band");
                xs.prev.next = xs.next;
                xs.next.prev = xs.prev;
            }
            // // Coverage.cover(125, "band advanced");
            xs = xs.next;
        }
        // validateList("-deleteInside", head);
    }

    /*
     * Utility function which destructively deletes from the
     * list any span that is outside x..x2
     */
    static void deleteOutside(int x, int x2, XSpan head) {
        // validateList("+deleteOutside", head);
        // // Coverage.cover(126, "deleteOutside(x,x2,head)");
        XSpan xs = head.next;
        while ((xs != head) && ((xs.x + xs.w) <= x)) {
            // // Coverage.cover(127, "scan forwards");
            xs = xs.next;
        }
        if (xs.x < x) {
            // // Coverage.cover(128, "shorten first band");
            xs.w = xs.x + xs.w - x;
            xs.x = x;
        }
        xs.prev = head;
        head.next = xs;
        xs = head.prev;
        while ((xs != head) && (xs.x > x2)) {
            // // Coverage.cover(129, "scan backwards");
            xs = xs.prev;
        }
        if ((xs.x + xs.w) > x2) {
            // // Coverage.cover(130, "shorten last band");
            xs.w = x2 - xs.x;
            if (xs.w <= 0) {
                // // Coverage.cover(183,"Delete band that was shortened to 0");
                xs.prev.next = xs.next;
                xs.next.prev = xs.prev;
            }
        }
        xs.next = head;
        head.prev = xs;
        // validateList("-deleteOutside", head);
    }

    /*
     * Copy a list of spans
     */
    static XSpan copy(XSpan head) {
        // validateList("+copy", head);
        // // Coverage.cover(131, "copy(XSpan)");
        if (head == null) {
            throw new RuntimeException("copy: head == null");
            // // Coverage.cover(132, "head == null");
            // return null;
        }
        // create dummy list head
        XSpan q = new XSpan(0, -1);
        // copy first real element
        XSpan p = head.next;
        q.next = new XSpan(p.x, p.w, q, q);
        q.prev = q.next;
        // copy remainder of list
        for (p = p.next; p != head; p = p.next) {
            // // Coverage.cover(133, "scan list");
            q.prev = q.prev.next = new XSpan(p.x, p.w, q.prev, q);
        }
        // validateList("-copy", q);
        return q;
    }

    /*
     * make a copy of spans, only considering those inside x..x2
     */
    static XSpan copyInside(int x, int x2, XSpan head) {
        // validateList("+copyInside", head);
        // // Coverage.cover(134, "copyInside(x,x2,head)");

        if (head == null) {
            // // Coverage.cover(135, "head == null");
            return null;
        }
        XSpan xs = head.next;
        XSpan last = head.prev;
        int start = xs.x;
        int end = (last.x + last.w);
        if ((end <= x) || (start >= x2)) {
            // // Coverage.cover(136, "everything to one side or other");
            // everything is to one side or the other
            return null;
        } else if ((start >= x) && (end <= x2)) {
            // // Coverage.cover(137, "all inside");
            // save some overhead -- they're all inside!
            return XSpan.copy(head);
        }
        // find the last child that could be in the region
        for (; last != head; last = last.prev) {
            // // Coverage.cover(138, "scan list");
            if (last.x < x2) {
                // // Coverage.cover(139, "found one");
                break;
            }
        }
        // if the last candidate is left of x, there are none.
        if ((last.x + last.w) <= x) {
            // // Coverage.cover(140, "last candidate is left of x");
            return null;
        }
        // now search forward for the first span in x..x2
        for (; xs != last; xs = xs.next) {
            // // Coverage.cover(141, "scan list");
            if ((xs.x + xs.w) > x) {
                // // Coverage.cover(142, "found one");
                break;
            }
        }
        // special case code to build the list head
        int newX = xs.x;
        if (x > newX) {
            // // Coverage.cover(143, "increase newX");
            newX = x;
        }
        int xEnd = xs.x + xs.w;
        if (x2 < xEnd) {
            // // Coverage.cover(144, "decrease xEnd");
            xEnd = x2;
        }
        XSpan newHead = new XSpan(0, -1);
        newHead.next = new XSpan(newX, xEnd - newX, newHead, newHead);
        newHead.prev = newHead.next;
        if (xs == last) {
            // // Coverage.cover(145, "only one span");
            // there was only one span in x..x2
            return newHead;
        }
        xs = xs.next;
        while (xs != last) {
            // // Coverage.cover(146, "scan list");
            newHead.prev = newHead.prev.next = 
                            new XSpan(xs.x, xs.w, newHead.prev, newHead);
            xs = xs.next;
        }
        if ((xs.x + xs.w) > x2) {
            // // Coverage.cover(147, "xs.x + xs.w > x2 (add new span)");
            newHead.prev = newHead.prev.next =
                            new XSpan(xs.x, x2 - xs.x, newHead.prev, newHead);
        } else {
            // // Coverage.cover(148, "xs.x + xs.w <= x2 (add new span)");
            newHead.prev = newHead.prev.next =
                            new XSpan(xs.x, xs.w, newHead.prev, newHead);
        }
        // validateList("-copyInside", newHead);
        return newHead;
    }

    /*
     * make a copy of spans, only considering those outside x..x2
     */
    static XSpan copyOutside(int x, int x2, XSpan head) {
        // validateList("+copyOutside", head);

        // // Coverage.cover(149, "copyOutside(x,x2,head)");
        // don't bother if all spans are inside x..x2
        if ((head == null)
            || ((head.next.x >= x) 
                && ((head.prev.x + head.prev.w) <= x2))) {
            // // Coverage.cover(150, "all inside");
            return null;
        }
        // search for the first child that is outside x..x2
        // (this is in case they're all on the x2 side)
        XSpan xs = head.next;
        if (xs.x >= x) {
            // // Coverage.cover(151, "xs.x >= x");
            while ((xs.next != head) && ((xs.x + xs.w) <= x2)) {
                // // Coverage.cover(152, "scan list");
                xs = xs.next;
            }
        }
        // now set up the list
        XSpan newHead = new XSpan(0, -1);
        int xEnd = xs.x + xs.w;
        if (xEnd > x) {
            if (xs.x < x) {
                // // Coverage.cover(153, "xEnd > x && xs.x < x");
                newHead.next = new XSpan(xs.x, x - xs.x, newHead, newHead);
            } else if ((xs.x < x2) && (xEnd > x2)) {
                // // Coverage.cover(154, "xEnd > x2 && xs.x < x2");
                newHead.next = new XSpan(x2, xEnd - x2, newHead, newHead);
            } else {
                // xs.x must be >= x2
                // // Coverage.cover(155, "copy span verbatim");
                newHead.next = new XSpan(xs.x, xs.w, newHead, newHead);
            }
        } else {
            // // Coverage.cover(156, "xEnd <= x");
            newHead.next = new XSpan(xs.x, xs.w, newHead, newHead);
        }
        newHead.prev = newHead.next;
        if ((newHead.prev.x >= x2) && (xs.next == head)) {
            // // Coverage.cover(157, "only one element?");
            return newHead;
        }
        if ((xs.x < x) && (xEnd > x2)) {
            // // Coverage.cover(158, "one element splits x..x2");
            // it covers all of x..x2
            newHead.prev = newHead.prev.next = 
                            new XSpan(x2, xEnd - x2, newHead.prev, newHead);
        }
        if (xs.x < x) {
            // // Coverage.cover(159, "xs.x < x");
            for (xs = xs.next; xs != head; xs = xs.next) {
                if (xs.x >= x) {
                    // // Coverage.cover(160, "xs.x >= x");
                    break;
                } else if ((xs.x + xs.w) > x) {
                    // // Coverage.cover(161, "xs.x + xs.w > x");
                    newHead.prev = newHead.prev.next =
                                    new XSpan(xs.x, x - xs.x, newHead.prev, newHead);
                    break;
                } else {
                    // // Coverage.cover(162, "default case");
                    newHead.prev = newHead.prev.next =
                                    new XSpan(xs.x, xs.w, newHead.prev, newHead);
                }
            }
        } else if (newHead.prev.x >= x2) {
            xs = xs.next;
        }
        // skip over children that are inside x..x2
        for (; xs != head; xs = xs.next) {
            // // Coverage.cover(163, "skip ");
            if ((xs.x + xs.w) > x2) {
                // // Coverage.cover(164, "found one that crosses x2");
                break;
            }
        }
        if ((xs != head) && (xs.x < x2)) {
            // // Coverage.cover(165, "insert new span");
            newHead.prev = newHead.prev.next =
                            new XSpan(x2, xs.x + xs.w - x2, newHead.prev, newHead);
            xs = xs.next;
        }
        for (; xs != head; xs = xs.next) {
            // // Coverage.cover(166, "copy end of list");
            newHead.prev = newHead.prev.next =
                            new XSpan(xs.x, xs.w, newHead.prev, newHead);
        }
        // validateList("-copyOutside("+x+", "+x2+")", newHead);
        return newHead;
    }

    /*
     * Utility function to subtract span lists.  Case analysis is
     * identical to intersection of YX lists, but simpler as there
     * are no children and no need to split spans.
     */
    static void subtract(XSpan headA, XSpan headB) {
        // validateList("+subtract", headA); 
        // validateList("+subtract", headB);
        // // Coverage.cover(167, "subtract(XSpan, XSpan)");
        XSpan a = headA.next;
        XSpan b = headB.next;
        while ((a != headA) && (b != headB)) {
            if ((a.x == b.x) && (a.w == b.w)) {
                // // Coverage.cover(168, "equal spans");
                // special case this in order to speed up common case
                // where large number of objects are equal
                XSpan saveA = a.prev;
                do {
                    a = a.next;
                    b = b.next;
                }
                while ((a != headA) && (b != headB)
                    && (a.x == b.x) && (a.w == b.w));
                a.prev = saveA;
                saveA.next = a;
                if ((a == headA) || (b == headB)) {
                    // // Coverage.cover(169, "all spans equal");
                    return;
                }
            }
            int aEnd = a.x + a.w;
            if (aEnd <= b.x) {
                // // Coverage.cover(170, "no overlap (skip a)");
                // no overlap -- skip
                a = a.next;
                continue;
            }
            int bEnd = b.x + b.w;
            if (bEnd <= a.x) {
                // // Coverage.cover(171, "no overlap (skip b)");
                // no overlap -- skip
                b = b.next;
                continue;
            }
            if (a.x < b.x) {
                // // Coverage.cover(172, "a.x < b.x (trim)");
                // keep the non-overlap area preceding the overlap
                a.w = b.x - a.x;
                if (aEnd > bEnd) {
                    // // Coverage.cover(173, "aEnd > bEnd (split band)");
                    // make a new band for the area following the overlap
                    a.next = a.next.prev =
                                    new XSpan(bEnd, aEnd - bEnd, a, a.next);
                }
                a = a.next;
            } else if (aEnd > bEnd) {
                // // Coverage.cover(174, "aEnd > bEnd (trim)");
                // set the band to the non-overlap area following the overlap
                a.w = aEnd - bEnd;
                a.x = bEnd;
                b = b.next;
            } else {
                // // Coverage.cover(175, "delete overlapping band");
                // it all overlaps--delete it
                a.prev.next = a.next;
                a.next.prev = a.prev;
                a = a.next;
            }
        }
        // validateList("-subtract", headA);
    }

    /*
     * Utility function to intersect span lists.  Case analysis is
     * identical to intersection of YX lists, but simpler as there are
     * no children to deal with.  There is one case where we must split
     * a span, changing
     *
     *    A    ----------
     *    B  -----    --------
     *
     *  to
     *
     *    A    ---    ---
     *    B  -----    --------
     */
    static void intersect(XSpan headA, XSpan headB) {
        // validateList("+intersect", headA); 
        // validateList("+intersect", headB);
        // // Coverage.cover(176, "intersect(XSpan,XSpan)");

        XSpan a = headA.next;
        XSpan b = headB.next;
        while ((a != headA) && (b != headB)) {
            int aEnd = a.x + a.w;
            if (aEnd <= b.x) {
                // // Coverage.cover(177, "no overlap (delete a)");
                // no overlap -- delete
                a.prev.next = a.next;
                a.next.prev = a.prev;
                a = a.next;
                continue;
            }
            int bEnd = b.x + b.w;
            if (bEnd <= a.x) {
                // // Coverage.cover(178, "no overlap (skip b)");
                // no overlap -- skip
                b = b.next;
                continue;
            }
            if (a.x <= b.x) {
                // // Coverage.cover(179, "a.x <= b.x (shorten band)");
                // we're not interested in the non-overlap.
                // begin by shortening the band.
                a.x = b.x;
                a.w = aEnd - a.x;
            }       
            if (aEnd > bEnd) {
                // // Coverage.cover(180, "aEnd > bEnd (shorten band)");
                // shorten the band
                a.w = bEnd - a.x;
                b = b.next;
                if ((b != headB) && (b.x < aEnd)) {
                    // // Coverage.cover(181, "split band");
                    // split the band
                    a.next = a.next.prev =
                                    new XSpan(b.x, aEnd - b.x, a, a.next);
                    // re-examine this band--it might overlap several b's
                }
            }
            // // Coverage.cover(182, "advance a");
            a = a.next;
        }
        // delete any remaining a elements which don't overlap.
        a.prev.next = headA;
        headA.prev = a.prev;
        // validateList("-intersect", headA);
    }
     
    static void merge(XSpan headA, XSpan headB) {
        // validateList("+merge", headA); 
        // validateList("+merge", headB);
        XSpan a = headA.next;
        XSpan b = headB.next;
        while ((a != headA) && (b != headB)) {
            int aEnd = a.x + a.w;
            // no overlap
            if (aEnd < b.x) {
                // Coverage.cover(29, "no overlap -- advance a");
                a = a.next;
                continue;
            }
            int bEnd = b.x + b.w;
            // no overlap
            if (bEnd < a.x) {
                // Coverage.cover(30, "no overlap -- insert (part of) b");
                a.prev = a.prev.next = new XSpan(b.x, b.w, a.prev, a);
                b = b.next;
                continue;
            }
            if (a.x > b.x) {
                // Coverage.cover(31, "a.x > b.x");
                a.x = b.x;
                a.w = aEnd - a.x;
                if ((a.prev != headA) && (a.prev.x + a.prev.w >= a.x)) {
                    // Coverage.cover(32, "merge with previous span");
                    // merge with the previous span
                    a.prev.w = aEnd - a.prev.x;
                    a.next.prev = a.prev;
                    a.prev.next = a.next;
                    a = a.prev;
                }
            }
            b = b.next;         // from this point on we only need bEnd
            if (bEnd > aEnd) {
                // Coverage.cover(33, "bEnd > aEnd");
                a.w = bEnd - a.x;
                aEnd = a.x + a.w;
                a = a.next;
                if ((a != headA) && (aEnd >= a.x)) {
                    // Coverage.cover(34, "merge with previous span");
                    // merge with the new previous span
                    a.prev.w = a.x + a.w - a.prev.x;
                    a.next.prev = a.prev;
                    a.prev.next = a.next;
                    a = a.prev;
                }
            } else {
                // Coverage.cover(35, "advance a");
                a = a.next;
            }
        }
        if (b == headB) {
            // Coverage.cover(36, "no more elements in b");
            return;
        }
        // handle leftovers in b.  If they overlap, must extend a
        a = headA.prev;
        int aEnd = a.x + a.w;
        while ((aEnd >= b.x) && (b != headB)) {
            int bEnd = b.x + b.w;
            // Coverage.cover(37, "overlapping leftover");
            if (bEnd > aEnd) {
                // Coverage.cover(38, "extend a");
                a.w = bEnd - a.x;
            }
            b = b.next;
        }
        // now all that's left is things to append to the list.
        while (b != headB) {
            // Coverage.cover(39, "nonoverlapping leftover");
            a.next = headA.prev = new XSpan(b.x, b.w, a, headA);
            a = a.next;
            b = b.next;
        }
        // validateList("-merge", headA);
    }
     
    // no test coverage
    public static String makeString(XSpan head) {
        String ret = "[";
        for (XSpan xs = head.next; xs != head; xs = xs.next) {
            if (xs != head.next) {
                ret += " ";
            }
            String s = "(" + xs.x + "," + xs.w + ")";
            ret += s;
        }
        return ret + "]";
    }

    // no test coverage
    public String toString() {
        return "(x = " + x + ", w = " + w + ")";
    }
}
