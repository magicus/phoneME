/*
 * @(#)EventQueue.java	1.19 06/10/10
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

import java.awt.event.PaintEvent;
import java.awt.event.InvocationEvent;
import java.awt.event.KeyEvent;
import java.awt.event.ActionEvent;
import java.awt.event.FocusEvent;
import java.awt.event.InputEvent;
import java.awt.event.WindowEvent;
import java.awt.ActiveEvent;
import java.util.EmptyStackException;
import java.lang.reflect.InvocationTargetException;
import java.lang.ref.WeakReference;

import sun.awt.AppContext;
import sun.awt.SunToolkit;

/*
 * Fix for 6370528 - EventQueue race condition or synchronization bug
 * Both the EventQueue class and EventQueueProxy class were used for
 * synchronization, which caused the race condition. Fixed by changing
 * all synchronization using the EventQueueProxy class, since that is
 * the class that contains the core logic of EventQueue after the
 * refactoring to fix 6261461
 */

/**
 * EventQueue is a platform-independent class that queues events, both
 * from the underlying peer classes and from trusted application classes.
 * There is only one EventQueue for each AppContext.
 *
 * @author Thomas Ball
 * @author Fred Ecks
 * @author David Mendenhall
 *
 * @version 	1.67, 02/11/00
 * @since 	1.1
 */
public class EventQueue {
    // From Thread.java
    private static int threadInitNumber;
    private static synchronized int nextThreadNum() {
        return threadInitNumber++;
    }
    private static final int LOW_PRIORITY = 0;
    private static final int NORM_PRIORITY = 1;
    private static final int HIGH_PRIORITY = 2;
    static final int NUM_PRIORITIES = HIGH_PRIORITY + 1; // 6261461

    /*
     * We maintain one Queue for each priority that the EventQueue supports.
     * That is, the EventQueue object is actually implemented as
     * NUM_PRIORITIES queues and all Events on a particular internal Queue
     * have identical priority. Events are pulled off the EventQueue starting
     * with the Queue of highest priority. We progress in decreasing order
     * across all Queues.
     */
    Queue[] queues = new Queue[NUM_PRIORITIES]; // 6261461

    /*
     * The next EventQueue on the stack, or null if this EventQueue is
     * on the top of the stack.  If nextQueue is non-null, requests to post
     * an event are forwarded to nextQueue.
     */
    private EventQueue nextQueue;
    /*
     * The previous EventQueue on the stack, or null if this is the
     * "base" EventQueue.
     */
    private EventQueue previousQueue;
    private EventDispatchThread dispatchThread;
    /*
     * Debugging flag -- set true and recompile to enable checking.
     */
    private final static boolean debug = false;

    private WeakReference currentEvent;
    private long mostRecentEventTime = System.currentTimeMillis();

    private EventQueueProxy proxy = null ; // 6261461
    public EventQueue() {
        for (int i = 0; i < NUM_PRIORITIES; i++) {
            queues[i] = new Queue();
        }
        String name = "AWT-EventQueue-" + nextThreadNum();
        dispatchThread = EventDispatchThread.create(name, 
                             this.proxy = new EventQueueProxy(this)); // 6261461 
        
        dispatchThread.setPriority(Thread.NORM_PRIORITY + 1);
        dispatchThread.start();
    }

    /**
     * Post a 1.1-style event to the EventQueue.  If there is an
     * existing event on the queue with the same ID and event source,
     * the source Component's coalesceEvents method will be called.
     *
     * @param theEvent an instance of java.awt.AWTEvent, or a
     * subclass of it.
     */
    public void postEvent(AWTEvent theEvent) {
        postEventPrivate(theEvent);
    }

    /**
     * Post a 1.1-style event to the EventQueue.  If there is an
     * existing event on the queue with the same ID and event source,
     * the source Component's coalesceEvents method will be called.
     *
     * @param theEvent an instance of java.awt.AWTEvent, or a
     * subclass of it.
     */
    final void postEventPrivate(AWTEvent theEvent) {
        synchronized (this.proxy) { // 6261461
            int id = theEvent.getID();
            if (nextQueue != null) {
                // Forward event to top of EventQueue stack.
                nextQueue.postEventPrivate(theEvent);
            } else if (id == PaintEvent.PAINT ||
                id == PaintEvent.UPDATE) {
                postEvent(theEvent, LOW_PRIORITY);
            } else {
                postEvent(theEvent, NORM_PRIORITY);
            }
        }
    }

