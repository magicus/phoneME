/*
 * @(#)CharToByteCp420.java	1.16 06/10/10
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
 * Tables and data to convert Unicode to Cp420
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class CharToByteCp420 extends CharToByteSingleByte {
    public String getCharacterEncoding() {
        return "Cp420";
    }

    public CharToByteCp420() {
        super.mask1 = 0xFF00;
        super.mask2 = 0x00FF;
        super.shift = 8;
        super.index1 = index1;
        super.index2 = index2;
    }
    private final static String index2 =

        "\u0000\u0001\u0002\u0003\u0037\u002D\u002E\u002F" + 
        "\u0016\u0005\u0025\u000B\f\r\u000E\u000F" + 
        "\u0010\u0011\u0012\u0013\u003C\u003D\u0032\u0026" + 
        "\u0018\u0019\u003F\'\u001C\u001D\u001E\u001F" + 
        "\u0040\u005A\u007F\u007B\u005B\u006C\u0050\u007D" + 
        "\u004D\u005D\\\u004E\u006B\u0060\u004B\u0061" + 
        "\u00F0\u00F1\u00F2\u00F3\u00F4\u00F5\u00F6\u00F7" + 
        "\u00F8\u00F9\u007A\u005E\u004C\u007E\u006E\u006F" + 
        "\u007C\u00C1\u00C2\u00C3\u00C4\u00C5\u00C6\u00C7" + 
        "\u00C8\u00C9\u00D1\u00D2\u00D3\u00D4\u00D5\u00D6" + 
        "\u00D7\u00D8\u00D9\u00E2\u00E3\u00E4\u00E5\u00E6" + 
        "\u00E7\u00E8\u00E9\u0000\u0000\u0000\u0000\u006D" + 
        "\u0000\u0081\u0082\u0083\u0084\u0085\u0086\u0087" + 
        "\u0088\u0089\u0091\u0092\u0093\u0094\u0095\u0096" + 
        "\u0097\u0098\u0099\u00A2\u00A3\u00A4\u00A5\u00A6" + 
        "\u00A7\u00A8\u00A9\u0000\u004F\u0000\u0000\u0007" + 
        "\u0020\u0021\"\u0023\u0024\u0015\u0006\u0017" + 
        "\u0028\u0029\u002A\u002B\u002C\t\n\u001B" + 
        "\u0030\u0031\u001A\u0033\u0034\u0035\u0036\b" + 
        "\u0038\u0039\u003A\u003B\u0004\u0014\u003E\u00FF" + 
        "\u0041\u0000\u004A\u0000\u0000\u0000\u006A\u0000" + 
        "\u0000\u0000\u0000\u0000\u005F\u00CA\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u00E0" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u00A1" + 
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
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0079\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u00C0" + 
        "\u0000\u0000\u0000\u00D0\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0044\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u00DF\u00EA\u00EB\u00ED" + 
        "\u00EE\u00EF\u00FB\u00FC\u00FD\u00FE\u0000\u0000" + 
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
        "\u0000\u0000\u0000\u0000\u00E1\u0000\u0000\u0000" + 
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
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + 
        "\u0000\u0000\u0000\u0000\u0000\u008B\u0080\u0077" + 
        "\u008D\u0000\u0000\u0000\u0000\u0045\u0000\u0000" + 
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
        "\u0000\u0000\u0042\u0043\u0000\u0000\u0046\u0047" + 
        "\u0048\u0049\u0051\u0052\u0000\u0000\u0000\u0000" + 
        "\u0000\u0055\u0000\u0056\u0057\u0058\u0000\u0059" + 
        "\u0000\u0062\u0000\u0063\u0000\u0064\u0000\u0065" + 
        "\u0000\u0066\u0000\u0067\u0000\u0068\u0000\u0069" + 
        "\u0000\u0070\u0000\u0071\u0000\u0072\u0000\u0073" + 
        "\u0000\u0074\u0000\u0075\u0000\u0076\u0000\u0000" + 
        "\u0000\u0078\u0000\u0000\u0000\u008A\u0000\u0000" + 
        "\u0000\u008C\u0000\u0000\u0000\u008E\u0000\u0000" + 
        "\u0000\u008F\u0000\u0000\u0000\u0090\u0000\u009A" + 
        "\u009B\u009C\u009D\u009E\u009F\u00A0\u00AA\u00AB" + 
        "\u0000\u00AC\u0000\u00AD\u0000\u00AE\u0000\u00AF" + 
        "\u0000\u00B0\u0000\u00B1\u0000\u00BA\u0000\u00BB" + 
        "\u0000\u00BC\u0000\u00BD\u0000\u00BE\u0000\u00BF" + 
        "\u0000\u00CB\u00CD\u00CF\u0000\u00DA\u00DB\u00DC" + 
        "\u00DD\u00DE\u0000\u00B2\u00B3\u00B4\u00B5\u0000" + 
        "\u0000\u00B8\u00B9\u0000\u0000\u0000"; 
    private final static short index1[] = {
            0, 248, 248, 248, 248, 248, 492, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            741, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 
            248, 248, 248, 248, 248, 248, 248, 248, 753, 248, 248, 248, 248, 248, 1006, 248, 
        };
}
