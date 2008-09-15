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
import java.util.Vector;

final class AutoEventSequenceImpl implements AutoEventSequence {
    private Vector events;

    AutoEventSequenceImpl() {
        events = new Vector();
    }

    public String toString() {
        if (isEmpty()) {
            return "";
        }

        StringBuffer buf = new StringBuffer();
        int sz = events.size() - 1;
        for (int i = 0; i < sz; ++i) {
            buf.append(events.elementAt(i));
            buf.append("\n");
        }
    
        buf.append(events.elementAt(sz));

        return buf.toString();
    }

    public AutoEvent[] getEvents() {
        int sz = events.size();
        AutoEvent a[] = new AutoEvent[sz];
        for (int i = 0; i < sz; ++i) {
            a[i] = (AutoEvent)events.elementAt(i);
        }

        return a;
    }

    public boolean isEmpty() {
        return events.isEmpty();
    }

    public void addEvents(AutoEvent event) 
        throws IllegalArgumentException {

        if (event == null) {
            throw new IllegalArgumentException("Event is null");
        }

        events.addElement(event);
    }

    public void addEvents(AutoEvent[] arr) 
        throws IllegalArgumentException {

        if (arr == null) {
            throw new IllegalArgumentException("Event array is null");
        }        

        int size = arr.length;
        for (int i = 0; i < size; ++i) {
            addEvents(arr[i]);
        }
    }
}
