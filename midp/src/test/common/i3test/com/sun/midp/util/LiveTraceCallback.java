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
 * Provides a rendezvous point and synchronous callback within a
 * LiveTraceListener. An instance of LiveTraceCallback is installed into a
 * LiveTracer tracepoint like any other LiveTraceListener. Once installed, the
 * LiveTrace code calls a listener here, which synchronously calls a callback
 * established by a test making a call to await(). If a callback isn't
 * available, the trace listener blocks until one becomes available. After the
 * callback returns, it is removed. Therefore, while the listener is active,
 * <b>every</b> trace point must be matched by a call to await(). Otherwise,
 * the thread being traced will block and all the tests will timeout.
 *
 * Sample usage from an i3test:
 *
 * <code>
 *     tcb = new LiveTraceCallback();
 *     targetObject.liveTracer.add(TRACE_TAG, tcb);
 *     // initiate some operation that will eventually hit TRACE_TAG
 *     assertTrue(
 *         "trace point was hit",
 *         tcb.await(
 *             new Runnable() {
 *                 public void run() {
 *                     // stuff executed by the traced thread
 *                 }
 *             })
 *         );
 * </code>
 */
public class LiveTraceCallback implements LiveTraceListener {

    /**
     * The default timeout period, in milliseconds.
     */
    public static final long DEFAULT_TIMEOUT = 2000L;

    boolean active = true;
    Runnable callback = null;
    Runnable doNothing = new Runnable() { public void run() { } };


    /**
     * Called by Display's LiveTracer object after a screen change occurs.
     */
    public synchronized void call(String tag) {
        if (!active) {
            return;
        }

        while (active && callback == null) {
            try {
                wait();
            } catch (InterruptedException ignore) { }
        }

        if (active) {
            callback.run();
        }

        callback = null;
        notifyAll();
    }


    /**
     * Blocks until a tracepoint is reached, causes r.run() to be called
     * synchronously by the traced thraed, and then lets both the traced
     * thread and the caller continue. Times out after the indicated timeout
     * period. If a timeout or interrupt occurs, the callback is cleared
     * without being called.
     *
     * @param r the Runnable whose run() method is to be called
     * @param timeout timeout period in milliseconds
     * @return true if the tracepoint was handled normally, false if a timeout 
     * or interrupt occurred
     */
    public synchronized boolean await(Runnable r, long timeout) {
        long deadline = System.currentTimeMillis() + timeout;
        long remain = timeout;
        boolean retval = true;

        callback = r;
        notifyAll();

        try {
            while (callback != null && remain > 0) {
                wait(remain);
                remain = deadline - System.currentTimeMillis();
            }

            if (callback != null) {
                System.out.println("LiveTraceCallback.await: timed out");
                retval = false;
            }
        } catch (InterruptedException ie) {
            System.out.println("LiveTraceCallback.await: interrupted");
            retval = false;
        } finally {
            callback = null;
        }

        return retval;
    }


    /**
     * Same as await(r, DEFAULT_TIMEOUT).
     */
    public boolean await(Runnable r) {
        return await(r, DEFAULT_TIMEOUT);
    }


    /**
     * Same as await(<i>do nothing</i>, DEFAULT_TIMEOUT).
     */
    public boolean await() {
        return await(doNothing, DEFAULT_TIMEOUT);
    }


    /**
     * Shuts down this trace listener by arranging for any listener not to
     * block and instead to be ignored. Any blocked listener is unblocked
     * and returns without calling the callback, even if one was currently
     * installed.
     */
    public synchronized void shutdown() {
        active = false;
        notifyAll();
    }
}
