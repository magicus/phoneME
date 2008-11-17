/*
 * @(#)EventDispatchThread.java	1.31 06/10/10
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

package java.awt;

import java.lang.reflect.Method;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;
import java.lang.ref.WeakReference;
import java.security.PrivilegedAction;

/**
 * EventDispatchThread is a package-private AWT class which takes
 * events off the EventQueue and dispatches them to the appropriate
 * AWT components.
 *
 * The Thread starts a "permanent" event pump with a call to
 * pumpEvents(Conditional) in its run() method. Event handlers can choose to
 * block this event pump at any time, but should start a new pump (<b>not</b>
 * a new EventDispatchThread) by again calling pumpEvents(Conditional). This
 * secondary event pump will exit automatically as soon as the Condtional
 * evaluate()s to false and an additional Event is pumped and dispatched.
 *
 * @author Tom Ball
 * @author Amy Fowler
 * @author Fred Ecks
 * @author David Mendenhall
 *
 * @version 1.34, 02/02/00
 * @since 1.1
 */
class EventDispatchThread extends Thread {
    EventQueueProxy theQueue; // 6261461
    boolean doDispatch = true;
    static final int ANY_EVENT = -1;
    
    EventDispatchThread() { // 6261461
    }
    
    static EventDispatchThread create(String name, EventQueueProxy queue) {
        EventDispatchThread edt = (EventDispatchThread)
                AccessController.doPrivileged(new PrivilegedAction() {
                    public Object run() {
                        try {
                            Class dispatchThreadClass = Class.forName(
                                    System.getProperty(
                                    "java.awt.EventDispatchThread.classname",
                                    "java.awt.EventDispatchThread"));
                            return dispatchThreadClass.newInstance();
                        } catch (Exception e) {
                            return new EventDispatchThread();
                        }
                    }
                });
        edt.setName(name);
        edt.theQueue = queue;
        return edt;
    }

    public void stopDispatching() {
        // Note: We stop dispatching via a flag rather than using
        // Thread.interrupt() because we can't guarantee that the wait()
        // we interrupt will be EventQueue.getNextEvent()'s.  -fredx 8-11-98

        doDispatch = false;
        // fix 4122683, 4128923
        // Post an empty event to ensure getNextEvent is unblocked
        //
        // We have to use postEventPrivate instead of postEvent because
        // EventQueue.pop calls EventDispatchThread.stopDispatching.
        // Calling SunToolkit.flushPendingEvents in this case could
        // lead to deadlock.
        theQueue.postEventPrivate(new EmptyEvent());
        // wait for the dispatcher to complete
        if (Thread.currentThread() != this) {
            try {
                join();
            } catch (InterruptedException e) {}
        }
    }
    class EmptyEvent extends AWTEvent implements ActiveEvent {
        public EmptyEvent() {
            super(EventDispatchThread.this, 0);
        }

        public void dispatch() {}
    }
    public void run() {
        pumpEvents(new Conditional() {
                public boolean evaluate() {
                    return true;
                }
            }
        );
    }

    void pumpEvents(Conditional cond) {
        pumpEvents(ANY_EVENT, cond);
    }

    void pumpEvents(int id, Conditional cond) {
        while (doDispatch && cond.evaluate()) {
            if (isInterrupted() || !pumpOneEvent(id)) {
                doDispatch = false;
            }
        }
    }

