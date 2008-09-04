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

class AutoKeyEventFromStringFactory 
    implements AutoEventFromStringFactory {

    private AutoDelayEventFromStringFactory delayEventFactory;

    AutoKeyEventFromStringFactory() {
        delayEventFactory = new  AutoDelayEventFromStringFactory();
    }

    public String getPrefix() {
        return AutoEventType.KEY.getName();
    }

    public AutoEvent[] create(Hashtable args)
        throws IllegalArgumentException {

        AutoDelayEvent delayEvent = null;
        AutoKeyEvent keyEvent1 = null;
        AutoKeyEvent keyEvent2 = null;
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

        String codeS = (String)args.get(AutoKeyEventImpl.CODE_ARG_NAME);
        if (codeS == null) {
            throw new IllegalArgumentException("No key code specified");
        }

        char keyChar = ' ';
        AutoKeyCode keyCode = null;
        if (codeS.length() == 1) {
            keyChar = codeS.charAt(0);
        } else {
            keyCode = AutoKeyCode.getByName(codeS);
            if (keyCode == null) {
                throw new IllegalArgumentException(
                        "Invalid key code: " + codeS);
            }
        }

        String stateS = (String)args.get(AutoKeyEventImpl.STATE_ARG_NAME);
        if (stateS == null) {
            throw new IllegalArgumentException("No key state specified");
        }

        if (stateS.equals("clicked")) {
            if (keyCode != null) {
                keyEvent1 = new AutoKeyEventImpl(keyCode, 
                        AutoKeyState.PRESSED);
                keyEvent2 = new AutoKeyEventImpl(keyCode, 
                        AutoKeyState.RELEASED);
            } else {
                keyEvent1 = new AutoKeyEventImpl(keyChar, 
                        AutoKeyState.PRESSED);
                keyEvent2 = new AutoKeyEventImpl(keyChar, 
                        AutoKeyState.RELEASED);
            }

            totalEvents += 2;
        } else {
            AutoKeyState keyState = AutoKeyState.getByName(stateS);
            if (keyState == null) {
                throw new IllegalArgumentException(
                        "Invalid key state: " + stateS);
            }

            if (keyCode != null) {
                keyEvent1 = new AutoKeyEventImpl(keyCode, keyState);
            } else {
                keyEvent1 = new AutoKeyEventImpl(keyChar, keyState);
            }

            totalEvents += 1;
        }

        AutoEvent[] events = new AutoEvent[totalEvents];
        totalEvents = 0;

        if (delayEvent != null) {
            events[totalEvents++] = delayEvent;
        }

        events[totalEvents++] = keyEvent1;

        if (keyEvent2 != null) {
            events[totalEvents++] = keyEvent2; 
        }

        return events;
    }   
}

