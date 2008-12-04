/*
 * @(#)ByteToCharCp949C.java	1.11 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.io;

import sun.io.*;

public class ByteToCharCp949C extends ByteToCharDBCS_ASCII {
    protected static final String singleByteToChar;
    protected static final boolean leadByte[];
    protected static final short   index1[];
    protected static final String  index2;
    protected static final int     mask1;
    protected static final int     mask2;
    protected static final int     shift;
    static {
        ByteToCharDBCS_ASCII y = new ByteToCharCp949();
        mask1 = y.mask1;
        mask2 = y.mask2;
        shift = y.shift;
        leadByte = y.leadByte;
        index1 = y.index1;
        index2 = y.index2;
        /* Fix converter to pass through 0x00 to 0x7f unchanged to U+0000 to U+007F */
        String indexs = "";
        for (char c = '\0'; c < '\u0080'; ++c) indexs += c;
        singleByteToChar = indexs + y.singleByteToChar.substring(indexs.length());
    }
    public String getCharacterEncoding() {
        return "Cp949C";
    }

    ByteToCharCp949C() {
        super();
        super.mask1 = mask1;
        super.mask2 = mask2;
        super.shift = shift;
        super.leadByte = leadByte;
        super.singleByteToChar = singleByteToChar;
        super.index1 = index1;
        super.index2 = index2;
    }
}
