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

import com.sun.midp.main.MIDletSuiteUtils;
import com.sun.midp.security.*;
import com.sun.j2me.security.AccessController;

/**
 * A special queue that recieves events from all other queues,
 * thus allowing to spy on events flow.
 */
public class EventSpyingQueue extends EventQueue {
    /** The event spying queue. */
    private static EventSpyingQueue eventSpyingQueue = null;

    /**
     * Get a reference to the event spying queue in a secure way.
     *
     * @param token security token with the com.sun.midp permission "allowed"
     *
     * @return MIDP event queue reference
     */
    public static EventQueue getEventQueue(SecurityToken token) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        
        if (eventSpyingQueue == null) {
            eventSpyingQueue = newInstance();
        }

        return eventSpyingQueue;
    }

    /**
     * Get a reference to the event queue in a secure way.
     * The calling suite must have the com.sun.midp permission "allowed".
     *
     * @return MIDP event queue reference
     */
    public static EventQueue getEventQueue() {
        AccessController.checkPermission(Permissions.MIDP_PERMISSION_NAME);

        if (eventSpyingQueue == null) {
            eventSpyingQueue = newInstance();
        }

        return eventSpyingQueue;
    }
    
    EventSpyingQueue() {
        super();
    }

    /**
     * Get the handle of the native peer event queue.
     *
     * @return Native event queue handle
     */
    protected native int getNativeEventQueueHandle();

    /**
     * Creates event queue instance.
     */
    private static EventSpyingQueue newInstance() {
        EventSpyingQueue instance = null;

        if (MIDletSuiteUtils.isAmsIsolate()) {
            instance = new EventSpyingQueue(); 
            instance.start();
        }

        return instance;
    }

}
