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

package com.sun.midp.automation;
import java.util.*;

/**
 * Implements AutoEventFromArgsFactory interface for delay events.
 */
class AutoDelayEventFromArgsFactory 
    implements AutoEventFromArgsFactory {

    /**
     * Gets prefix that this factory knows about.
     *
     * @return string prefix
     */
    public String getPrefix() {
        return AutoEventType.DELAY.getName();
    }

    /**
     * Creates event(s) from arguments. Multiple events can be created,
     * for example, in case of key click, which consists of two events:
     * key pressed and key released.
     *
     * @param args argument name/argument value pairs in form of Hashtable 
     * with argument names used as key
     * @return event(s) created from arguments in form of AutoEvent array
     * @throws IllegalArgumentException if factory was unable to create
     * event(s) from these arguments
     */
    public AutoEvent[] create(Hashtable args)
        throws IllegalArgumentException {

        AutoDelayEvent delayEvent = null;

        if (args == null) {
            throw new IllegalArgumentException("No arguments specified"); 
        }
        
        String delayS = (String)args.get(AutoDelayEventImpl.MSEC_ARG_NAME);
        if (delayS == null) {
            throw new IllegalArgumentException("No delay value specified");
        }

        int msec = 0;
        try {
            msec = Integer.parseInt(delayS);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                    "Invalid delay value: " + delayS);
        }
        
        delayEvent = new AutoDelayEventImpl(msec);
        AutoEvent[] events = new AutoEvent[1];
        events[0] = delayEvent;

        return events;
    }
}
