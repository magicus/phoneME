/*
 * @(#)Worker.java	1.10 06/10/10
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

package com.sun.xlet.ixc;

import java.rmi.RemoteException;

/**
 * A Worker is an object that takes requests in one thread and executes
 * them in another.  For correctness reasons, it's important that a Worker
 * appear to allocate a different thread for each request -- it must be
 * possible to have an unlimited number of concurrently executing requests.
 */

class Worker implements Runnable {
    // This implementation keeps one thread around at all times.  If a call
    // comes in and this thread is busy, it creates a new thread just for
    // that second call.  The expectation is that concurrently executing
    // calls are relatively rare, and that thread creation is relatively
    // expensive.

    private Thread thread = null;
    private Runnable work = null;
    private Object ticket = new Object();   // Null means thread is available
    private boolean destroyed = false;
    private String threadName;
    private int threadNo = 0;
    Worker(String threadName) {
        this.threadName = threadName;
    }

    synchronized void destroy() {
        destroyed = true;
        work = null;
        ticket = null;
        notifyAll();
    }

    //
    // called by execcuteForClient() with the lock held
    //
    private void createThread() throws RemoteException {
        Object t = ticket;
        thread = new Thread(this, threadName);
        thread.start();
        while (t == ticket) {
            try {
                wait();
            } catch (InterruptedException ex) {
                throw new RemoteException("Thread interrupted", ex);
            }
        }
    }

    void execute(Runnable r) throws RemoteException {
        boolean busy;
        synchronized (this) {
            if (destroyed) {
                throw new RemoteException("Remote xlet has been killed");
            }
            if (thread == null) {
                createThread();
            }
            busy = ticket != null;
            if (!busy) {
                Object t = new Object();
                ticket = t;
                work = r;
                notifyAll();
                try {
                    do {
                        wait();
                    }
                    while (t == ticket);
                } catch (InterruptedException ex) {
                    throw new RemoteException("Worker thread interruped", ex);
                }
            }
        }
        if (busy) {
            Thread t = new Thread(r, threadName + " subthread " + (++threadNo));
            t.start();
            try {
                t.join();
            } catch (InterruptedException ex) {
                throw new RemoteException("Worker thread interruped", ex);
            }
        }
    }

    //
    // The method that thread runs
    //
    public void run() {
        Runnable r = null;
        for (;;) {
            synchronized (this) {
                // assert (ticket != null)
                work = null;
                ticket = null;
                notifyAll();
                do {
                    if (destroyed) {
                        return;
                    }
                    try {
                        wait();
                    } catch (InterruptedException ex) {// assert(false)
                        // If assertions are off, ignore.
                    }
                    r = work;
                }
                while (r == null);
                // assert ticket != null
            }
            try {
                r.run();
            } catch (Throwable t) {
                t.printStackTrace();
                // assert(false);
            }
        }
    }
}
