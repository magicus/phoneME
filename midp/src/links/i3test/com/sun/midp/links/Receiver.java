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

package com.sun.midp.links;

/**
 * A thread that receives a message on a given link.
 */
class Receiver extends Thread {
    public static final long TIMEOUT = 1000L;

    Link link;
    LinkMessage msg;
    boolean done;
    Throwable exception;


    /**
     * Constructs a new Receiver that starts a thread that calls receive() on
     * newlink. After starting the thread, sleeps the indicated number of
     * milliseconds to give the new thread a chance to run and call receive()
     * before letting the caller's thread continue.
     */
    Receiver(Link newlink, long sleeptime) {
        link = newlink;
        done = false;
        exception = null;
        start();

        if (sleeptime > 0) {
            Utils.sleep(sleeptime);
        }
    }


    /**
     * Constructs a new Receiver, providing a default sleep time.
     */
    Receiver(Link newlink) {
        this(newlink, 50L);
    }


    /**
     * Waits until the thread finishes or until a timeout has expired.
     */
    public void await() {
        long timeout = System.currentTimeMillis() + TIMEOUT;
        synchronized (this) {
            try {
                while (System.currentTimeMillis() < timeout && !done) {
                    wait(TIMEOUT);
                }
            } catch (InterruptedException ignore) { }
        }
    }

    /**
     * Receives a message and notifies when done, capturing any exceptions.
     */
    public void run() {
        LinkMessage lm;
        try {
            msg = link.receive();
        } catch (Throwable t) {
            exception = t;
        } finally {
            synchronized (this) {
                done = true;
                notifyAll();
            }
        }

        completed(msg, exception);
    }


    /**
     * A completion callback. Called after the thread returns from the
     * receive() call. The msg parameter contains the received message, or
     * null if there was an exception was thrown. The thr parameter contains
     * any Throwable caught, or null there was none.  This is intended to be 
     * overridden by a subclass. The default implementation does nothing.
     */
    public void completed(LinkMessage msg, Throwable thr) {
    }

}
