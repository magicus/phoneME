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

import com.sun.midp.i3test.*;
import java.util.*;

/**
 * i3test for keyboard events simulation
 */
public class TestEventFromStringCreation extends TestCase {
    private static final String eventString1 = 
        "key code: a, state: pressed\n" +
        "key code: a, state: released\n" +
        "key code: A, state: pressed\n" +
        "key code: A, state: released\n" +
        "key code: soft1, state: pressed\n" +
        "key code: soft1, state: released\n" +
        "pen x: 1, y: 1, state: pressed\n" +
        "pen x: -1, y: -1, state: released\n" +
        "pen x: -1, y: 1, state: dragged\n" + 
        "delay msec: 500";

    private static final String seqString1Expected = eventString1;    

    private static final String eventString2 = 
        "key    code  : a , state :clicked\n" + 
        "key code :   soft1  ,state : pressed   , msec : 500\n" +
        "key code: A, state: clicked, msec: 200\n" + 
        "pen x: -1, y: 1, state: clicked\n" +
        "pen x: 1, y: 1, state: pressed, msec: 500\n" +
        "pen x: -1, y: -1, state: clicked, msec: 200";

    private static final String seqString2Expected = 
        "key code: a, state: pressed\n" + 
        "key code: a, state: released\n" +
        "delay msec: 500\n" +
        "key code: soft1, state: pressed\n" + 
        "delay msec: 200\n" +
        "key code: A, state: pressed\n" +
        "key code: A, state: released\n" + 
        "pen x: -1, y: 1, state: pressed\n" +
        "pen x: -1, y: 1, state: released\n" +
        "delay msec: 500\n" +
        "pen x: 1, y: 1, state: pressed\n" +
        "delay msec: 200\n" +
        "pen x: -1, y: -1, state: pressed\n" +
        "pen x: -1, y: -1, state: released";


    void testEventFromStringCreation() {
        Automation a = Automation.getInstance();
        declare("Get AutoEventFactory instance");
        AutoEventFactory eventFactory = a.getEventFactory();
        assertNotNull("Failed to get AutoEventFactory instance", eventFactory);

        AutoEventSequence seq = eventFactory.createFromString(eventString1);
        String seqString = seq.toString();
        assertTrue("Event string and event sequence string doesn't match",
                seqString.equals(seqString1Expected));

        seq = eventFactory.createFromString(eventString2);
        seqString = seq.toString();
        assertTrue("Event string and event sequence string doesn't match",
                seqString.equals(seqString2Expected));
    }

    /**
     * Run tests
     */
    public void runTests() {
        testEventFromStringCreation();
    }    
}
