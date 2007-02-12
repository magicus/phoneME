/*
 * %W% %E%
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
 */

package com.sun.jumpimpl.process;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.message.JUMPMessageReceiveQueue;
import com.sun.jump.message.JUMPMessageRegistration;
import com.sun.jump.message.JUMPTimedOutException;

import com.sun.jump.os.JUMPOSInterface;
import com.sun.jumpimpl.os.JUMPMessageQueueInterfaceImpl;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

/**
 * A generic JUMPMessageDispatcher implementation.  It can be
 * configured for different underlying messaging implementations
 * by constructing it with the proper JUMPMessageReceiveQueueFactory.
 */
public class JUMPMessageDispatcherImpl implements JUMPMessageDispatcher
{
    // A JUMPMessageDispatcherImpl has one Listener for each
    // messageType with a registered handler.
    // JUMPMessageDispatcherImpl and Listener are by necessity
    // somewhat intertwined, as explained here.
    // JUMPMessageDispatcherImpl.register() creates Listeners on
    // demand and adds them to listeners.  Listeners are not removed
    // by cancelRegistration(); instead, Listeners remove themselves
    // and exit some time after all their handlers have been cancelled
    // and no other handlers have been registered.  This ensures that
    // at most one thread is ever listening for any messageType, and
    // that no message that has a registered handler will be dropped
    // since there will always be a Listener running for that
    // messageType.  Both JUMPMessageDispatcherImpl and Listener
    // synchronize on lock while accessing listeners.  Additionally,
    // Listener synchronizes on lock when accessing Listener.handlers.
    // It could synchronize on itself, but in most cases we already
    // need to synchronize on lock, so using lock for everything is
    // simpler.  We never block while holding lock and there shouldn't
    // be much if any contention for it.

    private static JUMPMessageDispatcherImpl INSTANCE = null;

    // listeners maps String messageType to Listener.
    // Guarded by lock.
    // Invariant: If there is a mapping from messageType to a Listener,
    // then there is a viable listener with a thread running.  If there
    // is a mapping from messageType to null, then the messageType
    // was passed to createJUMPMessageReceiveQueue and the returned
    // JUMPMessageReceiveQueue has not yet been closed.  If there is
    // no mapping, then the messageType is not in use.
    private final Map listeners = new HashMap();
    private final Object lock = new Object();

    private final JUMPMessageReceiveQueueFactory
	jumpMessageReceiveQueueFactory;

    public static synchronized JUMPMessageDispatcherImpl getInstance() 
    {
	if (INSTANCE == null) {
	    INSTANCE = new JUMPMessageDispatcherImpl(
		JUMPMessageReceiveQueueImpl.createFactory());
	}
	return INSTANCE;
    }

    /**
     * Constructs a JUMPMessageDispatcherImpl which can be configured
     * for different underlying messaging implementations by constructing
     * it with the proper factories.  Currently private until it needs
     * to be made public.
     */
    private JUMPMessageDispatcherImpl (
	JUMPMessageReceiveQueueFactory jumpMessageReceiveQueueFactory)
    {
	this.jumpMessageReceiveQueueFactory = jumpMessageReceiveQueueFactory;
    }

    /**
     * NOTE: the handler will be called in an arbitrary thread.  Use
     * appropriate synchronization.  Handlers may be called in an
     * arbitrary order.  If a handler is registered multiple times, it
     * will be called a corresponding number of times for each
     * message, and must be cancelled a corresponding number of times.
     */
    public JUMPMessageRegistration
    registerHandler(String messageType, JUMPMessageHandler handler)
	throws JUMPMessageDispatcherTypeException
    {
        if (messageType == null) {
            throw new NullPointerException("messageType can't be null");
        }
	if (handler == null) {
            throw new NullPointerException("handler can't be null");
        }

	Listener listener;
	synchronized (lock) {
	    listener = getListener(messageType);
	    // Add the handler while synchronized on lock so that a
	    // new Listener won't exit before the handler is added.
	    // If this fails its ok, the Listener will exit soon if no
	    // other handlers are registered for it.
	    listener.addHandler(handler);
	}

	return new JUMPMessageRegistrationImpl(listener, handler);
    }

    // Externally synchronized on lock.
    private Listener getListener (String messageType)
	throws JUMPMessageDispatcherTypeException
    {
	Listener listener = (Listener) listeners.get(messageType);
	if (listener == null) {
	    if (listeners.containsKey(messageType)) {
		throw new JUMPMessageDispatcherTypeException(
		    "messageType " + messageType +
		    " is already in direct use.");
	    }

	    listener = new Listener(messageType);

	    // Be careful to maintain our invariant (and free
	    // resources) even on OutOfMemoryError, etc.
	    boolean success = false;
	    try {
		listeners.put(messageType, listener);
		listener.start();
		success = true;
	    }
	    finally {
		if (!success) {
		    // Free OS resources.
		    listener.close();
		    // Remove listener from the Map.  This is ok even
		    // if it was never added.
		    listeners.remove(messageType);
		}
	    }
	}
	return listener;
    }

