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
        // find the first token
        StringBuffer returnString=new StringBuffer();
        int token=0, startIndex=0;
        token = format.indexOf('%', token);
        if (token == -1)
            return format;
        while (token > -1) {
            String numString="";
            returnString.append(format.substring(startIndex, token));
            token++;
            // parse the digit that comes after the % as the 
            // number of the argument to substitute
            numString += format.charAt(token++);
            int argIndex = Integer.valueOf(numString).intValue();
            if (argIndex < args.length)
                returnString.append(args[argIndex]);
            startIndex = token;
            token = format.indexOf('%', token);
        }
        return returnString.toString();
    }

    protected String getProperty(String key) {
        return props.get(key);
    }

    BaseScreen(ScreenProperties props) {
        this.props = props;
    }
}
