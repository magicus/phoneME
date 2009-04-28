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

package com.sun.j2me.content;

/**
 * Thread to monitor pending invocations and notify a listener
 * when a matching one is present.
 */
class RequestListenerImpl implements Runnable {

    /** ContenHandlerImpl for which this is listening. */
    private final ContentHandlerImpl handler;

    /** The active thread processing the run method. */
    private ThreadEx listenerThread;

    /**
     * Create a new listener for pending invocations.
     *
     * @param handler the ContentHandlerImpl to listen for
     * @param listener the listener to notify when present
     */
    RequestListenerImpl(ContentHandlerImpl handler) {
		this.handler = handler;
		activate(true);
    }

    /**
     * Set the listener to be notified and start/stop the monitoring
     * thread as necessary.
     * If the listener is non-null make sure there is a thread active
     * to monitor it.
     * If there is no listener, then stop the monitor thread.
     * Unblock any blocked threads so they can get the updated listener.
     * @param listener the listener to update
     */
    void activate(boolean activate) {

		if (activate) {
		    // Ensure a thread is running to watch for it
		    if (listenerThread == null || !listenerThread.isAlive()) {
				listenerThread = new ThreadEx(this);
				listenerThread.start();
		    }
		} else {
		    // Forget the thread doing the listening; it will exit
			if( AppProxy.LOGGER != null )
				AppProxy.LOGGER.println("stop listening ...");
			ThreadEx t = listenerThread;
		    listenerThread = null;
			if( t != null && t.isAlive() ) t.unblock();
		}
	
		/*
		 * Reset notified flags on pending requests.
		 */
		InvocationImpl.store.resetListenNotifiedFlag(handler.applicationID, true);
    }

    /**
     * The run method checks for pending invocations for a
     * desired ContentHandler or application.
     * When an invocation is available the listener is
     * notified.
     */
    public void run() {
		if( AppProxy.LOGGER != null )
			AppProxy.LOGGER.println("listener thread started");
		final Thread mythread = Thread.currentThread();
		while (mythread == listenerThread) {
		    // Wait for a matching invocation
		    boolean pending = InvocationImpl.store.waitForEvent(handler.applicationID, true, 
		    											listenerThread.blockID);
		    if (pending) {
		    	handler.requestNotify();
		    }
		}
		if( AppProxy.LOGGER != null )
			AppProxy.LOGGER.println("listener thread stopped");
    }
}
