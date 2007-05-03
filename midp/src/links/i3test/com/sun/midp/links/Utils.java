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

package com.sun.midp.links;

import java.util.Random;

/**
 * Various shared utilities for Links unit testing.
 */
public class Utils {

    /**
     * Sleeps for the indicated number of milliseconds.
     */
    public static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException ignore) { }
    }

    /**
     * Utility method to fill in a byte array with some reasonable data, 
     * derived from the string passed as an argument.
     */
    public static byte[] extractBytes(String str) {
        int len = str.length();
        byte[] data = new byte[len];

        for (int i = 0; i < len; i++) {
            data[i] = (byte)(str.charAt(i) & 0xff);
        }

        return data;
    }

    /**
     * Compares two byte arrays for equality.
     */
    public static boolean bytesEqual(byte[] a, byte[] b) {
        if (a == b) {
            // handles null == null
            return true;
        }

        if (a == null || b == null) {
            return false;
        }

        // a and b both non-null

        if (a.length != b.length) {
            return false;
        }

        for (int i = 0; i < a.length; i++) {
            if (a[i] != b[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * Compares a subrange of byte array a to the entirety of b.
     */
    public static boolean bytesEqual(byte[] a, int off, int len, byte[] b) {
        byte[] sub = new byte[len];
        System.arraycopy(a, off, sub, 0, len);
        return bytesEqual(sub, b);
    }

    /**
     * Fills a byte array with random data.
     */
    public static void fillRandom(byte[] ba) {
        Random rand = new Random();
        int randInt = 0;   // current set of random data

        for (int i = 0; i < ba.length; i++) {
            int r = i % 4;
            if (r == 0) {
                randInt = rand.nextInt();
            }
            ba[i] = (byte)((randInt >> (8 * r)) & 0xFF);
        }
    }

    
    /**
     * Prints out the ASCII representation of a number without allocating any 
     * memory.
     */
    static void p(int intval) {
        int val = intval;
        int power = 1000000000;

        while (power > 0) {
            System.out.write(((val / power) % 10) + 48);
            power /= 10;
        }

        System.out.write(10);
        System.out.flush();
    }

    /**
     * Fills memory and returns an object that refers to all of what was 
     * allocated. The intent is that all subsequent allocations will cause 
     * OutOfMemoryError until the reference to the returned object is 
     * discarded.
     */
    public static Object fillMemory() {
        byte[][] arrays = new byte[100][];
        int size = 1 << 30; 
        int idx = 0;

        while (true) {
            try {
                while (true) {
                    arrays[idx++] = new byte[size];
                    // p(size);
                }
            } catch (OutOfMemoryError oome) {
                if (size == 0) {
                    break;
                } else {
                    size /= 2;
                }
            }
        }
        return arrays;
    }

    /**
     * Private constructor to prevent creation on instances.
     */
    private Utils() {
    }

    public static native void forceGC();

    public static native int[] getFreedRendezvousPoints();

    public static native int getRefCount(Link link);
}
