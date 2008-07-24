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
import com.sun.jump.message.JUMPTimedOutException;
import com.sun.jump.message.JUMPUnblockedException;

import com.sun.jump.os.JUMPOSInterface;
import com.sun.jumpimpl.os.JUMPMessageQueueInterfaceImpl;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

/**
 * A generic JUMPMessageDispatcher implementation.
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
    // and exit some time after all their handlers have been canceled
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

    // A JUMPMessageDispatcherImpl has one DirectRegistration for each
    // messageType with at least one outstanding registration.  We
    // must be careful to unreserve a messageType only when it is no
    // longer in use and can no longer be used, otherwise the
    // low-level code could use memory that it has freed, which would
    // be very un-Java.  With the messageType -> DirectRegistration
    // required by the API, we need to keep a use count and do other
    // gymnastics to ensure this.

    private static final JUMPMessageQueueInterfaceImpl
	jumpMessageQueueInterfaceImpl = (JUMPMessageQueueInterfaceImpl)
	JUMPOSInterface.getInstance().getQueueInterface();

    private static JUMPMessageDispatcherImpl INSTANCE = null;

    // directRegistrations maps String messageType to DirectRegistration.
    // Guarded by lock.
    // Invariant: If there is a mapping from messageType to a
    // DirectRegistration, then at least one registration is still
    // outstanding for the messageType, otherwise no registrations
    // are outstanding.

    private final Map directRegistrations = new HashMap();

    // listeners maps String messageType to Listener.
    // Guarded by lock.
    // Invariant: If there is a mapping from messageType to a Listener,
    // then the Listener is active, otherwise there is no Listener.

    private final Map listeners = new HashMap();

    // lock guards both directRegistrations and listeners.  We need
    // one lock so we can tell whether a messageType is registered one
    // way or the other without races.

    private final Object lock = new Object();

    public static synchronized JUMPMessageDispatcherImpl getInstance() 
    {
	if (INSTANCE == null) {
	    INSTANCE = new JUMPMessageDispatcherImpl();
	}
	return INSTANCE;
    }

    /**
     * Construction allowed only by getInstance().
     */
    private JUMPMessageDispatcherImpl ()
    {
    }

    public Object registerDirect(String messageType)
	throws JUMPMessageDispatcherTypeException, IOException
    {
	DirectRegistration directRegistration;
	synchronized (lock) {
	    if (listeners.containsKey(messageType)) {
		throw new JUMPMessageDispatcherTypeException(
		    "Type " + messageType +
		    " already registered with handlers");
	    }

	    directRegistration = getDirectRegistration(messageType);
	    directRegistration.incrementUseCount();
	}

	return new DirectRegistrationToken(directRegistration);
    }

    // Externally synchronized on lock.
    private DirectRegistration getDirectRegistration (String messageType)
	throws IOException
    {
	DirectRegistration directRegistration =
	    (DirectRegistration) directRegistrations.get(messageType);

	if (directRegistration == null) {
	    directRegistration = new DirectRegistration(messageType);

	    // Be careful to maintain our invariant (and free
	    // resources) even on OutOfMemoryError, etc.

	    boolean success = false;
	    try {
		directRegistrations.put(messageType, directRegistration);
		success = true;
	    }
	    finally {
		if (!success) {
		    // Free OS resources.
		    directRegistration.close();
		}
	    }
	}

	return directRegistration;
    }

    public JUMPMessage waitForMessage(String messageType, long timeout)
        throws JUMPMessageDispatcherTypeException, JUMPTimedOutException, IOException
    {
	DirectRegistration directRegistration;
	synchronized (lock) {
	    directRegistration =
		(DirectRegistration) directRegistrations.get(messageType);
	    if (directRegistration == null) {
		throw new JUMPMessageDispatcherTypeException(
		    "Type " + messageType +
		    " not registered for direct listening");
	    }
	    directRegistration.incrementUseCount();
	}

	try {
	    return doWaitForMessage(messageType, timeout);
	}
	finally {
	    directRegistration.decrementUseCountMaybeClose();
	}
    }

    /**
     * @throws JUMPTimedOutException
     * @throws JUMPUnblockedException
     * @throws IOException
     */
    private JUMPMessage doWaitForMessage(String messageType, long timeout)
        throws JUMPTimedOutException, IOException 
    {
	byte[] raw = jumpMessageQueueInterfaceImpl.receiveMessage(
	    messageType, timeout);
	return new MessageImpl.Message(raw);
    }

    /**
     * NOTE: the handler will be called in an arbitrary thread.  Use
     * appropriate synchronization.  Handlers may be called in an
     * arbitrary order.  If a handler is registered multiple times, it
     * will be called a corresponding number of times for each
     * message, and must be canceled a corresponding number of times.
     */
    public Object
    registerHandler(String messageType, JUMPMessageHandler handler)
	throws JUMPMessageDispatcherTypeException, IOException
    {
        if (messageType == null) {
            throw new NullPointerException("messageType can't be null");
        }
	if (handler == null) {
            throw new NullPointerException("handler can't be null");
        }

	Listener listener;
	synchronized (lock) {
	    if (directRegistrations.containsKey(messageType)) {
		throw new JUMPMessageDispatcherTypeException(
		    "Type " + messageType +
		    " already registered for direct listening");
	    }

	    listener = getListener(messageType);

	    // Add the handler while synchronized on lock so that a
	    // new Listener won't exit before the handler is added.
	    // If this fails its ok, the Listener will exit soon if no
	    // other handlers are registered for it.

	    listener.addHandler(handler);
	}

	return new HandlerRegistrationToken(listener, handler);
    }

    // Externally synchronized on lock.
    private Listener getListener (String messageType)
	throws IOException
    {
	Listener listener = (Listener) listeners.get(messageType);
	if (listener == null) {
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

    public void cancelRegistration(Object registrationToken)
	throws IOException
    {
	((RegistrationToken)registrationToken).cancelRegistration();
    }

    private interface RegistrationToken
    {
	void cancelRegistration() throws IOException;
    }

    private static class HandlerRegistrationToken
	implements RegistrationToken
    {
        private final Listener listener;
        private final JUMPMessageHandler handler;

	// Don't allow cancelRegistration() to be called twice.
	private boolean canceled = false;

        public HandlerRegistrationToken (
	    Listener listener, JUMPMessageHandler handler)
	{
	    this.listener = listener;
	    this.handler = handler;
	}

	public void cancelRegistration ()
	    throws IOException
	{
	    synchronized (this) {
		if (canceled) {
		    throw new IllegalStateException(
			"Registration has already been canceled.");
		}
		canceled = true;
	    }

	    listener.removeHandler(handler);
	}
    }

    private static class DirectRegistrationToken
	implements RegistrationToken
    {
	private final DirectRegistration directRegistration;

	// Don't allow cancelRegistration() to be called twice.
	private boolean canceled = false;

        public DirectRegistrationToken (DirectRegistration directRegistration)
	{
	    this.directRegistration = directRegistration;
	}

	public void cancelRegistration ()
	{
	    synchronized (this) {
		if (canceled) {
		    throw new IllegalStateException(
			"Registration has already been canceled.");
		}
		canceled = true;
	    }
	    directRegistration.decrementUseCountMaybeClose();
	}
    }

    private class DirectRegistration
    {
	private final String messageType;

	// useCount is incremented for every direct registration of
	// messageType and when a message receive begins, and
	// decremented when the registration is canceled or a message
	// read is finished.  When the count falls to zero, the
	// low-level resources are freed, and the directRegistrations
	// mapping is removed, therefore this DirectRegistration can
	// never be used again to access the (freed) low-level
	// resources.

	private int useCount = 0;

	public DirectRegistration (String messageType)
	    throws IOException
	{
	    this.messageType = messageType;
	    // Make sure we've got a receive queue for the messageType.
	    jumpMessageQueueInterfaceImpl.reserve(messageType);
	}

	// Externally synchronized on lock.
	public void incrementUseCount ()
	{
	    useCount++;
	}

	public void decrementUseCountMaybeClose ()
	{
	    synchronized (lock) {
		useCount--;
		if (useCount == 0) {
		    close();
		    directRegistrations.remove(messageType);
		}
	    }
	}

	public void close ()
	{
	    // Tell the low-level code we're done with the message queue.
	    jumpMessageQueueInterfaceImpl.unreserve(messageType);
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
     *       slowly and its queue has filled up.  But in that case
     *       it should exit after reading one of the "real" messages
     *       whether our sentinel is sent/received or not.
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

	public Listener (String messageType)
	    throws IOException
	{
	    this.messageType = messageType;
	    // Make sure we've got a receive queue for the messageType.
	    jumpMessageQueueInterfaceImpl.reserve(messageType);
	}

	// Externally synchronized on lock.
	public void addHandler (JUMPMessageHandler handler)
	{
	    handlers.add(handler);
	}

	public void removeHandler (JUMPMessageHandler handler)
	    throws IOException
	{
	    synchronized (lock) {
		handlers.remove(handler);
		if (handlers.isEmpty()) {
		    // Wake up the listening thread so it can exit if
		    // it finds handlers is still empty.
		    jumpMessageQueueInterfaceImpl.unblock(messageType);
		}
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
	    // Tell the low-level code we're done with the message queue.
	    jumpMessageQueueInterfaceImpl.unreserve(messageType);
	}

	private void listen ()
	{
	    // FIXME We should either log Errors and RuntimeExceptions
	    // and continue, or cleanup and make sure they're thrown.
	    while (true) {
		try {
		    JUMPMessage msg = doWaitForMessage(messageType, 0L);
		    dispatchMessage(msg);
		} catch (JUMPUnblockedException e) {
		    // This is normal.  It's time to check for exit.
		} catch (JUMPTimedOutException e) {
		    // This shouldn't happen.  Handle like IOException.
		} catch (IOException e) {
		    // Unexpected exception.
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
