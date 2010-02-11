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

package com.sun.jumpimpl.module.eventqueue;

import java.util.Map;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.Enumeration;
import java.util.NoSuchElementException;
import com.sun.jump.module.eventqueue.JUMPEventQueueModule;
import com.sun.jump.module.eventqueue.JUMPEventHandler;

/**
 * <code>EventQueueModuleImpl</code> is responsible for JSR-specific
 * event dispatching.
 */
public class EventQueueModuleImpl implements JUMPEventQueueModule {
    /** Table to map event type to its processor. */
    private Hashtable handlers;

    /** Flag indicating whether the event receiving thread should proceed. */
    private volatile boolean initialized;

    /** Queue to store events of the corresponding type. */
    private LinkedList queue = new LinkedList();

    /** Thread for processing events in the queue. */
    private Thread eventProcessor;

    /**
     * Loads the event queue module and starts to receive native events.
     *
     * @param config properties of this module factory.
     */
    public void load(Map config) {
        // Initialize native event system.
        if (!initEventSystem()) {
            throw new RuntimeException("Cannot initialize event queue.");
        }

        initialized = true;
        handlers = new Hashtable();
        
        // Start the main thread with event receiving loop.
        new Thread() {
            public void run() {
                while (initialized) {
                    EventData event = new EventData();
                    event.type = receiveEvent(event);
                    if (event.type >= 0) {
                        synchronized (queue) {
                            queue.add(event);
                            queue.notify();
                        }
                    }
                }
            }
        }.start();

        // Start the event processing thread.
        eventProcessor = new Thread() {
            public void run() {
                while (true) {
                    EventData event;
                    synchronized (queue) {
                        try {
                            // Try to get the first event from the queue.
                            event = (EventData)queue.removeFirst();
                        } catch (NoSuchElementException e) {
                            try {
                                // The queue is empty. Sleep until an event is available.
                                queue.wait();
                                // A new event is available, restart the loop to process it.
                                continue;
                            } catch (InterruptedException ie) {
                                // Shutdown the processing thread.
                                return;
                            }
                        }
                    }
                    // An event has been extracted from the queue, try to find
                    // a handler that will process it.
                    // Note: we do the processing out of the synchronized block,
                    // so new events can be queued up while the current one is
                    // being processed.
                    JUMPEventHandler handler = (JUMPEventHandler)handlers.get(new Integer(event.type));
                    if (handler != null) {
                        handler.handleEvent(event.id, event.data);
                    }
                }
            }
        };
        eventProcessor.start();
    }

    /**
     * Unloads the event queue module.
     */
    public void unload() {
        // Shutdown main thread that receives events.
        initialized = false;

        // Shutdown native event system.
        shutdownEventSystem();

        // Finish the event processing thread.
        eventProcessor.interrupt();

        handlers.clear();
    }

    /**
     * Registers a handler for events of the given type.
     *
     * @param type event type, which equals to the number of JSR that
     *        will handle events.
     * @param handler a <code>JUMPEventHandler</code> instance that
     *        will deal with all events of this type.
     */
    public void registerEventHandler(int type, JUMPEventHandler handler) {
        handlers.put(new Integer(type), handler);
    }

    /**
     * Initializes platform events.
     *
     * @return <code>true</code> on success, <code>false</code> otherwise.
     */
    private native boolean initEventSystem();

    /**
     * Shuts platform events down.
     *
     * @return <code>true</code> on success, <code>false</code> otherwise.
     */
    private native boolean shutdownEventSystem();

    /**
     * Receives next event from the platform. This method will block until
     * an event arrives.
     *
     * @param event object to be filled with event data.
     * @return event type (i.e. JSR number) on success, -1 otherwise.
     */
    private native int receiveEvent(EventData event);
}

/**
 * This class holds event information that may be needed for the handler.
 */
class EventData {
    /** 
     * Event type, used for dispatching events to their respective handlers.
     */
    int type;

    /** 
     * Event identifier, used for distinguishing various
     * kinds of events processed by one handler.
     */
    int id;

    /** 
     * Event data, may contain any information specific to
     * the particular event type.
     */
    byte[] data;
}
