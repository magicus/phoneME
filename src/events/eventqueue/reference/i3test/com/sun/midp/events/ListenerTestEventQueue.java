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

/**
 * Test event queue. This queue's sendNativeEventToIsolate and post methods
 * just call the registered event listener directly.
 *
 * This class is used to test the producer, listener, consumer trio of classes
 * for an event group.
 */
public class ListenerTestEventQueue extends EventQueue {
    /**
     * Send a native event to a specific Isolate. This version just dispatches
     * the event directly.
     *
     * @param event a native event
     * @param isolateId ID of the receiving Isolate
     */
    public void sendNativeEventToIsolate(NativeEvent event,
                                         int isolateId) {
        dispatch(event);
    }

    /**
     * Post an event to the queue.  This version just dispatches
     * the event directly.
     *
     * @param event event to post
     */
    public void post(Event event) {
        dispatch(event);
    }

    /**
     * Dispatch the event.
     *
     * @param event event to dispatch
     */
    void dispatch(Event event) {
        DispatchData dispatchData = dispatchTable[event.type - 1];

        dispatchData.listener.process(event);
    }
}
