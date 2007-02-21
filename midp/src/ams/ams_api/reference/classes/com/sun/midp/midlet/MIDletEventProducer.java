/*
 *   
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

package com.sun.midp.midlet;

import com.sun.midp.events.EventTypes;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.events.EventQueue;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;

/**
 * This class provides methods to send events of types 
 * handled by MIDletEventConsumer I/F implementors. 
 * This class completely hide event construction & sending in its methods.
 *
 * The primary user of this class are MIDletProxyList & MIDletProxy 
 * in AMS isolate. Threrefore ve can use MIdletProxy object reference 
 * as sendXXXEvent() method parameter instead of currently used int IDs.
 * 
 * Generic comments for all XXXEventProducers:
 *
 * For each supported event type there is a separate sendXXXEvent() method, 
 * that gets all needed parameters to construct an event of an approprate class.
 * The method also performs event sending itself.
 *
 * If a given event type merges a set of logically different subtypes, 
 * this class shall provide separate methods for these subtypes.
 *
 * It is assumed that only one object instance of this class (per isolate)
 * is created at (isolate) startup. 
 * All MIDP stack subsystems that need to send events of supported types, 
 * must get a reference to an already created istance of this class. 
 * Typically, this instance should be passed as a constructor parameter.
 *
 * For security reasons constructor is not public. 
 * Use createXXXProducer(...) method, 
 * protected by security, to create and object instance of this class
 * from a different package.
 *
 * Class is NOT final to allow debug/profile/test/automation subsystems
 * to change, substitute, complement default "event sending" functionality :
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
 */
public class MIDletEventProducer {
    
    /** Cached reference to the MIDP event queue. */
    protected EventQueue eventQueue;
    
    /**
     * Construct a new MIDletEventProducer.
     *
     * @param  token security token that controls instance creation.
     * @param  theEventQueue An event queue where new events will be posted.
     */
    public MIDletEventProducer(SecurityToken token, EventQueue theEventQueue) {
        
        /**
         * token must have AMS permissions 
         *(primary user of this object is AMS's MIDletProxyList & MIDletProxy) 
         */
        token.checkIfPermissionAllowed(Permissions.AMS);
        
        eventQueue = theEventQueue;
    }

    /*
     * MIDlet State Management (Lifecycle) Events
     *
     * ACTIVATE_MIDLET_EVENT
     * PAUSE_MIDLET_EVENT
     * DESTROY_MIDLET_EVENT
     *
     */

    /**
     * Pause a MIDlet.
     * Called by the system event handler within Display.
     * Former: void MIDletProxy.activateMidlet();
     * Probably: need some form of MIDlet ID instead of displayId.
     *           use MIDletProxy instead of 2 int ID parameters.
     *
     * @param isolateId ID of the target isolate (where to send event)
     * @param displayId ID of the target display
     */
    public void sendMIDletActivateEvent(int isolateId, int displayId) {
        sendMIDletEvent(EventTypes.ACTIVATE_MIDLET_EVENT,
                isolateId, displayId);
    }
    /**
     * Activate a MIDlet.
     * Called by the system event handler within Display.
     * Former: void MIDletProxy.pauseMidlet();
     * Probably: need some form of MIDlet ID instead of displayId.
     *           use MIDletProxy instead of 2 int ID parameters.
     *
     * @param isolateId ID of the target isolate (where to send event)
     * @param displayId ID of the target display
     */
    public void sendMIDletPauseEvent(int isolateId, int displayId) {
        sendMIDletEvent(EventTypes.PAUSE_MIDLET_EVENT, isolateId, displayId);
    }

    /**
     * Destroy a MIDlet.
     * Called by the system event handler within Display.
     * Former: void MIDletProxy.destroyMidlet();
     * Probably: need some form of MIDlet ID instead of displayId.
     *           use MIDletProxy instead of 2 int ID parameters.
     *
     * @param isolateId ID of the target isolate (where to send event)
     * @param displayId ID of the target display
     */
    public void sendMIDletDestroyEvent(int isolateId, int displayId) {
        sendMIDletEvent(EventTypes.DESTROY_MIDLET_EVENT, isolateId, displayId);
    }

    /*
     * Foreground Display Management Events:
     *
     * FOREGROUND_NOTIFY_EVENT
     *
     * Probably: move to DisplayEventProducer or a separate Producer ?
     *
     */    
    /**
     * Called to process a change a display's foreground/background status.
     * Former: void MIDletProxy.notifyMIDletHasForeground(
     *              boolean hasForeground = true)
     * Probably: need some form of MIDlet ID instead of displayId.
     *           use MIDletProxy instead of 2 int ID parameters.
     *
     * @param midletIsolateId ID of the target isolate (where to send event)
     * @param midletDisplayId ID of the target display
     */
    public void sendDisplayForegroundNotifyEvent(
        int midletIsolateId, 
        int midletDisplayId) {
        NativeEvent event =
            new NativeEvent(EventTypes.FOREGROUND_NOTIFY_EVENT);

        event.intParam1 = 1; /* hasForeground = true */
        event.intParam4 = midletDisplayId;
        
        eventQueue.sendNativeEventToIsolate(event, midletIsolateId);
    }

    /**
     * Called to process a change a display's foreground/background status.
     * Former: void MIDletProxy.notifyMIDletHasForeground(
     *              boolean hasForeground = false)
     * Probably: need some form of MIDlet ID instead of displayId.
     *           use MIDletProxy instead of 2 int ID parameters.
     *
     * @param midletIsolateId ID of the target isolate (where to send event)
     * @param midletDisplayId ID of the target display
     */
    public void sendDisplayBackgroundNotifyEvent(
        int midletIsolateId, 
        int midletDisplayId) {
        NativeEvent event =
            new NativeEvent(EventTypes.FOREGROUND_NOTIFY_EVENT);

        event.intParam1 = 0; /* hasForeground = true */
        event.intParam4 = midletDisplayId;
        
        eventQueue.sendNativeEventToIsolate(event, midletIsolateId);
    }

    /**
     * Sends an event with the only integer parameter for display ID
     * to a MIDlet .
     *
     * @param eventType type of event to be sent
     * @param midletIsolateId ID of the target isolate
     * @param midletDisplayId ID of the target display
     */
    private void sendMIDletEvent(int eventType, int midletIsolateId,
                                 int midletDisplayId) {
        NativeEvent event = new NativeEvent(eventType);

        event.intParam4 = midletDisplayId;

        eventQueue.sendNativeEventToIsolate(event, midletIsolateId);
    }
}
