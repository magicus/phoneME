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

import javax.microedition.lcdui.Display;

/**
 * An adaptor class that lets a caller synchronize with the event thread. The
 * invokeAndWait() method enqueues an event using Display.callSerially() and
 * blocks the caller. When the event reaches the front of the queue, it calls
 * this object's run() method on the event thread, and unblocks the caller.
 *
 * Clients may override the run() method if they wish to provide specialized 
 * behavior that must be run on the event thread. If run() is not overridden, 
 * invokeAndWait() simply blocks until the event thread has processed the 
 * event. This is useful for callers that need to wait until after an event 
 * has been processed.
 *
 * The invokeAndWait() method must not be called on the event thread, 
 * otherwise the system will deadlock.
 */
public class SerialCallback implements Runnable {

    Display dpy;
    Callback callback;
    boolean done;

    /**
     * Constructs this callback object. Requires a Display object upon which 
     * the callSerially() method is to be invoked.
     */
    public SerialCallback(Display dpy) {
        this.dpy = dpy;
        callback = new Callback();
    }
    

    /**
     * Blocks the caller until the events currently in the event queue have 
     * been processed, calls the run() method on the event thread, then 
     * unblocks the caller.
     */
    public synchronized void invokeAndWait() {
        dpy.callSerially(callback);
        done = false;

        try {
            while (!done) {
                wait();
            }
        } catch (InterruptedException ignore) { }
    }


    /**
     * Subclassers may override this if they wish to provide any specialized
     * behavior.  The default implementation does nothing.
     */
    public void run() {
    }


    /**
     * Called on the event thread when the callSerially event reaches the
     * front of the queue. Calls the client's run() method and awakens the 
     * thread that had called invokeAndWait().
     */
    synchronized void called() {
        run();
        done = true;
        notifyAll();
    }


    /**
     * A nested class that provides a run() method to callSerially(), distinct 
     * from the run() method that can be overridden by clients.
     */
    class Callback implements Runnable {
        public void run() {
            called();
        }
    }
}
