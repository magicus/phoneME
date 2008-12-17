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
 * Implements AutoEventFromArgsFactory interface for pen events.
 */
class AutoPenEventFromArgsFactory 
    implements AutoEventFromArgsFactory {

    /** 
     * Delay event factory for handling 
     * AutoDelayEventImpl.MSEC_ARG_NAME argument 
     */
    private AutoDelayEventFromArgsFactory delayEventFactory;

    /** Special state: "clicked" */
    private static final String CLICKED_STATE_NAME = "clicked";


    /**
     * Gets prefix that this factory knows about.
     *
     * @return string prefix
     */
    public String getPrefix() {
        return AutoEventType.PEN.getName();
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
        AutoPenEvent penEvent1 = null;
        AutoPenEvent penEvent2 = null;
        int totalEvents = 0;

        if (args == null) {
            throw new IllegalArgumentException("No arguments specified");
        }        

        String delayS = (String)args.get(AutoDelayEventImpl.MSEC_ARG_NAME);
        if (delayS != null) {
            AutoEvent[] events = delayEventFactory.create(args);
            delayEvent = (AutoDelayEvent)events[0];
            totalEvents++;
        }

        int x = 0;
        String xS = (String)args.get(AutoPenEventImpl.X_ARG_NAME);
        if (xS == null) {
            throw new IllegalArgumentException("No x coord specified");
        }

        try {
            x = Integer.parseInt(xS);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                    "Invalid x coord value: " + xS);
        }

        int y = 0;
        String yS = (String)args.get(AutoPenEventImpl.Y_ARG_NAME);
        if (yS == null) {
            throw new IllegalArgumentException("No y coord specified");
        }

        try {
            y = Integer.parseInt(yS);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                    "Invalid y coord value: " + yS);
        }

        String stateS = (String)args.get(AutoPenEventImpl.STATE_ARG_NAME);
        if (stateS == null) {
            throw new IllegalArgumentException("No state code specified");
        }

        if (stateS.equals(CLICKED_STATE_NAME)) {
            penEvent1 = new AutoPenEventImpl(x, y, AutoPenState.PRESSED);
            penEvent2 = new AutoPenEventImpl(x, y, AutoPenState.RELEASED);

            totalEvents += 2;
        } else {
            AutoPenState penState = AutoPenState.getByName(stateS);
            if (penState == null) {
                throw new IllegalArgumentException(
                        "Invalid pen state: " + stateS);
            }

            penEvent1 = new AutoPenEventImpl(x, y, penState);

            totalEvents += 1;
        }

        AutoEvent[] events = new AutoEvent[totalEvents];
        totalEvents = 0;

        if (delayEvent != null) {
            events[totalEvents++] = delayEvent;
        }

        events[totalEvents++] = penEvent1;

        if (penEvent2 != null) {
            events[totalEvents++] = penEvent2;
        }

        return events;
    }

    /**
     * Constructor.
     */
    AutoPenEventFromArgsFactory() {
        delayEventFactory = new  AutoDelayEventFromArgsFactory();
    }
}
