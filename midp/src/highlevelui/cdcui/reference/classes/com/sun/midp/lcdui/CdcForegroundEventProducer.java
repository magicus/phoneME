/*
 *   
 *
 * Copyright 2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui;

import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.NativeEvent;

/**
 * This class provides methods to send events of types 
 * handled by ForegroundEventConsumer I/F implementors. 
 * This class completely hide event construction & sending in its methods.
 * <p>
 * Generic comments for all XXXEventProducers:
 * <p>
 * For each supported event type there is a separate sendXXXEvent() method, 
 * that gets all needed parameters to construct an event of an approprate
 * class.
 * The method also performs event sending itself.
 * <p>
 * If a given event type merges a set of logically different subtypes, 
 * this class shall provide separate methods for these subtypes.
 * <p>
 * It is assumed that only one object instance of this class
 * is initialized with the system event that is created at (isolate) startup. 
 * <p>
 * This class only operates on the event queue given to it during
 * construction, the class does not obtain any restricted object itself,
 * so it does not need protection.
 * <p>
 * All MIDP stack subsystems that need to send events of supported types, 
 * must get a reference to an already created istance of this class. 
 * Typically, this instance should be passed as a constructor parameter.
 * <p>
 * Class is NOT final to allow debug/profile/test/automation subsystems
 * to change, substitute, complement default "event sending" functionality :
 * <code> 
 * Ex.
 * class LogXXXEventProducer 
 *      extends XXXEventProducer {
 *  ...
 *  void sendXXXEvent(parameters) {
 *      LOG("Event of type XXX is about to be sent ...")
 *      super.sendXXXEvent(parameters);
 *      LOG("Event of type XXX has been sent successfully !")
 *  }
 *  ...
 * }
 * </code>
 */
public class CdcForegroundEventProducer {
    
    /** Cached reference to the MIDP event queue. */
    private EventQueue eventQueue;
    
    /**
     * Construct a new ForegroundEventProducer.
     *
     * @param  theEventQueue An event queue where new events will be posted.
     */
    public CdcForegroundEventProducer(EventQueue theEventQueue) {
        eventQueue = theEventQueue;
    }

    /**
     * Called to process a change a display's foreground/background status.
     *
     * @param midletDisplayId ID of the target display
     */
    public void sendDisplayForegroundNotifyEvent(
        int midletDisplayId) {

        NativeEvent event =
            new NativeEvent(EventTypes.FOREGROUND_NOTIFY_EVENT);

        event.intParam4 = midletDisplayId;
        
        eventQueue.post(event);
    }

    /**
     * Called to process a change a display's foreground/background status.
     *
     * @param midletDisplayId ID of the target display
     */
    public void sendDisplayBackgroundNotifyEvent(
        int midletDisplayId) {
        NativeEvent event =
            new NativeEvent(EventTypes.BACKGROUND_NOTIFY_EVENT);

        event.intParam4 = midletDisplayId;
        
        eventQueue.post(event);
    }
}