    /**
     * Posts the event to the internal Queue of specified priority,
     * coalescing as appropriate.
     */
    private void postEvent(AWTEvent theEvent, int priority) {
        EventQueueItem newItem = new EventQueueItem(theEvent);
        boolean notifyID = (theEvent.getID() == this.proxy.waitForID);//6261461
        if (queues[priority].head == null) {
            boolean shouldNotify = noEvents();
            queues[priority].head = queues[priority].tail = newItem;
            // This component doesn't have any events of this type on the
            // queue, so we have to initialize the RepaintArea with theEvent
            if (theEvent.getID() == PaintEvent.PAINT ||
                theEvent.getID() == PaintEvent.UPDATE) {
                Object source = theEvent.getSource();
                ((Component) source).coalesceEvents(theEvent, theEvent);
            }
            if (shouldNotify) {
                proxy.notifyAll(); // 6261461
            }
            if (notifyID) {
                proxy.notifyAll(); // 6261461
            }
        } else {
            Object source = theEvent.getSource();
            // For Component source events, traverse the entire list,
            // trying to coalesce events
            if (source instanceof Component) {
                EventQueueItem q = queues[priority].head;
                for (;;) {
                    if (q.id == newItem.id && q.event.getSource() == source) {
                        AWTEvent coalescedEvent;
                        coalescedEvent = ((Component) source).coalesceEvents(q.event, theEvent);
                        if (coalescedEvent != null) {
                            q.event = coalescedEvent;
                            return;
                        }
                    }
                    if (q.next != null) {
                        q = q.next;
                    } else {
                        break;
                    }
                }
            }
            // The event was not coalesced or has non-Component source.
            // Insert it at the end of the appropriate Queue.
            if (theEvent.getID() == PaintEvent.PAINT ||
                theEvent.getID() == PaintEvent.UPDATE) {
                // This component doesn't have any events of this type on the
                // queue, so we have to initialize the RepaintArea with theEvent
                ((Component) source).coalesceEvents(theEvent, theEvent);
            }
            queues[priority].tail.next = newItem;
            queues[priority].tail = newItem;
            if (notifyID) {
                proxy.notifyAll(); // 6261461
            }
        }
    }

    /**
     * @return whether an event is pending on any of the separate Queues
     */
    private boolean noEvents() {
        for (int i = 0; i < NUM_PRIORITIES; i++) {
            if (queues[i].head != null) {
                return false;
            }
        }
        return true;
    }

    /**
     * Remove an event from the EventQueue and return it.  This method will
     * block until an event has been posted by another thread.
     * @return the next AWTEvent
     * @exception InterruptedException
     *            if another thread has interrupted this thread.
     */
    public AWTEvent getNextEvent() throws InterruptedException {
        return this.proxy.getNextEvent(); // 6261461
    }

    /**
     * Return the first event on the EventQueue without removing it.
     * @return the first event
     */
    public AWTEvent peekEvent() {
        synchronized (this.proxy) { // 6370528
        for (int i = NUM_PRIORITIES - 1; i >= 0; i--) {
            if (queues[i].head != null) {
                return queues[i].head.event;
            }
        }
        return null;
        }
    }

    /**
     * Return the first event with the specified id, if any.
     * @param id the id of the type of event desired.
     * @return the first event of the specified id
     */
    public AWTEvent peekEvent(int id) {
        synchronized (this.proxy) { // 6370528
        for (int i = NUM_PRIORITIES - 1; i >= 0; i--) {
            EventQueueItem q = queues[i].head;
            for (; q != null; q = q.next) {
                if (q.id == id) {
                    return q.event;
                }
            }
        }
        return null;
        }
    }