    public JUMPMessageReceiveQueue createJUMPMessageReceiveQueue(
	final String messageType)
	throws JUMPMessageDispatcherTypeException
    {
	synchronized (lock) {
	    if (listeners.containsKey(messageType)) {
		if (listeners.get(messageType) != null) {
		    throw new JUMPMessageDispatcherTypeException(
			"messageType " + messageType +
			" already as a handler registered.");
		}
		else {
		    // This would work fine, but it's disallowed anyway.
		    throw new JUMPMessageDispatcherTypeException(
			"messageType " + messageType +
			" is already in direct use.");
		}
	    }

	    final JUMPMessageReceiveQueue queue =
		createJUMPMessageReceiveQueueInternal(messageType);

	    // Be careful to maintain our invariant (and free
	    // resources) even on OutOfMemoryError, etc.

	    boolean success = false;
	    try {
		// Add a null entry to listeners to indicate this
		// messageType is in use.

		listeners.put(messageType, null);

		// Wrap queue in a JUMPMessageReceiveQueue which will
		// remove the Map entry on close.

		JUMPMessageReceiveQueue wrappedQueue =
		    new JUMPMessageReceiveQueue () {
			private final Object closeLock = new Object();
			private boolean closed = false;

			public JUMPMessage receiveMessage(long timeout)
			    throws JUMPTimedOutException, IOException
			{
			    return queue.receiveMessage(timeout);
			}

			public void close()
			{
			    synchronized (closeLock) {
				if (closed) {
				    return;
				}
				else {
				    closed = true;
				}
			    }
			    queue.close();
			    synchronized(lock) {
				listeners.remove(messageType);
			    }
			}
		    };

		success = true;

		return wrappedQueue;
	    }
	    finally {
		if (!success) {
		    // Free OS resources.
		    queue.close();
		    // Remove null entry from the Map.  This is ok
		    // even if it was never added.
		    listeners.remove(messageType);
		}
	    }
	}
    }

    private JUMPMessageReceiveQueue createJUMPMessageReceiveQueueInternal(
	String messageType) 
    {
	return jumpMessageReceiveQueueFactory.
	    createJUMPMessageReceiveQueue(messageType);
    }

    private static class JUMPMessageRegistrationImpl
	implements JUMPMessageRegistration
    {
        private final Listener listener;
        private final JUMPMessageHandler handler;

        public JUMPMessageRegistrationImpl (
	    Listener listener, JUMPMessageHandler handler)
	{
	    this.listener = listener;
	    this.handler = handler;
	}

	public void cancelRegistration ()
	{
	    listener.removeHandler(handler);
	}
    }

    /*
     * Frequently asked questions:
     * 1. Why doesn't Listener extend Thread?  Extending Thread would
     *    put lots of unnecessary and inappropriate methods into its
     *    API.  It should keep control over those things to itself.
     * 2. How about implementing Runnable then?  The fact that Listener
     *    uses a Thread and/or Runnable is an implementation detail
     *    and shouldn't be exposed in its API.  The inner class
     *    implementing Runnable keeps the implementation private.
     *    Only those methods intended to be called from outside the
     *    class itself are public.
     * 3. How can we get the thread to exit when it's blocking in
     *    JUMPMessageReceiveQueue.receiveMessage()?
     *    There are three choices:
     *    1. Make JUMPMessageReceiveQueue.receiveMessage() interruptible,
     *       and interrupt the thread.  We probably don't want to go there.
     *    2. Send a message that the thread will see and exit on.
     *       This isn't as easy as it sounds since sending messages
     *       may fail, e.g., if the Listener is processing messages
     *       slowly and its queue has filled up.
     *    3. Periodically time out and check for exit.  We do this,
     *       it's simple and effective and doesn't need any extra low-level
     *       support such as interrupt handling, although it doesn't stop
     *       the thread immediately, and requires the thread to wake up
     *       periodically.
     */
    private class Listener
    {
	// Guarded by this.
	private final List handlers = new ArrayList();

	private final String messageType;
	private final JUMPMessageReceiveQueue queue;

	public Listener (String messageType)
	{
	    this.messageType = messageType;
	    queue = createJUMPMessageReceiveQueueInternal(messageType);
	}

	// Externally synchronized on lock.
	public void addHandler (JUMPMessageHandler handler)
	{
	    handlers.add(handler);
	}

	public void removeHandler (JUMPMessageHandler handler)
	{
	    synchronized (lock) {
		handlers.remove(handler);
	    }
	}

	public void start ()
	{
	    Thread thread = new Thread(
		new Runnable() {
		    public void run() {
			try {
			    listen();
			}
			finally {
			    close();
			}
		    }
		});
	    thread.setName(this.getClass().getName() + ": " + messageType);
	    thread.setDaemon(true);
	    thread.start();
	}

	public void close ()
	{
	    queue.close();
	}

	private void listen ()
	{
	    // XXX We should either log Errors and RuntimeExceptions
	    // and continue, or cleanup and make sure they're thrown.
	    while (true) {
		try {
		    JUMPMessage msg = queue.receiveMessage(2000L);
		    dispatchMessage(msg);
		} catch(JUMPTimedOutException e) {
		    // This is normal.  It's time to check for exit.
		} catch(IOException e) {
		    e.printStackTrace();
		}
		synchronized (lock) {
		    if (handlers.isEmpty()) {
			// Remove ourselves from the map and exit.
			listeners.remove(messageType);
			break;
		    }
		}
	    }
	}

	// NOTE: Handlers should not be called while holding our
	// monitor since it can lead to inadvertent deadlocks.
	// However, not synchronizing on "this" here can result in
	// handlers being called even after they've been removed.
	// This is a generally accepted hazard of patterns like this.

	private void dispatchMessage(JUMPMessage msg)
	{
	    JUMPMessageHandler[] handlersSnapshot;

	    // Get a snapsot with the lock held.

	    synchronized (lock) {
		handlersSnapshot = (JUMPMessageHandler[])
		    handlers.toArray(new JUMPMessageHandler[handlers.size()]);
	    }

	    // Call handlers with the lock released.

	    for (int i = 0; i < handlersSnapshot.length; i++) {
		JUMPMessageHandler handler = handlersSnapshot[i];
		try {
		    handler.handleMessage(msg);
		} catch (RuntimeException e) {
		    e.printStackTrace();
		}
	    }
	}
    }
}
