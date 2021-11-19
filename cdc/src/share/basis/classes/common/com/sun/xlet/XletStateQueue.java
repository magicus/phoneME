/*
 * @(#)XletStateQueue.java	1.9 06/10/10
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

package com.sun.xlet;

/*
 * Implements a queue that keeps track of incoming xlet state change requests
 * and executes them in order.  There is one queue per one xlet.
 * By having this class, the caller requesting the xlet state change 
 * does not have to be blocked waiting for a lock even if another xlet state change
 * request is being processed.
 */

public class XletStateQueue {
    EventQueueItem head = null;
    EventQueueItem tail = null;
    XletManager manager;

    // The thread that's processing the requests. It should be in the 
    // ThreadGroup created by the XletManager.
    Thread queueThread;

    // To check if the xlet is still alive (not destroyed).
    boolean isAlive = true;
    public XletStateQueue(XletManager xletManager) {
        this.manager = xletManager;
        queueThread = new Thread(manager.threadGroup,
                    new Runnable() {
                        public void run() {
                            while (isAlive && 
                                manager.getXletState() != XletState.DESTROYED) {
                                dispatchRequests();
                            }
                        }
                    }, "XletStateQueue lookup thread");
        queueThread.setContextClassLoader(xletManager.getClassLoader());
        queueThread.start();
    }

    // Push the request to the queue.
    public synchronized void push(XletState desired) {
        if (head == null) {
            // empty queue
            head = new EventQueueItem(desired);
            tail = head;
        } else {
            tail.next = new EventQueueItem(desired);
            tail = tail.next;
        }
        notifyAll();
    }

    // Pop the request from the queue.
    public synchronized XletState pop() {
        XletState returningState = null;
        if (head != null) { 
            returningState = head.state; 
            head = head.next;
        }
        return returningState;
    }

    // True if there is no request pending in the queue.
    public synchronized boolean isEmpty() {
        return (head == null);
    }

    // Clear up all the requests in this queue.
    public synchronized void clear() {
        head = tail = null;
    }

    // Destroy the queue. Called when the xlet is destroyed.
    public synchronized void destroy() {
        clear();
        isAlive = false;
        notifyAll();
    }

    // Process the xlet state change request pending in this queue.
    public synchronized void dispatchRequests() {
        while (isEmpty() && isAlive) {
            try {
                wait();
            } catch (InterruptedException e) {}
        }
        if (!isAlive) return;
        manager.handleRequest(pop());
    }
}

// Basic state change request date object kept in the queue. 
class EventQueueItem {
    XletState state;
    EventQueueItem next;
    EventQueueItem(XletState state) {
        this.state = state;
    }
}

// Extended state change request date object kept in the queue. 
// This is to hide XletState objects that cannot be of a final state 
// (INITIALIZE and CONDITIONAL_DESTROY are valid state change requests
// but not valid states for an xlet to be in.)
class DesiredXletState extends XletState {
    static final XletState INITIALIZE;
    static final XletState CONDITIONAL_DESTROY;
    static {
        INITIALIZE = new XletState("initialize");
        CONDITIONAL_DESTROY = new XletState("conditional_destroy");
    }
    protected DesiredXletState(String name) {
        super(name);
    }
}

// XletState class object - to keep track of the current xlet state.
class XletState {
    static final XletState UNLOADED;
    static final XletState LOADED;
    static final XletState PAUSED;
    static final XletState ACTIVE;
    static final XletState DESTROYED;
    private String name = null;
    protected XletState(String name) { 
        this.name = name; 
    }

    public String toString() { 
        return name; 
    }
    static {
        UNLOADED = new XletState("unloaded");
        LOADED = new XletState("loaded");
        PAUSED = new XletState("paused");
        ACTIVE = new XletState("active");
        DESTROYED = new XletState("destroyed");
    }
}
