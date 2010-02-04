/*
 * @(#)ByteToCharCp921.java	1.14 06/10/10
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
 * A table to convert Cp921 to Unicode
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class ByteToCharCp921 extends ByteToCharSingleByte {
    public String getCharacterEncoding() {
        return "Cp921";
    }

    public ByteToCharCp921() {
        super.byteToCharTable = byteToCharTable;
    }
    private final static String byteToCharTable =

        "\u0080\u0081\u0082\u0083\u0084\u0085\u0086\u0087" + 	// 0x80 - 0x87
        "\u0088\u0089\u008A\u008B\u008C\u008D\u008E\u008F" + 	// 0x88 - 0x8F
        "\u0090\u0091\u0092\u0093\u0094\u0095\u0096\u0097" + 	// 0x90 - 0x97
        "\u0098\u0099\u009A\u009B\u009C\u009D\u009E\u009F" + 	// 0x98 - 0x9F
        "\u00A0\u201D\u00A2\u00A3\u00A4\u201E\u00A6\u00A7" + 	// 0xA0 - 0xA7
        "\u00D8\u00A9\u0156\u00AB\u00AC\u00AD\u00AE\u00C6" + 	// 0xA8 - 0xAF
        "\u00B0\u00B1\u00B2\u00B3\u201C\u00B5\u00B6\u00B7" + 	// 0xB0 - 0xB7
        "\u00F8\u00B9\u0157\u00BB\u00BC\u00BD\u00BE\u00E6" + 	// 0xB8 - 0xBF
        "\u0104\u012E\u0100\u0106\u00C4\u00C5\u0118\u0112" + 	// 0xC0 - 0xC7
        "\u010C\u00C9\u0179\u0116\u0122\u0136\u012A\u013B" + 	// 0xC8 - 0xCF
        "\u0160\u0143\u0145\u00D3\u014C\u00D5\u00D6\u00D7" + 	// 0xD0 - 0xD7
        "\u0172\u0141\u015A\u016A\u00DC\u017B\u017D\u00DF" + 	// 0xD8 - 0xDF
        "\u0105\u012F\u0101\u0107\u00E4\u00E5\u0119\u0113" + 	// 0xE0 - 0xE7
        "\u010D\u00E9\u017A\u0117\u0123\u0137\u012B\u013C" + 	// 0xE8 - 0xEF
        "\u0161\u0144\u0146\u00F3\u014D\u00F5\u00F6\u00F7" + 	// 0xF0 - 0xF7
        "\u0173\u0142\u015B\u016B\u00FC\u017C\u017E\u2019" + 	// 0xF8 - 0xFF
        "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007" + 	// 0x00 - 0x07
        "\b\t\n\u000B\f\r\u000E\u000F" + 	// 0x08 - 0x0F
        "\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017" + 	// 0x10 - 0x17
        "\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F" + 	// 0x18 - 0x1F
        "\u0020\u0021\"\u0023\u0024\u0025\u0026\'" + 	// 0x20 - 0x27
        "\u0028\u0029\u002A\u002B\u002C\u002D\u002E\u002F" + 	// 0x28 - 0x2F
        "\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037" + 	// 0x30 - 0x37
        "\u0038\u0039\u003A\u003B\u003C\u003D\u003E\u003F" + 	// 0x38 - 0x3F
        "\u0040\u0041\u0042\u0043\u0044\u0045\u0046\u0047" + 	// 0x40 - 0x47
        "\u0048\u0049\u004A\u004B\u004C\u004D\u004E\u004F" + 	// 0x48 - 0x4F
        "\u0050\u0051\u0052\u0053\u0054\u0055\u0056\u0057" + 	// 0x50 - 0x57
        "\u0058\u0059\u005A\u005B\\\u005D\u005E\u005F" + 	// 0x58 - 0x5F
        "\u0060\u0061\u0062\u0063\u0064\u0065\u0066\u0067" + 	// 0x60 - 0x67
        "\u0068\u0069\u006A\u006B\u006C\u006D\u006E\u006F" + 	// 0x68 - 0x6F
        "\u0070\u0071\u0072\u0073\u0074\u0075\u0076\u0077" + 	// 0x70 - 0x77
        "\u0078\u0079\u007A\u007B\u007C\u007D\u007E\u007F"; 	// 0x78 - 0x7F
}
