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

final class AutoEventFactoryImpl extends AutoEventFactory {
    private static AutoEventFactoryImpl instance = null;
    private final static Object sync = new Object();
    

    static AutoEventFactoryImpl getInstanceImpl() {
        synchronized (sync) {
            AutomationInitializer.guaranteeAutomationInitialized();

            if (instance == null) {
                instance = new AutoEventFactoryImpl();
            }            
        }

        return instance;
    }


    public AutoEvent createFromString(String str)
        throws IllegalArgumentException {

        return createFromString(str, 0, null);
    }

    public AutoEvent createFromString(String str, int offset)
        throws IllegalArgumentException {

        return createFromString(str, offset, null);
    }

    public AutoEvent createFromString(String str, int offset, 
            Integer newOffset) 
        throws IllegalArgumentException {

        if (str == null) {
            throw new IllegalArgumentException("Event string is null");
        }

        AutoEvent event = null;

        int size = eventCreators.size();
        for (int i = 0; i < size; ++i) {
            Object o = eventCreators.elementAt(i);
            EventFromStringCreator c = (EventFromStringCreator)o;

            String prefix = c.getEventType().getName();
            if (str.startsWith(prefix, offset)) {
                event = c.createFromString(str, offset, newOffset);
                break;
            }
        }

        if (event == null) {
            throw new IllegalArgumentException("Illegal AutoEvent string");
        }

        return event;
    }

    public AutoKeyEvent createKeyEvent(AutoKeyState keyState, 
            AutoKeyCode keyCode) 
        throws IllegalArgumentException {

        return new AutoKeyEventImpl(keyState, keyCode);
    }

    public AutoKeyEvent createKeyEvent(AutoKeyState keyState, 
            char keyChar) throws IllegalArgumentException {

        return new AutoKeyEventImpl(keyState, keyChar);
    }

    public AutoPenEvent createPenEvent(AutoPenState penState, int x, int y) {
        return null;
    }
    
    
    interface EventFromStringCreator {
        AutoEventType getEventType();

        AutoEvent createFromString(String str, int offset, 
            Integer newOffset) throws IllegalArgumentException;
    }

    private Vector eventCreators = null; 

    void registerEventFromStringCreator(EventFromStringCreator creator) 
        throws IllegalArgumentException {
    
        if (creator == null) {
            throw new IllegalArgumentException(
                    "EventFromStringCreator is null");
        }

        if (eventCreators == null) {
            eventCreators = new Vector();
        }

        eventCreators.addElement(creator);
    }

    /**
     * Private constructor to prevent user from creating an instance.
     */
    private AutoEventFactoryImpl() {
    }    
}
