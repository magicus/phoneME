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

package com.sun.midp.util;

import java.util.Hashtable;

/**
 * A simple tracing mechanism that's useful for i3 tests to get calls when a 
 * particular tracepoint is hit.
 */
public class LiveTracer {
    // Hashtable<String, LiveTraceListener>
    Hashtable listeners = new Hashtable();

    /**
     * Registers a listener for tracepoints identified by this tag.  When a
     * tracepoint is hit whose tag matches this one, the listener is called.
     * Replaces any listener that might already exist for this tag.
     */
    public void add(String tag, LiveTraceListener l) {
        listeners.put(tag, l);
    }

    /**
     * Unregisters the indicated listener from the tracer.
     */
    public void remove(String tag) {
        listeners.remove(tag);
    }

    /**
     * Clears all listeners from the tracer.
     */
    public void clear() {
        listeners.clear();
    }

    /**
     * Indicates a tracepoint identified by this tag. If there is a listener 
     * registered for this tag, the listener is called.
     */
    public void trace(String tag) {
        LiveTraceListener l = 
            (LiveTraceListener)listeners.get(tag);

        if (l != null) {
            l.call(tag);
        }
    }
}
