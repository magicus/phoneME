/*
 * @(#)MessageUtils.java	1.16 06/10/10
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

package sun.misc;

/**
 * MessageUtils: miscellaneous utilities for handling error and status
 * properties and messages.
 *
 * @version 1.12, 08/19/02
 * @author Herb Jellinek
 */

public class MessageUtils {
    // can instantiate it for to allow less verbose use - via instance
    // instead of classname
    
    public MessageUtils() {}

    public static String subst(String patt, String arg) {
        String args[] = { arg };
        return subst(patt, args);
    }

    public static String subst(String patt, String arg1, String arg2) {
        String args[] = { arg1, arg2 };
        return subst(patt, args);
    }

    public static String subst(String patt, String arg1, String arg2,
        String arg3) {
        String args[] = { arg1, arg2, arg3 };
        return subst(patt, args);
    }

    public static String subst(String patt, String args[]) {
        StringBuffer result = new StringBuffer();
        int len = patt.length();
        for (int i = 0; i >= 0 && i < len; i++) {
            char ch = patt.charAt(i);
            if (ch == '%') {
                if (i != len) {
                    int index = Character.digit(patt.charAt(i + 1), 10);
                    if (index == -1) {
                        result.append(patt.charAt(i + 1));
                        i++;
                    } else if (index < args.length) {
                        result.append(args[index]);
                        i++;
                    }
                }
            } else {
                result.append(ch);
            }
        }
        return result.toString();
    }

    public static String substProp(String propName, String arg) {
        return subst(System.getProperty(propName), arg);
    }

    public static String substProp(String propName, String arg1, String arg2) {
        return subst(System.getProperty(propName), arg1, arg2);
    }

    public static String substProp(String propName, String arg1, String arg2,
        String arg3) {
        return subst(System.getProperty(propName), arg1, arg2, arg3);
    }

    public static void main(String args[]) {
        String stringArgs[] = new String[args.length - 1];
        System.arraycopy(args, 1, stringArgs, 0, args.length - 1);
        System.out.println("> result = " + subst(args[0], stringArgs));
    }
}
