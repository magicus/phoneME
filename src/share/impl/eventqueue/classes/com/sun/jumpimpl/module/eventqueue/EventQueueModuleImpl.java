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
    Hashtable handlers;

    /** Flag indicating whether the event receiving thread should proceed. */
    volatile boolean initialized;

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
                    EventData event = new EventData(0, null);
                    int type = receiveEvent(event);
                    EventProcessor ep = (EventProcessor)handlers.get(new Integer(type));
                    if (ep != null) {
                        ep.addEvent(event);
                    }
                }
            }
        }.start();
    }

    /**
     * Unloads the event queue module.
     */
    public void unload() {
        // Shutdown main thread that receives events.
        initialized = false;

        // Shutdown native event system.
        shutdownEventSystem();

        // Finish all event processing threads.
        Enumeration e = handlers.elements();
        while (e.hasMoreElements()) {
            ((EventProcessor)e.nextElement()).interrupt();
        }
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
        // Make a dedicated queue to store events of the given type.
        handlers.put(new Integer(type), new EventProcessor(handler));
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
 * This class represents a processing entity for one type of events.
 */
class EventProcessor extends Thread {
    /** Queue to store events of the corresponding type. */
    private LinkedList queue;

    /** Handler for events of the corresponding type. */
    private JUMPEventHandler handler;

    /**
     * Creates an <code>EventProcessor</code> instance.
     *
     * @param h handler for this type of events.
     */
    EventProcessor(JUMPEventHandler h) {
        queue = new LinkedList();
        handler = h;

        // Start a new thread for handling events of the given type.
        start();
    }

    /**
     * Puts a new event in the queue and wakes up the processing thread.
     *
     * @param event event data.
     */
    synchronized void addEvent(EventData event) {
        queue.add(event);
        notify();
    }

    /**
     * Event processing happens here.
     */
    public void run() {
        while (true) {
            EventData event;
            synchronized (this) {
                try {
                    // Try to get the first event from the queue.
                    event = (EventData)queue.removeFirst();
                } catch (NoSuchElementException e) {
                    try {
                        // The queue is empty. Sleep until an event is available.
                        wait();
                        // A new event is available, restart the loop to process it.
                        continue;
                    } catch (InterruptedException ie) {
                        // Shutdown the processing thread.
                        return;
                    }
                }
            }
            // An event has been extracted from the queue, process it.
            // Note: we do the processing out of the synchronized block,
            // so new events can be queued up while the current one is
            // being processed.
            handler.handleEvent(event.id, event.data);
        }
    }
}

/**
 * This class holds event information that may be needed for the handler.
 */
class EventData {
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

    /** 
     * Creates a new entity representing a single event.
     *
     * @param id event identifier.
     * @param data event data.
     */
    EventData(int id, byte[] data) {
        this.id = id;
        this.data = data;
    }
}
