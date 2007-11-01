/*
 * @(#)WorkerThreadPool.java	1.3 06/08/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.ixc;

/**
 * A WorkerThreadPool executes a Runnable in the background.  For efficiency, a
 * simple thread pool is maintained.  Threads are created when needed,
 * and a thread with no work to do for ten seconds is destroyed.
 * <p>
 * It would make sense to use this WorkerThreadPool class in the app manager,
 * e.g. for xlet lifecycle control, since sending those events is rare.
 * <p>
 * This class assumes that creation and destruction of java.lang.Thread
 * instances is relatively expensive.  Of course, java.lang.Thread might
 * do its own pooling of underlying native threads; if this is so,
 * then it may be somewhat more efficient to just create a new
 * java.lang.Thread for each call.
 */

public class WorkerThreadPool implements Runnable {

    private static int TIMEOUT = 10000;		// Timeout after 10 seconds
    private static Object LOCK = new Object();
    private static int num = 1;	   // For unique names, which helps debugging

    // Linked list of available workers, with the most recently used
    // worker first.
    private static WorkerThreadPool available = null;

    private Thread thread;
    private Runnable work = null;
    private WorkerThreadPool next = null;

    private static int counter = 0;
    private static int LIMIT = 10; // Max number of threads.

    /**
     * Run r in the background, returning immediatedly.
     *
     * @param priority	Thread priority for this task
     * @param r		Task to run
     **/
    public static void execute(int priority, Runnable r) {
	WorkerThreadPool w;
        while (counter > LIMIT && available == null) {
            try {
                Thread.currentThread().wait(1000);
            } catch (Exception e) {}
        } 
	synchronized(LOCK) {
            if (available == null) {
		w = new WorkerThreadPool();
                counter++;
	    } else {
		w = available;
		available = available.next;
	    }
	    w.thread.setPriority(priority);
	    w.work = r;
	    // We can't synchronize on w here, because the locking
	    // order is first WorkerThreadPool lock, then global lock.
	}
	synchronized(w) {
	    w.notifyAll();
	}
    }

    private WorkerThreadPool() {
	int n = num++;
        thread = new Thread(this, "WorkerThreadPool-" + n);
	thread.setPriority(9);
	thread.setDaemon(true);
        thread.start();
    }

    /**
     * This is an internal method of WorkerThreadPool, and should not
     * be called by clients.
     **/
    public void run() {
        Runnable r = null;
	long lastUsed = System.currentTimeMillis();
        for (;;) {
            synchronized (this) {
		while (work == null) {
		    long tm = lastUsed + TIMEOUT - System.currentTimeMillis();
		    if (tm <= 0L) {	// timed out
			synchronized(LOCK) {
			    if (work == null) {
	//System.out.println("@@ " + thread + " unused, terminating.");
				if (available == this) {
				    available = this.next;
				} else if (available == null) {
				    // Shouldn't happen
				} else {
				    WorkerThreadPool w = available;
				    while (w != null) {
					if (w.next == this) {
					    w.next = this.next;
					    break;
					} else {
					    w = w.next;
					}
				    }
				}
                                counter--;
				return;
			    }
			}
		    } else {
			try {
			    wait(tm);
			} catch (InterruptedException ex) {
			    // ignore.
			}
		    }
                }
		r = work;
            }
            try {
                r.run();
            } catch (Throwable t) {
                t.printStackTrace();
            } finally {
		synchronized(this) {
		    work = null;
		    lastUsed = System.currentTimeMillis();
		    thread.setPriority(9);
		    synchronized(LOCK) {
			this.next = available;
			available = this;
		    }
		}
	    }
        }
    }
}