    boolean pumpOneEvent(int id) {
        try {
            AWTEvent event = (id == ANY_EVENT) ? theQueue.getNextEvent() :
                theQueue.getNextEvent(id);
            theQueue.dispatchEvent(event);
            return true;
        } catch (ThreadDeath death) {
            return false;
        } catch (InterruptedException interruptedException) {
            return false; // AppContext.dispose() interrupts all
            // Threads in the AppContext

        } catch (Throwable e) {
            if (!handleException(e)) {
                System.err.println(
                    "Exception occurred during event dispatching:");
                e.printStackTrace();
            }
            return true;
        }
    }
    private static final String handlerPropName = "sun.awt.exception.handler";
    private static String handlerClassName = null;
    private static String NO_HANDLER = new String();
    /**
     * Handles an exception thrown in the event-dispatch thread.
     *
     * <p> If the system property "sun.awt.exception.handler" is defined, then
     * when this method is invoked it will attempt to do the following:
     *
     * <ol>
     * <li> Load the class named by the value of that property, using the
     *      current thread's context class loader,
     * <li> Instantiate that class using its zero-argument constructor,
     * <li> Find the resulting handler object's <tt>public void handle</tt>
     *      method, which should take a single argument of type
     *      <tt>Throwable</tt>, and
     * <li> Invoke the handler's <tt>handle</tt> method, passing it the
     *      <tt>thrown</tt> argument that was passed to this method.
     * </ol>
     *
     * If any of the first three steps fail then this method will return
     * <tt>false</tt> and all following invocations of this method will return
     * <tt>false</tt> immediately.  An exception thrown by the handler object's
     * <tt>handle</tt> will be caught, and will cause this method to return
     * <tt>false</tt>.  If the handler's <tt>handle</tt> method is successfully
     * invoked, then this method will return <tt>true</tt>.  This method will
     * never throw any sort of exception.
     *
     * <p> <i>Note:</i> This method is a temporary fix to work around the
     * absence of a real API that provides the ability to replace the
     * event-dispatch thread.  The magic "sun.awt.exception.handler" property
     * <i>will be removed</i> in a future release.
     *
     * @param  thrown  The Throwable that was thrown in the event-dispatch
     *                 thread
     *
     * @returns  <tt>false</tt> if any of the above steps failed, otherwise
     *           <tt>true</tt>.
     */
    boolean handleException(Throwable thrown) {
        try {
            if (handlerClassName == NO_HANDLER) {
                return false;   /* Already tried, and failed */
            }
            /* Look up the class name */
            if (handlerClassName == null) {
                handlerClassName = ((String) AccessController.doPrivileged(
                                new GetPropertyAction(handlerPropName)));
                if (handlerClassName == null) {
                    handlerClassName = NO_HANDLER; /* Do not try this again */
                    return false;
                }
            }
            /* Load the class, instantiate it, and find its handle method */
            Method m;
            Object h;
            try {
                ClassLoader cl = Thread.currentThread().getContextClassLoader();
                Class c = Class.forName(handlerClassName, true, cl);
                m = c.getMethod("handle", new Class[] { Throwable.class }
                        );
                h = c.newInstance();
            } catch (Throwable x) {
                handlerClassName = NO_HANDLER; /* Do not try this again */
                return false;
            }
            /* Finally, invoke the handler */
            m.invoke(h, new Object[] { thrown }
            );
        } catch (Throwable x) {
            return false;
        }
        return true;
    }

    boolean isDispatching(EventQueue eq) {
        // note : getQueue() can return null, hence we changed the order 
        return eq.equals(theQueue.getQueue()); // 6261461
    }

    EventQueue getEventQueue() {
        return theQueue.getQueue(); // 6261461 this can be null
    }
}

// 6261461
// Ideally EventQueueProxy should be defined in EventQueue class, since it
// accesses package protected variables. This is defined here since we
// share this file between basis and personal and the implementation of the
// methods are identical. This helps us in ease of maintaining the proxy class.
/**
 */
class EventQueueProxy {
    WeakReference eventQueueRef ;
    int waitForID;

    EventQueueProxy(EventQueue eq) {
        this.eventQueueRef = new WeakReference(eq);
    }

    void postEventPrivate(AWTEvent theEvent) {
        EventQueue eq = getQueue();
        if ( eq != null ) 
            eq.postEventPrivate(theEvent);
        else {
            // the event queue must have been collected, so call notify
            // so that any thread blocked on getNextEvent() can unblock
            synchronized(this) {
                this.notifyAll();
            }
        }
    }

    public synchronized AWTEvent getNextEvent() throws InterruptedException {
        do {
            EventQueue eq = getQueue();
            if ( eq == null ) 
                throw new InterruptedException();
            for (int i = EventQueue.NUM_PRIORITIES - 1; i >= 0; i--) {
                if (eq.queues[i].head != null) {
                    EventQueueItem eqi = eq.queues[i].head;
                    eq.queues[i].head = eqi.next;
                    if (eqi.next == null) {
                        eq.queues[i].tail = null;
                    }
                    return eqi.event;
                }
            }
            // dont reference event queue since we are going to wait.
            // this will allow gc on the event queue.
            eq = null;
            wait();
        }
        while (true);
    }

    AWTEvent getNextEvent(int id) throws InterruptedException {
        do {
            EventQueue eq = getQueue();
            if ( eq == null ) 
                throw new InterruptedException();
            synchronized(this) {
                for (int i = 0; i < EventQueue.NUM_PRIORITIES; i++) {
                    for (EventQueueItem entry = eq.queues[i].head, prev = null;
                         entry != null; prev = entry, entry = entry.next) {
                        if (entry.id == id) {
                            if (prev == null) {
                                eq.queues[i].head = entry.next;
                            } else {
                                prev.next = entry.next;
                            }
                            if (eq.queues[i].tail == entry) {
                                eq.queues[i].tail = prev;
                            }
                            return entry.event;
                        }
                    }
                }
                // dont reference event queue since we are going to wait.
                // this will allow gc on the event queue.
                eq = null; 
                this.waitForID = id;
                wait();
                this.waitForID = 0;
            }
        } while(true);
    }

    void dispatchEvent(AWTEvent event) {
        EventQueue eq = getQueue();
        if ( eq != null ) 
            eq.dispatchEvent(event);
    }

    EventQueue getQueue() {
        return (EventQueue)(EventQueue)eventQueueRef.get();
    }
}
// 6261461
