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

final class AutoEventFactoryImpl implements AutoEventFactory {
    private static AutoEventFactoryImpl instance = null;

    private Hashtable eventFromStringFactories;
    private AutoEventStringParser eventStringParser;

    synchronized static AutoEventFactoryImpl getInstance() {
        if (instance == null) {
            instance = new AutoEventFactoryImpl();
        }            

        return instance;
    }


    public AutoEventSequence createFromString(String str)
        throws IllegalArgumentException {

        return createFromString(str, 0, null);
    }

    public AutoEventSequence createFromString(String str, int offset)
        throws IllegalArgumentException {

        return createFromString(str, offset, null);
    }

    public AutoEventSequence createFromString(String eventString, int offset,
            Integer newOffset) 
        throws IllegalArgumentException {

        int curOffset = offset;
        AutoEventSequence seq = new AutoEventSequenceImpl();
        AutoEvent[] events = null;

        do {
            eventStringParser.parse(eventString, offset);
            String eventPrefix = eventStringParser.getEventPrefix();
            if (eventPrefix == null) {
                break;
            }

            Object o = eventFromStringFactories.get(eventPrefix);
            AutoEventFromStringFactory f = (AutoEventFromStringFactory)o;
            if (f == null) {
                throws IllegalArgumentException(
                        "Illegal event prefix: " + eventPrefix);
            }

            Hashtable eventArgs = eventStringParser.getEventArgs();
            events = f.create(eventArgs);
            curOffset = eventStringParser.getOffset();
            
            if (events != null) {
                seq.addEvents(events);
            }

        } while (events != null);


        return seq;
    }

    public AutoKeyEvent createKeyEvent(AutoKeyCode keyCode, 
            AutoKeyState keyState) 
        throws IllegalArgumentException {

        return new AutoKeyEventImpl(keyCode, keyState);
    }

    public AutoKeyEvent createKeyEvent(char keyChar, AutoKeyState keyState) 
        throws IllegalArgumentException {

        return new AutoKeyEventImpl(keyChar, keyState);
    }

    public AutoPenEvent createPenEvent(int x, int y, AutoPenState penState) 
        throws IllegalArgumentException {

        return new AutoPenEventImpl(x, y, penState);
    }

    public AutoDelayEvent createDelayEvent(int msec) 
        throws IllegalArgumentException {

        return new AutoDelayEventImpl(msec);
    }

    
    void registerEventFromStringFactory(AutoEventFromStringFactory factory) 
        throws IllegalArgumentException {
    
        if (factory == null) {
            throw new IllegalArgumentException(
                    "AutoEventFromStringFactory is null");
        }

        eventFromStringFactories.put(factory,getPrefix(), factory);
    }


    /**
     * Private constructor to prevent user from creating an instance.
     */
    private AutoEventFactoryImpl() {
        eventFromStringFactories = new Hashtable();
        eventStringParser = new AutoEventStringParser();
    }
}
