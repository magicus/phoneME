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
 * A thread that sends the given message on the given link.
 */
class Sender extends Thread {
    public static final long TIMEOUT = 1000L;

    Link link;
    LinkMessage msg;
    boolean done;
    Throwable exception;

    /**
     * Constructs a new Sender, starts its thread, and waits to give the new
     * thread a chance to block in send().
     */
    Sender(Link newlink, LinkMessage newmsg) {
        link = newlink;
        msg = newmsg;
        done = false;
        exception = null;
        start();
        Utils.sleep(50);
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
     * Sends a message and notifies when done, capturing any exceptions.
     */
    public void run() {
        try {
            link.send(msg);
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
     * A completion callback. Called after the thread returns from the send()
     * call. The msg parameter contains the message sent.  The thr parameter
     * contains any Throwable caught, or null there was none.  This is
     * intended to be overridden by a subclass. The default implementation
     * does nothing.
     */
    public void completed(LinkMessage msg, Throwable thr) {
    }

}
