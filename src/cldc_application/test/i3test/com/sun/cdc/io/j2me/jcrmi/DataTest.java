/*
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
package com.sun.cdc.io.j2me.jcrmi;

import javax.microedition.jcrmi.*;
import com.sun.midp.i3test.TestCase;

import javax.microedition.jcrmi.JavaCardRMIConnection;
import java.rmi.RemoteException;

import com.sun.cardreader.CardDeviceException;
import com.sun.cardreader.SlotFactory;

/**
 * This test class verifies Java Card RMI Client API.
 */
public class DataTest extends TestCase {

    byte[] byteValues = {-128, -65, -64, -1, 0, 1, 88, 127};
    boolean[] booleanValues = {true, false, true};
    short[] shortValues = {-32768, -4097, -4096, -256, -128, -1, 0, 1,
                         255, 256, 32767};
    String appletURL = "jcrmi:0;AID=a0.0.0.0.62.bb.bb.bb.bb";

    /**
     * Tests data transfer.
     */
    public void testData() 
            throws java.io.IOException, CardDeviceException {
        JavaCardRMIConnection con = null;
        Remote1 rem;
        boolean stub_flag = false;
        
        try {
            SlotFactory.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }

        if (!stub_flag) {
            con = (JavaCardRMIConnection) 
                new com.sun.midp.io.j2me.jcrmi.Protocol();
            ((com.sun.midp.io.j2me.jcrmi.Protocol)con)
                .openPrim(appletURL, 0, false);
            rem = (Remote1)con.getInitialReference();
            assertNotNull("Initial reference", rem);
            rem.voidMethod();
            assertTrue("void method", true);
            byte[] b = rem.getByteArray(byteValues);
            if (b == null || b.length != byteValues.length) {
                assertTrue("byte[] method(byte[]) bad return value", false);
            } else {
                for (int i = 0; i < b.length; i++) {
                    if (b[i] != byteValues[i]) {
                        assertEquals("byte array", b[i], byteValues[i]);
                        break;
                    }
                }
                assertTrue("byte[] method(byte[])", true);
            }
            
            short[] s = rem.getShortArray(shortValues);
            if (s == null || s.length != shortValues.length) {
                assertTrue("short[] method(short[]) bad return value", false);
            } else {
                for (int i = 0; i < s.length; i++) {
                    if (s[i] != shortValues[i]) {
                        assertEquals("short array", s[i], shortValues[i]);
                        break;
                    }
                }
                assertTrue("short[] method(short[])", true);
            }
            
            boolean[] l = rem.getBooleanArray(booleanValues);
            if (l == null || l.length != booleanValues.length) {
                assertTrue("boolean[] method(boolean[]) bad return value", false);
            } else {
                for (int i = 0; i < l.length; i++) {
                    if (l[i] != booleanValues[i]) {
                        assertTrue("boolean array", (l[i] == booleanValues[i]));
                        break;
                    }
                }
                assertTrue("boolean[] method(boolean[])", true);
            }
            
            con.close();
        }
        assertTrue(true);
    }
    
    /**
     * Tests exception handling.
     */
    public void testException() 
            throws java.io.IOException, CardDeviceException {
        JavaCardRMIConnection con = null;
        Remote1 rem;
        boolean stub_flag = false;
        
        try {
            SlotFactory.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }
        
        if (!stub_flag) {
            con = (JavaCardRMIConnection) 
                new com.sun.midp.io.j2me.jcrmi.Protocol();
            ((com.sun.midp.io.j2me.jcrmi.Protocol)con)
                .openPrim(appletURL, 0, false);
            rem = (Remote1)con.getInitialReference();
            assertNotNull("Initial reference", rem);
            
            try {
                rem.throwException((short)12);
            }
            catch (java.rmi.RemoteException e) {
                assertTrue("proper exception", true);
            }
            catch (Throwable e1) {
                assertTrue("improper exception: " + e1, false);
            }
            
            try {
                rem.throwException((short)22);
            }
            catch (javacard.framework.service.ServiceException e) {
                assertTrue("proper exception", true);
            }
            catch (Throwable e1) {
                assertTrue("improper exception: " + e1, false);
            }
            
            try {
                rem.throwException((short)11);
            }
            catch (java.io.IOException e) {
                assertTrue("proper exception", true);
            }
            catch (Throwable e1) {
                assertTrue("improper exception: " + e1, false);
            }
            
            try {
                rem.throwSubclass((short)11);
            }
            catch (java.io.IOException e) {
                assertTrue("proper exception", true);
            }
            catch (Throwable e1) {
                assertTrue("improper exception: " + e1, false);
            }
            
            con.close();
        } else {
            assertTrue(true);
        }
    }
    
    /**
     * Run tests.
     */
    public void runTests() {
        try {
            declare("testData");
            testData();

            declare("testException");
            testException();

        }
        catch (Throwable t) {
            fail("" + t);
        }
    }
}
