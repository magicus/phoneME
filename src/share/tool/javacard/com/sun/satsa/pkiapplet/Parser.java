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

package com.sun.satsa.pkiapplet;

import javacard.framework.Util;

/**
 * Utility class for byte buffer parsing.
 */
public class Parser {

    /** Data buffer. */
    static byte[] data;
    /** Current offset. */
    static short offset;

    /**
     * Initialises the parser.
     * @param buff data buffer
     */
    static void init(byte[] buff) {
        data = buff;
        offset = 0;
    }

    /**
     * Returns the next byte from the buffer.
     * @return byte value
     */
    static byte getByte() {
        return data[offset++];
    }

    /**
     * Returns the next short value from the buffer.
     * @return short value
     */
    static short getShort() {
        short s = Util.getShort(data, offset);
        offset += 2;
        return s;
    }

    /**
     * Changes the current offset to skip sequence of bytes.
     * @param len the number of bytes to skip
     */
    static void skip(short len) {
        offset += len;
    }
}
