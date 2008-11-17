/*
 * @(#)ByteToCharMS950.java	1.13 06/10/10
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

/**
 * Tables and data to convert MS950 to Unicode
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class ByteToCharMS950 extends ByteToCharBig5 {
    int _start, _end;
    public String getCharacterEncoding() {
        return "MS950";
    }

    public ByteToCharMS950() {
        super();
        _start = 0x40;
        _end = 0xFE;
    }

    protected char getUnicode(int byte1, int byte2) {
        int n = (_index1[byte1] & 0xf) * (_end - _start + 1) + (byte2 - _start);
        char unicode = _index2[_index1[byte1] >> 4].charAt(n);
        if (unicode == '\u0000')
            return (super.getUnicode(byte1, byte2));
        else
            return unicode;
    }
    private final static String _innerIndex0 = 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u2027\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\uFE51\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u2574\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u00AF\uFFE3\u0000\u02CD\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\uFF5E\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u2295\u2299\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\uFF0F\uFF3C\u2215" +
        "\uFE68\u0000\uFFE5\u0000\uFFE0\uFFE1\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u5341\u0000\u5345\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" +
        "\uFFFD\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
        "\u0000\u7881\u92B9\u88CF\u58BB\u6052\u7CA7\u5AFA" +
        "\u2554\u2566\u2557\u2560\u256C\u2563\u255A\u2569" +
        "\u255D\u2552\u2564\u2555\u255E\u256A\u2561\u2558" +
        "\u2567\u255B\u2553\u2565\u2556\u255F\u256B\u2562" +
        "\u2559\u2568\u255C\u2551\u2550\u256D\u256E\u2570" +
        "\u256F\u2593";
    private final static short _index1[] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0
        };
    String _index2[] = {
            _innerIndex0
        };
}
