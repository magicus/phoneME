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

/**
 * Represents a romized byte array.
 */
public class RomizedByteArray {
    /** romized binary data */
    private byte[] data;

    /**
     * Constructor
     *
     * @param dataBytes romized image data
     */
    public RomizedByteArray(byte dataBytes[]) {
        data = dataBytes;
    }

    /**
     * Returns the size of the array.
     *
     * @return number of bytes in the array
     */
    public int size() {
        if (data == null) {
            return 0;
        }
        return data.length; 
    }

    /**
     * Prints romized image data as C array
     *
     * @param writer where to print
     * @param indent indent string for each row
     * @param maxColumns max number of columns
     */
    public void printDataArray(PrintWriter writer,
                               String indent, int maxColumns) {
        int len = data.length;

        writer.print(indent);
        for (int i = 0; i < len; i++) {
            writer.print(toHex(data[i]));
            if (i != len - 1) {
                writer.print(", ");

                if ((i > 0) && ((i+1) % maxColumns == 0)) {
                    writer.println("");
                    writer.print(indent);
                }
            }
        }
    }

    /**
     * Converts byte to a hex string
     *
     * @param b byte value to convert
     * @return hex representation of byte
     */
    private static String toHex(byte b) {
        Integer I = new Integer((((int)b) << 24) >>> 24);
        int i = I.intValue();

        if (i < (byte)16) {
            return "0x0" + Integer.toString(i, 16);
        } else {
            return "0x" + Integer.toString(i, 16);
        }
    }
}
