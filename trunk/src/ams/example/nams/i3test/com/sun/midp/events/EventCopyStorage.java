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

import java.util.Vector;

import com.sun.midp.events.NativeEvent;
import com.sun.midp.i3test.TestCase;
    
/**
 * Storage of native event copies for events being posted to evet queue.
 * It is expected that NamsAPIWrapper uses this class each time it calls 
 * NAMS API method.
 */
public class EventCopyStorage {
    
    /** storage for Native Event cpopies */
    private Vector storage = new Vector(10, 10);
    
    public synchronized int putTail(
        int t, 
        int i1, int i2, int i3, int i4,
        String s1, String s2, String s3, String s4, String s5, String s6) {
        
        NativeEvent e = new NativeEvent(t);
        
        e.intParam1 = i1;
        e.intParam2 = i2;
        e.intParam3 = i3;
        e.intParam4 = i4;
        e.stringParam1 = s1;
        e.stringParam2 = s2;
        e.stringParam3 = s3;
        e.stringParam4 = s4;
        e.stringParam5 = s5;
        e.stringParam6 = s6;
        
        storage.addElement((Object)e);
        return storage.size();
    }
    
    private boolean compareEvents(
        NativeEvent e1,
        int t, 
        int i1, int i2, int i3, int i4,
        String s1, String s2,  String s3, String s4, String s5, String s6,
        TestCase test,
        String message) {
        
        test.assertTrue(message + " 'type' field", e1.type == t);
        
        test.assertTrue(message + " 'intParam1' field", e1.intParam1 == i1);
        test.assertTrue(message + " 'intParam2' field", e1.intParam2 == i2);
        test.assertTrue(message + " 'intParam3' field", e1.intParam3 == i3);
        test.assertTrue(message + " 'intParam4' field", e1.intParam4 == i4);
        
        test.assertEquals(message + " 'stringParam1' field",  
                e1.stringParam1, s1);
        test.assertEquals(message + " 'stringParam2' field",  
                e1.stringParam2, s2);
        test.assertEquals(message + " 'stringParam3' field",  
                e1.stringParam3, s3);
        test.assertEquals(message + " 'stringParam4' field",  
                e1.stringParam4, s4);
        test.assertEquals(message + " 'stringParam5' field",  
                e1.stringParam5, s5);
        test.assertEquals(message + " 'stringParam6' field",  
                e1.stringParam6, s6);
        
        return 
            (e1.type == t) &&
                
            (e1.intParam1 == i1) &&
            (e1.intParam2 == i2) &&
            (e1.intParam3 == i3) &&
            (e1.intParam4 == i4) &&
                
            (e1.stringParam1 == s1) &&
            (e1.stringParam2 == s2) &&
            (e1.stringParam3 == s3) &&
            (e1.stringParam4 == s4) &&
            (e1.stringParam5 == s5) &&
            (e1.stringParam6 == s6);
    }
    
    public synchronized NativeEvent getHead() {
        NativeEvent e = (NativeEvent)storage.elementAt(0);
        storage.removeElementAt(0);
        return e;
    }
    
    public synchronized boolean getHead(
        NativeEvent e, 
        TestCase test,
        String message) {
        NativeEvent e1 = (NativeEvent)storage.elementAt(0);
        storage.removeElementAt(0);
        
        return compareEvents(e1, 
            e.type, 
            e.intParam1, e.intParam2, e.intParam3, e.intParam4, 
            e.stringParam1, e.stringParam2, e.stringParam4, 
            e.stringParam4, e.stringParam5, e.stringParam6, 
            test,
            message);
    }
        
    public synchronized boolean getHead(
        int t, 
        int i1, int i2, int i3, int i4,
        String s1, String s2,  String s3, String s4, String s5, String s6, 
        TestCase test,
        String message) {
        
        NativeEvent e1 = (NativeEvent)storage.elementAt(0);
        storage.removeElementAt(0);
        
        return compareEvents(e1, 
                t, 
                i1, i2, i3, i4, 
                s1, s2, s3, s4, s5, s6, 
                test, 
                message);
    }
    
    public synchronized NativeEvent checkHead() {
        return (NativeEvent)storage.elementAt(0);
    }

    public synchronized boolean checkHead(
        NativeEvent e, 
        TestCase test,
        String message) {
        
        NativeEvent e1 = (NativeEvent)storage.elementAt(0);
        
        return compareEvents(e1, 
            e.type, 
            e.intParam1, e.intParam2, e.intParam3, e.intParam4, 
            e.stringParam1, e.stringParam2, e.stringParam4, 
            e.stringParam4, e.stringParam5, e.stringParam6, 
            test, 
            message);
    }
    public synchronized boolean checkHead(
        int t, 
        int i1, int i2, int i3, int i4,
        String s1, String s2,  String s3, String s4, String s5, String s6, 
        TestCase test,
        String message) {
        
        NativeEvent e1 = (NativeEvent)storage.elementAt(0);
        
        return compareEvents(e1, 
                t, 
                i1, i2, i3, i4, 
                s1, s2, s3, s4, s5, s6, 
                test, 
                message);
    }
    
    public synchronized int checkSize() {
        return storage.size();
    }
    
    public synchronized void clearAll() {
        storage.removeAllElements();
    }
}
