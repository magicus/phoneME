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
 *
 */


package com.sun.midp.romization;

import java.io.*;
import java.util.*;

/**
 * Binary output stream capable of writing data
 * in big/little endian format.
 */
public class BinaryOutputStream {
    /** Underlying stream for writing bytes into */
    protected DataOutputStream outputStream = null;

    /** true for big endian format, false for little */
    private boolean isBigEndian = false;

    /**
     * Constructor.
     *
     * @param out underlying output stream for writing bytes into
     * @param bigEndian true for big endian format, false for little
     */
    public BinaryOutputStream(OutputStream out, boolean bigEndian) {
        outputStream = new DataOutputStream(out);
        isBigEndian = bigEndian;
    }

    /**
     * Writes byte value into stream
     *
     * @param value byte value to write
     */
    public void writeByte(int value)
        throws java.io.IOException {

        outputStream.writeByte(value);
    }

    /**
     * Writes integer value into stream
     *
     * @param value integer value to write
     */
    public void writeInt(int value)
        throws java.io.IOException {

        if (isBigEndian) {
            outputStream.writeByte((value >> 24) & 0xFF);
            outputStream.writeByte((value >> 16) & 0xFF);
            outputStream.writeByte((value >> 8) & 0xFF);
            outputStream.writeByte(value & 0xFF);
        } else {
            outputStream.writeByte(value & 0xFF);
            outputStream.writeByte((value >> 8) & 0xFF);
            outputStream.writeByte((value >> 16) & 0xFF);
            outputStream.writeByte((value >> 24) & 0xFF);
        }
    }

    /**
     * Closes stream
     */
    public void close()
        throws java.io.IOException {

        outputStream.close();
    }
}
