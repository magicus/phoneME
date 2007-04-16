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

package com.sun.satsa.utils;

import com.sun.midp.i3test.TestCase;

import com.sun.satsa.pki.RFC2253Name;
import com.sun.satsa.util.TLV;
import com.sun.satsa.util.TLVException;
import com.sun.satsa.util.Utils;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;

/**
 * This test case tests DomainComponent encoding 
 * made by RFC2253 class.
 */
public class TestIA5 extends TestCase {
    static byte[] testOneResult = {
        0x30,0x55,
            0x31,0x13,
                0x30,0x11,
                    0x06,0x0a,0x09,(byte)0x92,0x26,(byte)0x89,(byte)0x93,
                                          (byte)0xf2,0x2c,0x64,0x01,0x19,
                    0x16,0x03,0x63,0x6f,0x6d,
            0x31,0x13,
                0x30,0x11,
                    0x06,0x0a,0x09,(byte)0x92,0x26,(byte)0x89,(byte)0x93,
                                          (byte)0xf2,0x2c,0x64,0x01,0x19,
                    0x16,0x03,0x73,0x75,0x6e,
            0x31,0x13,
                0x30,0x11,
                    0x06,0x0a,0x09,(byte)0x92,0x26,(byte)0x89,(byte)0x93,
                                          (byte)0xf2,0x2c,0x64,0x01,0x19,
                    0x16,0x03,0x77,0x77,0x77,
            0x31,0x14,
                0x30,0x12,
                    0x06,0x03,0x55,0x04,0x03,
                    0x0c,0x0b,0x43,0x65,0x72,0x74,0x69,0x66,0x69,0x63,
                                                       0x61,0x74,0x65
    };
    static byte[] testTwoResult = {
        0x30,0x29,
            0x31,0x10,
                0x30,0x0e,
                    0x06,0x0a,0x09,(byte)0x92,0x26,(byte)0x89,(byte)0x93,
                                          (byte)0xf2,0x2c,0x64,0x01,0x19,
                    0x16,0x00,
            0x31,0x15,
                0x30,0x13,
                    0x06,0x03,0x55,0x04,0x03,
                    0x0c,0x0c,0x43,0x65,0x72,0x74,0x69,0x66,0x69,0x63,0x61,
                                                            0x74,0x65,0x31
    };

    /**
     * Tests DER encoding of DomainComponent.
     */
    private void testOne() {
        String nameInfo = 
            "cn=Certificate, dc=www, Dc=sun, dC=com";
        TLV name;
        boolean ok = true;

        try {
            name = new TLV(RFC2253Name.toDER(nameInfo), 0);
        } catch (TLVException e) {
            name = null;
            ok = false;
        }
        assertTrue("Invalid name", ok);
        assertTrue("Bad DER result", name != null && 
                    equal(name.getDERData(), testOneResult));
    }
    
    private void testTwo() {
        String nameInfo = 
            "cn=Certificate1, dc=";
        TLV name;
        boolean ok = true;

        try {
            name = new TLV(RFC2253Name.toDER(nameInfo), 0);
        } catch (TLVException e) {
            name = null;
            ok = false;
        }
        assertTrue("Invalid name", ok);
        assertTrue("Bad DER result", name != null && 
                    equal(name.getDERData(), testTwoResult));
    }
    
    private void testThree() {
        String nameInfo = 
         "cn=Certificate, OID.0.9.2342.19200300.100.1.25=www, dc=sun, dc=com";
        TLV name;
        boolean ok = true;

        try {
            name = new TLV(RFC2253Name.toDER(nameInfo), 0);
        } catch (TLVException e) {
            name = null;
            ok = false;
        }
        assertTrue("Invalid name", ok);
        assertTrue("Bad DER result", name != null && 
                    equal(name.getDERData(), testOneResult));
    }
    
    /**
     * Run tests.
     */
    public void runTests() {
        try {
            declare("testOne");
            testOne();
            
            declare("testTwo");
            testTwo();
            
            declare("testThree");
            testThree();
        }
        catch (Throwable t) {
            fail("" + t);
        }
    }
    
    /**
     * Compare two byte arrays.
     * @param one the first array
     * @param two the second array
     * @return true if arrays are equal, false otherwise
    */
    private boolean equal(byte[] one, byte[] two) {
        if (one.length != two.length) {
            return false;
        }
        for (int i = 0; i < one.length; i++) {
            if (one[i] != two[i]) {
                return false;
            }
        }
        return true;
    }

}

