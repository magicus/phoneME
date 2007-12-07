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

package com.sun.cdc.io.j2me.apdu;

import javax.microedition.apdu.*;
import java.io.IOException;
import com.sun.midp.i3test.TestCase;
import com.sun.midp.security.*;
import com.sun.cardreader.*;

/**
 * This test case tests basic com.sun.cdc.io.j2me.apdu.Cad class 
 * functionality.
 */
public class TestCad extends TestCase {
    /**
     * Test class creation and initialization.
     */
    private void testCad() 
            throws java.io.IOException, 
                   ClassNotFoundException, 
                   InstantiationException,
                   IllegalAccessException,
                   CardDeviceException {

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
            int slot = 0;
            Cad cad = new Cad(slot);
            byte[] request = {
                (byte)0x00, (byte)0xA4, 
                    (byte)0x04, (byte)0x00, (byte)0x08,
                (byte)0xA0, (byte)0x00, (byte)0x00, 
                (byte)0x00, (byte)0x18, (byte)0x43, 
                (byte)0x4D, (byte)0x00, (byte)0x3F};
            
            int channel = cad.selectApplication(false, request);
            assertTrue(true);
    
            byte[] atr = cad.getATR();
            assertNotEmpty("Empty ATR", atr);
        } else {
            assertTrue(true);
        }
    }
    
    /**
     * Run tests.
     */
    public void runTests() {
        try {
            declare("testCad");
            testCad();

        }
        catch (Throwable t) {
            fail("" + t);
        }
    }
    
    /**
     * Tests if array arr is not empty.
     * @param message Message to say if array is empty.
     * @param arr Array to test.
     */
    public void assertNotEmpty(String message, byte[] arr) {

        boolean is_null = true;
        
        if (arr != null) {
            for (int i = 0; i < arr.length; i++) {
                if (arr[i] != (byte)0) {
                    is_null = false;
                }
            }
        }
        assertFalse(message, is_null);
    }
}