    /**
     * Dispatch an event. The manner in which the event is
     * dispatched depends upon the type of the event and the
     * type of the event's source
     * object:
     * <p> </p>
     * <table border>
     * <tr>
     *     <th>Event Type</th>
     *     <th>Source Type</th>
     *     <th>Dispatched To</th>
     * </tr>
     * <tr>
     *     <td>ActiveEvent</td>
     *     <td>Any</td>
     *     <td>event.dispatch()</td>
     * </tr>
     * <tr>
     *     <td>Other</td>
     *     <td>Component</td>
     *     <td>source.dispatchEvent(AWTEvent)</td>
     * </tr>
     * <tr>
     *     <td>Other</td>
     *     <td>MenuComponent</td>
     *     <td>source.dispatchEvent(AWTEvent)</td>
     * </tr>
     * <tr>
     *     <td>Other</td>
     *     <td>Other</td>
     *     <td>No action (ignored)</td>
     * </tr>
     * </table>
     * <p> </p>
     * @param theEvent an instance of java.awt.AWTEvent, or a
     * subclass of it.
     */
    protected void dispatchEvent(AWTEvent event) {
        Object src = event.getSource();
        if (event instanceof ActiveEvent) {
            // This could become the sole method of dispatching in time.
            ((ActiveEvent) event).dispatch();
        } else if (src instanceof Component) {
            ((Component) src).dispatchEvent(event);
        } else if (src instanceof MenuComponent) {
         ((MenuComponent)src).dispatchEvent(event);
         } else {
            System.err.println("unable to dispatch event: " + event);
        }
    }

    /**
     * Replace the existing EventQueue with the specified one.
     * Any pending events are transferred to the new EventQueue
     * for processing by it.
     *
     * @param an EventQueue (or subclass thereof) instance to be used.
     * @see      java.awt.EventQueue#pop
     */
    public void push(EventQueue newEventQueue) {
        synchronized (this.proxy) { // 6370528
        if (debug) {
            System.out.println("EventQueue.push(" + newEventQueue + ")");
        }
        if (nextQueue != null) {
            nextQueue.push(newEventQueue);
            return;
        }
        synchronized (newEventQueue.proxy) { // 6370528
            // Transfer all events forward to new EventQueue.
            while (peekEvent() != null) {
                try {
                    newEventQueue.postEventPrivate(getNextEvent());
                } catch (InterruptedException ie) {
                    if (debug) {
                        System.err.println("interrupted push:");
                        ie.printStackTrace(System.err);
                    }
                }
            }
            newEventQueue.previousQueue = this;
        }
        nextQueue = newEventQueue;

        AppContext appContext = AppContext.getAppContext();
        if (appContext.get(AppContext.EVENT_QUEUE_KEY) == this) {
            appContext.put(AppContext.EVENT_QUEUE_KEY, newEventQueue);
        }
        }

    }

    /**
     * Stop dispatching events using this EventQueue instance.
     * Any pending events are transferred to the previous
     * EventQueue for processing by it.
     *
     * @exception if no previous push was made on this EventQueue.
     * @see      java.awt.EventQueue#push
     */
    protected void pop() throws EmptyStackException {
        if (debug) {
            System.out.println("EventQueue.pop(" + this + ")");
        }
        // To prevent deadlock, we lock on the previous EventQueue before
        // this one.  This uses the same locking order as everything else
        // in EventQueue.java, so deadlock isn't possible.
        EventQueue prev = previousQueue;
        synchronized ((prev != null) ? prev.proxy : this.proxy) { // 6370528
            synchronized (this.proxy) { // 6370528
                if (nextQueue != null) {
                    nextQueue.pop();
                    return;
                }
                if (previousQueue == null) {
                    throw new EmptyStackException();
                }
                // Transfer all events back to previous EventQueue.
                previousQueue.nextQueue = null;
                while (peekEvent() != null) {
                    try {
                        previousQueue.postEventPrivate(getNextEvent());
                    } catch (InterruptedException ie) {
                        if (debug) {
                            System.err.println("interrupted pop:");
                            ie.printStackTrace(System.err);
                        }
                    }
                }

                AppContext appContext = AppContext.getAppContext();
                if (appContext.get(AppContext.EVENT_QUEUE_KEY) == this) {
                   appContext.put(AppContext.EVENT_QUEUE_KEY, previousQueue);
                }
                previousQueue = null;
            }
        }
        dispatchThread.stopDispatching(); // Must be done outside synchronized
        // block to avoid possible deadlock
    }

    /**
     * Returns true if the calling thread is the current AWT EventQueue's
     * dispatch thread.  Use this call the ensure that a given
     * task is being executed (or not being) on the current AWT
     * EventDispatchThread.
     *
     * @return true if running on the current AWT EventQueue's dispatch thread.
     */
    public static boolean isDispatchThread() {
        EventQueue eq = Toolkit.getEventQueue();
        EventQueue next = eq.nextQueue;
        while (next != null) {
            eq = next;
            next = eq.nextQueue;
        }
        return (Thread.currentThread() == eq.dispatchThread);
    }

