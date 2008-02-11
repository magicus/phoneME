/*
 *
 *
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
 */

package com.sun.ams.ui;

public class BaseScreen implements StringIds {
    private ScreenProperties props;

    private static String localizeString(int id) {
        return StringTable.getString(id);
    }

    private static String printfImpl(String format, Object args[]) {
        return format(format, args);
    }

    static String printf(int id, Object args[]) {
        return printfImpl(localizeString(id), args);
    }

    private static String format(String format, Object[] args) {
        StringBuffer returnString = new StringBuffer();

        int startIndex = 0;
        int token = format.indexOf('%');
        while(0 <= token) {
            returnString.append(format.substring(startIndex, token));

            // Get char after '%'. That char is within the ['0','9'] range.
            // Convert it into args array index.
            int argIdx = (format.charAt(token + 1) - '0');

            // Do the substitution.
            returnString.append(args[argIdx]);

            // Find the next '%'.
            startIndex = token + 2;
            token = format.indexOf('%', startIndex);
        }
        returnString.append(format.substring(startIndex));

        return returnString.toString();
    }

    protected String getProperty(String key) {
        return props.get(key);
    }

    BaseScreen(ScreenProperties props) {
        this.props = props;
    }

    private static void
    test(String format, String expected) {
        String res = BaseScreen.printfImpl(format, new Object[] { "One", "Two" });
        if(!expected.equals(res)) {
            throw new RuntimeException();
        }
    }

    public static void
    main(String args[]) {
        test(
            "Sha\nving %0 to %1  abc\nPlease Wait...",
            "Sha\nving One to Two  abc\nPlease Wait...");
        test("", "");
        test("Hello", "Hello");
        test("Hello%1", "HelloTwo");
        test("%1Hello%1", "TwoHelloTwo");
        test("%1Hello%0", "TwoHelloOne");
        test("%1%0", "TwoOne");
        test("%1\n%0\n", "Two\nOne\n");
    }
}
