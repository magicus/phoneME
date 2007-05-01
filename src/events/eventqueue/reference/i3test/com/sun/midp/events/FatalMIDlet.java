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

package com.sun.midp.events;

import javax.microedition.midlet.MIDlet;

import com.sun.midp.main.MIDletSuiteUtils;

/**
 * Register an event listener, send an event to self, and throw a
 * RuntimeException while processing an event.
 */
public class FatalMIDlet extends MIDlet implements EventListener {
    /**
     * Set up.
     */
    public void startApp() {
        EventQueue eventQueue;
        Event event;

        if (MIDletSuiteUtils.isAmsIsolate()) {
            // This is single VM mode don't throw a fatal error. Just end.
            notifyDestroyed();
            return;
        } 

        eventQueue = EventQueue.getEventQueue();

        eventQueue.registerEventListener(EventTypes.TEST_EVENT, this);

        event = new Event(EventTypes.TEST_EVENT);

        eventQueue.post(event);
    }

    /**
     * Pause; there are no resources that need to be released.
     */
    public void pauseApp() {
    }

    /**
     * Does nothing.
     *
     * @param unconditional is ignored
     */
    public void destroyApp(boolean unconditional) {
    }

    /**
     * Preprocess an event that is being posted to the event queue.
     * This method will get called in the thread that posted the event.
     * 
     * @param event event being posted
     *
     * @param waitingEvent previous event of this type waiting in the
     *     queue to be processed
     * 
     * @return true to allow the post to continue, false to not post the
     *     event to the queue
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        throw new RuntimeException("fatal test error");
    }
}