    /*
     * Get the EventDispatchThread for this EventQueue.
     */
    final EventDispatchThread getDispatchThread() {
        return dispatchThread;
    }

    /*
     * Change the target of any pending KeyEvents because of a focus change.
     */
    final void changeKeyEventFocus(Object newSource) {
        synchronized (this.proxy) { // 6370528
        for (int i = 0; i < NUM_PRIORITIES; i++) {
            EventQueueItem q = queues[i].head;
            for (; q != null; q = q.next) {
                if (q.event instanceof KeyEvent) {
                    q.event.setSource(newSource);
                }
            }
        }
        }
    }

    /*
     * Remove any pending events for the specified source object.
     * This method is normally called by the source's removeNotify method.
     */
    final void removeSourceEvents(Object source) {
        synchronized (this.proxy) { // 6370528
            for (int i = 0; i < NUM_PRIORITIES; i++) {
                EventQueueItem entry = queues[i].head;
                EventQueueItem prev = null;
                while (entry != null) {
                    if (entry.event.getSource() == source) {
                        if (entry.event instanceof SequencedEvent) {
                            ((SequencedEvent)entry.event).dispose();
                        }
                        if (entry.event instanceof SentEvent) {
                            ((SentEvent)entry.event).dispose();
                        }
                        if (prev == null) {
                            queues[i].head = entry.next;
                        } else {
                            prev.next = entry.next;
                        }
                    } else {
                        prev = entry;
                    }
                    entry = entry.next;
                }
                queues[i].tail = prev;
            }
        }
    }
	
    /*
     * Remove any pending events for the specified source object.
     * This method is normally called by the source's removeNotify method.
     */
    final void removeEvents(Class type, int id) {
        synchronized (this.proxy) { // 6370528
            for (int i = 0; i < NUM_PRIORITIES; i++) {
                EventQueueItem entry = queues[i].head;
                EventQueueItem prev = null;
                while (entry != null) {
                    if (entry.event.getClass().equals(type) && entry.event.id == id) {
                        if (prev == null) {
                            queues[i].head = entry.next;
                        } else {
                            prev.next = entry.next;
                        }
                    } else {
                        prev = entry;
                    }
                    entry = entry.next;
                }
                queues[i].tail = prev;
            }
        }
    }

    /**
     * Causes <i>runnable</i> to have its run() method called in the dispatch
     * thread of the EventQueue.  This will happen after all pending events
     * are processed.
     *
     * @param runnable  the Runnable whose run() method should be executed
     *                  synchronously on the EventQueue
     * @see             #invokeAndWait
     * @since           1.2
     */
    public static void invokeLater(Runnable runnable) {
        Toolkit.getEventQueue().postEvent(
            new InvocationEvent(Toolkit.getDefaultToolkit(), runnable));
    }

    /**
     * Causes <i>runnable</i> to have its run() method called in the dispatch
     * thread of the EventQueue.  This will happen after all pending events
     * are processed.  The call blocks until this has happened.  This method
     * will throw an Error if called from the event dispatcher thread.
     *
     * @param runnable  the Runnable whose run() method should be executed
     *                  synchronously on the EventQueue
     * @exception       InterruptedException  if another thread has
     *                  interrupted this thread
     * @exception       InvocationTargetException  if an exception is thrown
     *                  when running <i>runnable</i>
     * @see             #invokeLater
     * @since           1.2
     */
    public static void invokeAndWait(Runnable runnable)
        throws InterruptedException, InvocationTargetException {
        if (EventQueue.isDispatchThread()) {
            throw new Error("Cannot call invokeAndWait from the event dispatcher thread");
        }
        class AWTInvocationLock {}
        Object lock = new AWTInvocationLock();
        EventQueue queue = Toolkit.getEventQueue();
        InvocationEvent event =
            new InvocationEvent(Toolkit.getDefaultToolkit(), runnable, lock,
                true);
        synchronized (lock) {
            Toolkit.getEventQueue().postEvent(event);
            lock.wait();
        }
        Exception eventException = event.getException();
        if (eventException != null) {
            throw new InvocationTargetException(eventException);
        }
    }

    static void setCurrentEventAndMostRecentTime(AWTEvent e) {
        Toolkit.getEventQueue().setCurrentEventAndMostRecentTimeImpl(e);
    }

