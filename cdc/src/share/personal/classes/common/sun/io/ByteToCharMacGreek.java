/*
 * @(#)ByteToCharMacGreek.java	1.14 06/10/10
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
 * A table to convert MacGreek to Unicode
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class ByteToCharMacGreek extends ByteToCharSingleByte {
    public String getCharacterEncoding() {
        return "MacGreek";
    }

    public ByteToCharMacGreek() {
        super.byteToCharTable = byteToCharTable;
    }
    private final static String byteToCharTable =

        "\u00C4\u00B9\u00B2\u00C9\u00B3\u00D6\u00DC\u0385" + 	// 0x80 - 0x87
        "\u00E0\u00E2\u00E4\u0384\u00A8\u00E7\u00E9\u00E8" + 	// 0x88 - 0x8F
        "\u00EA\u00EB\u00A3\u2122\u00EE\u00EF\u2022\u00BD" + 	// 0x90 - 0x97
        "\u2030\u00F4\u00F6\u00A6\u00AD\u00F9\u00FB\u00FC" + 	// 0x98 - 0x9F
        "\u2020\u0393\u0394\u0398\u039B\u039E\u03A0\u00DF" + 	// 0xA0 - 0xA7
        "\u00AE\u00A9\u03A3\u03AA\u00A7\u2260\u00B0\u0387" + 	// 0xA8 - 0xAF
        "\u0391\u00B1\u2264\u2265\u00A5\u0392\u0395\u0396" + 	// 0xB0 - 0xB7
        "\u0397\u0399\u039A\u039C\u03A6\u03AB\u03A8\u03A9" + 	// 0xB8 - 0xBF
        "\u03AC\u039D\u00AC\u039F\u03A1\u2248\u03A4\u00AB" + 	// 0xC0 - 0xC7
        "\u00BB\u2026\u00A0\u03A5\u03A7\u0386\u0388\u0153" + 	// 0xC8 - 0xCF
        "\u2013\u2015\u201C\u201D\u2018\u2019\u00F7\u0389" + 	// 0xD0 - 0xD7
        "\u038A\u038C\u038E\u03AD\u03AE\u03AF\u03CC\u038F" + 	// 0xD8 - 0xDF
        "\u03CD\u03B1\u03B2\u03C8\u03B4\u03B5\u03C6\u03B3" + 	// 0xE0 - 0xE7
        "\u03B7\u03B9\u03BE\u03BA\u03BB\u03BC\u03BD\u03BF" + 	// 0xE8 - 0xEF
        "\u03C0\u03CE\u03C1\u03C3\u03C4\u03B8\u03C9\u03C2" + 	// 0xF0 - 0xF7
        "\u03C7\u03C5\u03B6\u03CA\u03CB\u0390\u03B0\uFFFD" + 	// 0xF8 - 0xFF
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
