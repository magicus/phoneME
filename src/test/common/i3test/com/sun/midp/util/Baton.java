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

/**
 * A concurrency control mechanism, a virtual baton that can be passed between
 * two threads to control the order of execution.  The main operation is
 * pass(), which unblocks the other thread and then blocks this thread until
 * the other thread passes the baton back.
 *
 * A typical use case is as follows:
 * 
 * 1. Thread A creates the baton and passes it to thread B.
 * 2. Thread A calls start() to wait for thread B to get to a known state.
 * 3. Thread B calls pass() to pass the baton to A, and then B blocks.
 * 4. Thread A unblocks does stuff, then calls pass() to pass the baton
 *    to B, and then A blocks.
 * 4. Threads A and B alternately call pass() so that they execute in
 *    lock step.
 * 5. One of the threads calls finish() to let the other thread continue
 *    indefinitely, ending the use of the baton.
 *
 * The baton can be passed between threads up to Integer.MAX_VALUE - 1 times 
 * before it ceases to function.
 */
public class Baton {
    int count;

    /**
     * Creates a new baton.
     */
    public Baton() {
        count = 0;
    }

    /**
     * Starts the protocol by waiting for the first pass from the other 
     * thread.
     *
     * @throws IllegalStateException if called after the protocol has been 
     * started
     */
    public synchronized void start() {
        if (count > 1) {
            throw new IllegalStateException();
        }
        await(1);
    }

    /**
     * Passes the baton to the other thread, unblocking it, and blocks until 
     * the baton is passed back. After finish() is called, simply returns 
     * without doing anything.
     */
    public synchronized void pass() {
        if (count < Integer.MAX_VALUE) {
            count++;
            notifyAll();
            await(count + 1);
        }
    }

    /**
     * Tells the other thread continue, and then returns. After a call to 
     * finish(), the baton becomes inoperative, and calls to pass() simply 
     * return.
     */
    public synchronized void finish() {
        count = Integer.MAX_VALUE;
        notifyAll();
    }

    private void await(int seq) {
        while (count < seq) {
            try {
                wait();
            } catch (InterruptedException ignore) { }
        }
    }
}
