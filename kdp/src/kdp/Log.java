/*
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

package kdp;

public class Log {

    static int verbose = 0;

    public static void SET_LOG(int level) {
        verbose = level;
    }

    public static void LOG(int level, String s) {
        if (verbose >= level)
            System.out.print(s);
    }

    public static void LOGE(int level, String s) {
        if (verbose == level)
            System.out.print(s);
    }

    public static void LOGN(int level, String s) {
        if (verbose >= level)
            System.out.println(s);
    }

    public static void LOGN(int level, Object o) {
        if (verbose >= level)
            System.out.println(o);
    }

    public static void LOGNE(int level, String s) {
        if (verbose == level)
            System.out.println(s);
    }

    public static String intToHex(int x) {
        return "0x" + Integer.toHexString(x);
    }

    public static String longToHex(long x) {
        return "0x" + Long.toHexString(x);
    }
}
