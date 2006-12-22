/*
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
package com.sun.mmedia;

import com.sun.midp.events.*;
import com.sun.midp.security.*;


/**
 * MMEventHandler is used by BasicPlayer to register an event handler.  
 */
public class MMEventHandler {

    /**
     * Security token to be set by MIDletSuiteLoader.
     */
    private static SecurityToken classSecurityToken;

    /**
     * The EventListener to receive the MM events.
     */
    private static EventListener listener;

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public final static void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }

    /**
     * Set the Event listener on the event queue.  
     * <br>
     * This method is made package-private so only classes from
     * com.sun.mmedia (BasicPlayer) can access it for security
     * reasons.
     *
     * @param l EventListener for the MM events.
     */
    static void setListener(EventListener l) {

	// This can only be set once.
	if (listener != null)
	    return;
	listener = l;
	
	EventQueue evtq = EventQueue.getEventQueue(classSecurityToken);
	evtq.registerEventListener(EventTypes.MM_EOM_EVENT, listener);
	evtq.registerEventListener(EventTypes.MM_SAT_EVENT, listener);
	evtq.registerEventListener(EventTypes.MM_TONEEOM_EVENT, listener);
    }
}
