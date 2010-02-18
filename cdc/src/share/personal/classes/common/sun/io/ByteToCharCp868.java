/*
 * @(#)ByteToCharCp868.java	1.14 06/10/10
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
 * A table to convert Cp868 to Unicode
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class ByteToCharCp868 extends ByteToCharSingleByte {
    public String getCharacterEncoding() {
        return "Cp868";
    }

    public ByteToCharCp868() {
        super.byteToCharTable = byteToCharTable;
    }
    private final static String byteToCharTable =

        "\u06F0\u06F1\u06F2\u06F3\u06F4\u06F5\u06F6\u06F7" + 	// 0x80 - 0x87
        "\u06F8\u06F9\u060C\u061B\u061F\uFE81\uFE8D\uFE8E" + 	// 0x88 - 0x8F
        "\uF8FB\uFE8F\uFE91\uFB56\uFB58\uFE93\uFE95\uFE97" + 	// 0x90 - 0x97
        "\uFB66\uFB68\uFE99\uFE9B\uFE9D\uFE9F\uFB7A\uFB7C" + 	// 0x98 - 0x9F
        "\uFEA1\uFEA3\uFEA5\uFEA7\uFEA9\uFB88\uFEAB\uFEAD" + 	// 0xA0 - 0xA7
        "\uFB8C\uFEAF\uFB8A\uFEB1\uFEB3\uFEB5\u00AB\u00BB" + 	// 0xA8 - 0xAF
        "\u2591\u2592\u2593\u2502\u2524\uFEB7\uFEB9\uFEBB" + 	// 0xB0 - 0xB7
        "\uFEBD\u2563\u2551\u2557\u255D\uFEBF\uFEC3\u2510" + 	// 0xB8 - 0xBF
        "\u2514\u2534\u252C\u251C\u2500\u253C\uFEC7\uFEC9" + 	// 0xC0 - 0xC7
        "\u255A\u2554\u2569\u2566\u2560\u2550\u256C\uFECA" + 	// 0xC8 - 0xCF
        "\uFECB\uFECC\uFECD\uFECE\uFECF\uFED0\uFED1\uFED3" + 	// 0xD0 - 0xD7
        "\uFED5\u2518\u250C\u2588\u2584\uFED7\uFB8E\u2580" + 	// 0xD8 - 0xDF
        "\uFEDB\uFB92\uFB94\uFEDD\uFEDF\uFEE0\uFEE1\uFEE3" + 	// 0xE0 - 0xE7
        "\uFB9E\uFEE5\uFEE7\uFE85\uFEED\uFBA6\uFBA8\uFBA9" + 	// 0xE8 - 0xEF
        "\u00AD\uFBAA\uFE80\uFE89\uFE8A\uFE8B\uFBFC\uFBFD" + 	// 0xF0 - 0xF7
        "\uFBFE\uFBB0\uFBAE\uFE7C\uFE7D\uFFFD\u25A0\u00A0" + 	// 0xF8 - 0xFF
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
