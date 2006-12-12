/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
    
    /**
     * Run tests.
     */
    public void runTests() {
        try {
            declare("testOne");
            testOne();
            
            declare("testTwo");
            testTwo();
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

   /**
     * IMPL_NOTE delete
     * Prints the a TLV structure, recursing down for constructed types.
     * @param tlv the TLV structure to be printed
     * /
    public static void print(TLV tlv) {
        print(System.out, 0, tlv);
    }

    /**
     * IMPL_NOTE delete
     * Prints the a TLV structure, recursing down for constructed types.
     * @param out output stream
     * @param tlv the TLV structure to be printed
     * /
    private static void print(PrintStream out, TLV tlv) {
        print(out, 0, tlv);
    }

    /**
     * IMPL_NOTE delete
     * Prints the a TLV structure, recursing down for constructed types.
     * @param out output stream
     * @param level what level this TLV is at
     * @param tlv the TLV structure to be printed
     * /
    private static void print(PrintStream out, int level, TLV tlv) {

        for (int i = 0; i < level; i++) {
            out.print("    ");
        }
        if (tlv == null) {
            out.println("(null)");
            return;
        }

        byte[] buffer;

        if (tlv.data != null) {
            buffer = tlv.data;
        } else {
            buffer = tlv.getDERData();
        }

        if (true) {
            out.print(hex(tlv.type) + 
                "," + hex(tlv.length) + ",");
            if (tlv.child == null) {
                for (int i = tlv.valueOffset; 
                            i < tlv.length + tlv.valueOffset; i++) {
                    out.print(hex(buffer[i]) + ",");
                }
                out.println();
            } else {
                out.println();
                print(out, level + 1, tlv.child);
            }
        } else {
            if (tlv.child == null) {
                out.print("Type: 0x" + Integer.toHexString(tlv.type) +
                                 " length: " + tlv.length + " value: ");
                if (tlv.type == TLV.PRINTSTR_TYPE ||
                    tlv.type == TLV.TELETEXSTR_TYPE ||
                    tlv.type == TLV.UTF8STR_TYPE ||
                    tlv.type == TLV.IA5STR_TYPE ||
                    tlv.type == TLV.UNIVSTR_TYPE) {
                    try {
                        out.print(new String(buffer, 
                                    tlv.valueOffset, tlv.length, Utils.utf8));
                    } catch (UnsupportedEncodingException e) {
                        // ignore
                    }
                } else if (tlv.type == TLV.OID_TYPE) {
                    out.print(Utils.OIDtoString(buffer, 
                                                tlv.valueOffset, tlv.length));
                } else {
                    out.print(Utils.hexNumber(buffer, 
                                                tlv.valueOffset, tlv.length));
                }
    
                out.println();
            } else {
                if (tlv.type == TLV.SET_TYPE) {
                    out.print("Set:");
                } else {
                    out.print("Sequence:");
                }
    
                out.println("  (0x" + Integer.toHexString(tlv.type) +
                                 " " + tlv.length + ")");
    
                print(out, level + 1, tlv.child);
            }
        }

        if (tlv.next != null) {
            print(out, level, tlv.next);
        }
    }

    /**
     * IMPL_NOTE delete
     * Converts integer value into hex representation
     * @param value the value
     * @return a string which contains result
     * /
    private static String hex(int value) {
        String ret = "";
        value &= 0xff;
        if (value > 127) {
            ret += "(byte)";
        }
        ret += "0x";
        if (value < 16) {
            ret += "0";
        }
        return ret + Integer.toHexString(value);
    }
/*  */

}

