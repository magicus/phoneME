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

package com.sun.midp.events;

import java.util.Vector;

/**
 * This is a stubbed-out version of EventQueue, for testing purposes.
 */
public class StubEventQueue extends EventQueue {

    Vector eventLog;

    /**
     * Constructs an stub event queue.
     */
    public StubEventQueue() {
        eventLog = new Vector();
    }

    /**
     * Send a native event to a specific Isolate.
     *
     * @param event a native event
     * @param isolateId ID of the receiving Isolate
     */
    public void sendNativeEventToIsolate(NativeEvent event,
                                                int isolateId) {
        eventLog.addElement(event);
        // IMPL_NOTE: keep track of isolateId somehow
    }

    /**
     * Post an event to the queue. The reference to the event will be put in
     * the queue without copying, so the caller should not reuse events
     * passed to this method.
     *
     * @param event event to post
     */
    public void post(Event event) {
        eventLog.addElement(event);
    }

    /**
     * Gets the set of logged events.
     */
    public Event[] getEventLog() {
        Event[] arr = new Event[eventLog.size()];
        eventLog.copyInto(arr);
        return arr;
    }

}
