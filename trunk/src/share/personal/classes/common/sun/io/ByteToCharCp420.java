/*
 * @(#)ByteToCharCp420.java	1.14 06/10/10
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
 * A table to convert Cp420 to Unicode
 *
 * @author  ConverterGenerator tool
 * @version >= JDK1.1.6
 */

public class ByteToCharCp420 extends ByteToCharSingleByte {
    public String getCharacterEncoding() {
        return "Cp420";
    }

    public ByteToCharCp420() {
        super.byteToCharTable = byteToCharTable;
    }
    private final static String byteToCharTable =

        "\uF8F5\u0061\u0062\u0063\u0064\u0065\u0066\u0067" + 	// 0x80 - 0x87
        "\u0068\u0069\uFEB7\uF8F4\uFEBB\uF8F7\uFEBF\uFEC3" + 	// 0x88 - 0x8F
        "\uFEC7\u006A\u006B\u006C\u006D\u006E\u006F\u0070" + 	// 0x90 - 0x97
        "\u0071\u0072\uFEC9\uFECA\uFECB\uFECC\uFECD\uFECE" + 	// 0x98 - 0x9F
        "\uFECF\u00F7\u0073\u0074\u0075\u0076\u0077\u0078" + 	// 0xA0 - 0xA7
        "\u0079\u007A\uFED0\uFED1\uFED3\uFED5\uFED7\uFED9" + 	// 0xA8 - 0xAF
        "\uFEDB\uFEDD\uFEF5\uFEF6\uFEF7\uFEF8\uFFFD\uFFFD" + 	// 0xB0 - 0xB7
        "\uFEFB\uFEFC\uFEDF\uFEE1\uFEE3\uFEE5\uFEE7\uFEE9" + 	// 0xB8 - 0xBF
        "\u061B\u0041\u0042\u0043\u0044\u0045\u0046\u0047" + 	// 0xC0 - 0xC7
        "\u0048\u0049\u00AD\uFEEB\uFFFD\uFEEC\uFFFD\uFEED" + 	// 0xC8 - 0xCF
        "\u061F\u004A\u004B\u004C\u004D\u004E\u004F\u0050" + 	// 0xD0 - 0xD7
        "\u0051\u0052\uFEEF\uFEF0\uFEF1\uFEF2\uFEF3\u0660" + 	// 0xD8 - 0xDF
        "\u00D7\u2007\u0053\u0054\u0055\u0056\u0057\u0058" + 	// 0xE0 - 0xE7
        "\u0059\u005A\u0661\u0662\uFFFD\u0663\u0664\u0665" + 	// 0xE8 - 0xEF
        "\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037" + 	// 0xF0 - 0xF7
        "\u0038\u0039\uFFFD\u0666\u0667\u0668\u0669\u009F" + 	// 0xF8 - 0xFF
        "\u0000\u0001\u0002\u0003\u009C\t\u0086\u007F" + 	// 0x00 - 0x07
        "\u0097\u008D\u008E\u000B\f\r\u000E\u000F" + 	// 0x08 - 0x0F
        "\u0010\u0011\u0012\u0013\u009D\u0085\b\u0087" + 	// 0x10 - 0x17
        "\u0018\u0019\u0092\u008F\u001C\u001D\u001E\u001F" + 	// 0x18 - 0x1F
        "\u0080\u0081\u0082\u0083\u0084\n\u0017\u001B" + 	// 0x20 - 0x27
        "\u0088\u0089\u008A\u008B\u008C\u0005\u0006\u0007" + 	// 0x28 - 0x2F
        "\u0090\u0091\u0016\u0093\u0094\u0095\u0096\u0004" + 	// 0x30 - 0x37
        "\u0098\u0099\u009A\u009B\u0014\u0015\u009E\u001A" + 	// 0x38 - 0x3F
        "\u0020\u00A0\uFE7C\uFE7D\u0640\uF8FC\uFE80\uFE81" + 	// 0x40 - 0x47
        "\uFE82\uFE83\u00A2\u002E\u003C\u0028\u002B\u007C" + 	// 0x48 - 0x4F
        "\u0026\uFE84\uFE85\uFFFD\uFFFD\uFE8B\uFE8D\uFE8E" + 	// 0x50 - 0x57
        "\uFE8F\uFE91\u0021\u0024\u002A\u0029\u003B\u00AC" + 	// 0x58 - 0x5F
        "\u002D\u002F\uFE93\uFE95\uFE97\uFE99\uFE9B\uFE9D" + 	// 0x60 - 0x67
        "\uFE9F\uFEA1\u00A6\u002C\u0025\u005F\u003E\u003F" + 	// 0x68 - 0x6F
        "\uFEA3\uFEA5\uFEA7\uFEA9\uFEAB\uFEAD\uFEAF\uF8F6" + 	// 0x70 - 0x77
        "\uFEB3\u060C\u003A\u0023\u0040\'\u003D\""; 	// 0x78 - 0x7F
}
