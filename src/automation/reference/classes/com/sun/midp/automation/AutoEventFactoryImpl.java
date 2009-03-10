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
import com.sun.midp.events.*;
import com.sun.midp.lcdui.EventConstants;

final class AutoEventFactoryImpl implements AutoEventFactory {
    /** The one and only instance */
    private static AutoEventFactoryImpl instance = null;

    /** All EventFromArgsFactory instances indexed by prefix */
    private Hashtable eventFromArgsFactories;

    /** Event string parser */
    private AutoEventStringParser eventStringParser;

    /**
     * Gets instance of AutoEventFactoryImpl class.
     *
     * @return instance of AutoEventFactoryImpl class
     */
    static final synchronized AutoEventFactoryImpl getInstance() {
        if (instance == null) {
            instance = new AutoEventFactoryImpl();
        }            

        return instance;
    }

    /**
     * Creates event sequence from string representation.
     *
     * @param sequenceString string representation of event sequence
     * @param offset offset into sequenceString
     * @return AutoEventSequence corresponding to the specified string 
     * representation
     * @throws IllegalArgumentException if specified string isn't valid 
     * string representation of events sequence
     */
    public AutoEventSequence createFromString(String eventString, int offset) 
        throws IllegalArgumentException {

        int curOffset = offset;
        AutoEventSequence seq = new AutoEventSequenceImpl();
        AutoEvent[] events = null;

        eventStringParser.parse(eventString, curOffset);
        String eventPrefix = eventStringParser.getEventPrefix();
        while (eventPrefix != null) {
            Object o = eventFromArgsFactories.get(eventPrefix);
            AutoEventFromArgsFactory f = (AutoEventFromArgsFactory)o;
            if (f == null) {
                throw new IllegalArgumentException(
                        "Illegal event prefix: " + eventPrefix);
            }

            Hashtable eventArgs = eventStringParser.getEventArgs();
            events = f.create(eventArgs);
            seq.addEvents(events);            

            curOffset = eventStringParser.getEndOffset(); 
            eventStringParser.parse(eventString, curOffset);
            eventPrefix = eventStringParser.getEventPrefix();
        }
        
        return seq;
    }

    /**
     * Creates event sequence from string representation.
     *
     * @param sequenceString string representation of event sequence
     * @return AutoEventSequence corresponding to the specified string 
     * representation
     * @throws IllegalArgumentException if specified string isn't valid 
     * string representation of events sequence
     */
    public AutoEventSequence createFromString(String eventString)
        throws IllegalArgumentException {

        return createFromString(eventString, 0);
    }

    /**
     * Creates key event.
     *
     * @param keyCode key code not representable as character 
     * (soft key, for example) 
     * @param keyState key state
     * @return AutoKeyEvent instance representing key event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoKeyEvent createKeyEvent(AutoKeyCode keyCode, 
            AutoKeyState keyState) 
        throws IllegalArgumentException {

        return new AutoKeyEventImpl(keyCode, keyState);
    }

    /**
     * Creates key event.
     *
     * @param keyChar key char (letter, digit)
     * @param keyState key state
     * @return AutoKeyEvent representing key event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoKeyEvent createKeyEvent(char keyChar, AutoKeyState keyState) 
        throws IllegalArgumentException {

        return new AutoKeyEventImpl(keyChar, keyState);
    }

    /**
     * Creates pen event.
     *
     * @param x x coord of pen tip
     * @param y y coord of pen tip
     * @param penState pen state
     * @return AutoPenEvent representing pen event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoPenEvent createPenEvent(int x, int y, AutoPenState penState) 
        throws IllegalArgumentException {

        return new AutoPenEventImpl(x, y, penState);
    }

    /**
     * Creates delay event.
     *
     * @param msec delay value in milliseconds
     * @return AutoDelayEvent representing delay event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoDelayEvent createDelayEvent(int msec) 
        throws IllegalArgumentException {

        return new AutoDelayEventImpl(msec);
    }

    /**
     * Creates event from native (used by our MIDP implementation) event.
     *
     * @param nativeEvent native event
     * @return created event
     * @throws IllegalArgumentException if this kind of native event
     * is not supported
     */
    public AutoEvent createFromNativeEvent(NativeEvent nativeEvent) {
        AutoEvent event = null;

        switch (nativeEvent.getType()) {
            case EventTypes.KEY_EVENT: {
                AutoKeyState keyState = AutoKeyState.getByMIDPKeyState(
                        nativeEvent.intParam1);
                AutoKeyCode keyCode =  AutoKeyCode.getByMIDPKeyCode(
                        nativeEvent.intParam2);

                if (keyCode != null) {
                    event = createKeyEvent(keyCode, keyState);
                } else {
                    char keyChar = (char)nativeEvent.intParam2;
                    event = createKeyEvent(keyChar, keyState);
                }

                break;
            }

            case EventTypes.PEN_EVENT: {
                AutoPenState penState = AutoPenState.getByMIDPPenState(
                        nativeEvent.intParam1);
                int x = nativeEvent.intParam2;
                int y = nativeEvent.intParam3;

                event = createPenEvent(x, y, penState);

                break;
            }

            default: {
                throw new IllegalArgumentException(
                     "Unexpected NativeEvent type: " + nativeEvent.getType());
            }
        }

        return event;
    }

    /**
     * Registers all AutoEventFromArgsFactory factories.
     */ 
    private void registerEventFromArgsFactories() {
        AutoEventFromArgsFactory f;
    
        f = new AutoKeyEventFromArgsFactory();
        eventFromArgsFactories.put(f.getPrefix(), f);

        f = new AutoPenEventFromArgsFactory();
        eventFromArgsFactories.put(f.getPrefix(), f);

        f = new AutoDelayEventFromArgsFactory();
        eventFromArgsFactories.put(f.getPrefix(), f);
    }


    /**
     * Private constructor to prevent creating class instances.
     */
    private AutoEventFactoryImpl() {
        eventFromArgsFactories = new Hashtable();
        eventStringParser = new AutoEventStringParser();

        registerEventFromArgsFactories();
    }
}