    private void setCurrentEventAndMostRecentTimeImpl(AWTEvent e)
    {
        synchronized(this.proxy) { // 6370528
        if (Thread.currentThread() != dispatchThread) {
            return;
        }

        currentEvent = new WeakReference(e);

        // This series of 'instanceof' checks should be replaced with a
        // polymorphic type (for example, an interface which declares a
        // getWhen() method). However, this would require us to make such
        // a type public, or to place it in sun.awt. Both of these approaches
        // have been frowned upon.
  
        // In tiger, we will probably give timestamps to all events, so this
        // will no longer be an issue.
        if (e instanceof InputEvent) {
            InputEvent ie = (InputEvent)e;
            mostRecentEventTime = ie.getWhen();
        } else if (e instanceof ActionEvent) {
            ActionEvent ae = (ActionEvent)e;
            mostRecentEventTime = ae.getWhen();
        } else if (e instanceof InvocationEvent) {
            InvocationEvent ie = (InvocationEvent)e;
            mostRecentEventTime = ie.getWhen();
        }
        }
    }

    final void removeSourceEvents(Object source, boolean removeAllEvents) {
        synchronized (this.proxy) { // 6370528
            for (int i = 0; i < NUM_PRIORITIES; i++) {
                EventQueueItem entry = queues[i].head;
                EventQueueItem prev = null;
                while (entry != null) {
                    if ((entry.event.getSource() == source) 
                        && (removeAllEvents 
                            || ! (
                                  entry.event instanceof SentEvent ||
                                  entry.event instanceof SequencedEvent ||
                                  entry.event instanceof FocusEvent ||
                                  entry.event instanceof WindowEvent ||
                                  entry.event instanceof KeyEvent))) {
                        if (entry.event instanceof SequencedEvent) {
                            ((SequencedEvent)entry.event).dispose();
                        }
                        if (entry.event instanceof SentEvent) {
                            ((SentEvent)entry.event).dispose();
                        }
                        if (prev == null) {
                            queues[i].head = entry.next;
                        } else {
                            prev.next = entry.next;
                        }
                    } else {
                        prev = entry;
                    }
                    entry = entry.next;
                }
                queues[i].tail = prev;
            }
        }
    }

    public static long getMostRecentEventTime() {
        return Toolkit.getEventQueue().getMostRecentEventTimeImpl();
    }
    private long getMostRecentEventTimeImpl() {
        synchronized(this.proxy) { // 6370528
        return (Thread.currentThread() == dispatchThread)
            ? mostRecentEventTime
            : System.currentTimeMillis();
        }
    }
    /**
     * Returns the the event currently being dispatched by the
     * <code>EventQueue</code> associated with the calling thread. This is
     * useful if a method needs access to the event, but was not designed to
     * receive a reference to it as an argument. Note that this method should
     * only be invoked from an application's event dispatching thread. If this
     * method is invoked from another thread, null will be returned.
     *
     * @return the event currently being dispatched, or null if this method is
     *         invoked on a thread other than an event dispatching thread
     * @since 1.4
     */
    public static AWTEvent getCurrentEvent() {
        return Toolkit.getEventQueue().getCurrentEventImpl();
    }
    private AWTEvent getCurrentEventImpl() {
        synchronized(this.proxy) { // 6370528

        return (Thread.currentThread() == dispatchThread)
            ? ((AWTEvent)currentEvent.get())
            : null;
        }
    }

    AWTEvent getNextEvent(int id) throws InterruptedException {
        return this.proxy.getNextEvent(id); // 6261461
    }

    // 6261461
    // EffectiveJava Pattern : Finalizer Guardian Idiom.
    private final Object finalizerGuardian = new Object() {
        protected void finalize() throws Throwable {
            try {
                EventQueue.this.dispatchThread.stopDispatching();
            }
            finally {
                super.finalize();
            }
        }
    };
    // 6261461
}

/**
 * The Queue object holds pointers to the beginning and end of one internal
 * queue. An EventQueue object is composed of multiple internal Queues, one
 * for each priority supported by the EventQueue. All Events on a particular
 * internal Queue have identical priority.
 */
class Queue {
    EventQueueItem head;
    EventQueueItem tail;
}

class EventQueueItem {
    AWTEvent event;
    int      id;
    EventQueueItem next;
    EventQueueItem(AWTEvent evt) {
        event = evt;
        id = evt.getID();
    }
}
