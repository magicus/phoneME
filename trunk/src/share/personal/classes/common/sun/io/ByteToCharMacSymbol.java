/*
 * @(#)ByteToCharMacSymbol.java	1.14 06/10/10
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
 * A table to convert MacSymbol to Unicode
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class ByteToCharMacSymbol extends ByteToCharSingleByte {
    public String getCharacterEncoding() {
        return "MacSymbol";
    }

    public ByteToCharMacSymbol() {
        super.byteToCharTable = byteToCharTable;
    }
    private final static String byteToCharTable =

        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" + 	// 0x80 - 0x87
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" + 	// 0x88 - 0x8F
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" + 	// 0x90 - 0x97
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" + 	// 0x98 - 0x9F
        "\uFFFD\u03D2\u2032\u2264\u2044\u221E\u0192\u2663" + 	// 0xA0 - 0xA7
        "\u2666\u2665\u2660\u2194\u2190\u2191\u2192\u2193" + 	// 0xA8 - 0xAF
        "\u00B0\u00B1\u2033\u2265\u00D7\u221D\u2202\u2022" + 	// 0xB0 - 0xB7
        "\u00F7\u2260\u2261\u2248\u2026\uFFFD\uFFFD\u21B5" + 	// 0xB8 - 0xBF
        "\u2135\u2111\u211C\u2118\u2297\u2295\u2205\u2229" + 	// 0xC0 - 0xC7
        "\u222A\u2283\u2287\u2284\u2282\u2286\u2208\u2209" + 	// 0xC8 - 0xCF
        "\u2220\u2207\u00AE\u00A9\u2122\u220F\u221A\u22C5" + 	// 0xD0 - 0xD7
        "\u00AC\u2227\u2228\u21D4\u21D0\u21D1\u21D2\u21D3" + 	// 0xD8 - 0xDF
        "\u22C4\u2329\uFFFD\uFFFD\uFFFD\u2211\uFFFD\uFFFD" + 	// 0xE0 - 0xE7
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" + 	// 0xE8 - 0xEF
        "\uFFFD\u232A\u222B\u2320\uFFFD\u2321\uFFFD\uFFFD" + 	// 0xF0 - 0xF7
        "\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD" + 	// 0xF8 - 0xFF
        "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007" + 	// 0x00 - 0x07
        "\b\t\n\u000B\f\r\u000E\u000F" + 	// 0x08 - 0x0F
        "\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017" + 	// 0x10 - 0x17
        "\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F" + 	// 0x18 - 0x1F
        "\u0020\u0021\u2200\u0023\u2203\u0025\u0026\u220D" + 	// 0x20 - 0x27
        "\u0028\u0029\u2217\u002B\u002C\u2212\u002E\u002F" + 	// 0x28 - 0x2F
        "\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037" + 	// 0x30 - 0x37
        "\u0038\u0039\u003A\u003B\u003C\u003D\u003E\u003F" + 	// 0x38 - 0x3F
        "\u2245\u0391\u0392\u03A7\u0394\u0395\u03A6\u0393" + 	// 0x40 - 0x47
        "\u0397\u0399\u03D1\u039A\u039B\u039C\u039D\u039F" + 	// 0x48 - 0x4F
        "\u03A0\u0398\u03A1\u03A3\u03A4\u03A5\u03C2\u03A9" + 	// 0x50 - 0x57
        "\u039E\u03A8\u0396\u005B\u2234\u005D\u22A5\u005F" + 	// 0x58 - 0x5F
        "\uFFFD\u03B1\u03B2\u03C7\u03B4\u03B5\u03C6\u03B3" + 	// 0x60 - 0x67
        "\u03B7\u03B9\u03D5\u03BA\u03BB\u03BC\u03BD\u03BF" + 	// 0x68 - 0x6F
        "\u03C0\u03B8\u03C1\u03C3\u03C4\u03C5\u03D6\u03C9" + 	// 0x70 - 0x77
        "\u03BE\u03C8\u03B6\u007B\u007C\u007D\u223C\u007F"; 	// 0x78 - 0x7F
}
