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

package com.sun.midp.automation;
import java.util.*;

final class AutoEventStringParser {
    private final static int EOL = -1;
    private final static String TOKEN_COLON = ":";
    private final static String TOKEN_COMMA = ",";
    private final static String TOKEN_NEWLINE = "\n";

    private String eventString;
    private int eventStringLength;   
    private Hashtable eventArgs;
    private String eventPrefix;
    private int curOffset;
    private boolean isEOL;

    AutoEventStringParser() {
        reset();
    }

    void parse(String s, int offset) {
        if (s == null) {
            throw new IllegalArgumentException("String is null");
        }

        if (offset < 0) {
            throw new IllegalArgumentException("Offset is negative");
        }

        reset();

        eventString = s;
        eventStringLength = eventString.length();
        if (offset >= eventStringLength) {
            isEOL = true;
        }

        curOffset = offset;

        doParse();
    }

    String getEventPrefix() {
        return eventPrefix;
    }

    Hashtable getEventArgs() {
        return eventArgs;
    }

    int getOffset() {
        return curOffset;
    }

    private void reset() {
        eventArgs = new Hashtable();
        eventPrefix = null;
        isEOL = false;
        curOffset = 0;        
    }

    private void doParse() {
        skipWSNL();
        parsePrefix();
        parseArgs();
    }

    private void parsePrefix() {
        String t = nextToken();
        if (t != null) {
            eventPrefix = t;
        }
    }

    private void parseArgs() {
        boolean ok = parseArg();
        while (ok) {
            String t = nextToken();
            if (t != TOKEN_COMMA) {
                break;
            }

            ok = parseArg();
        }
    }

    private boolean parseArg() {
        String argName = nextToken();

        String t = nextToken();
        if (t != TOKEN_COLON) {
            return false;
        }

        String argValue = nextToken();

        if (argName == null || argValue == null) {
            return false;
        }

        eventArgs.put(argName.toLowerCase(), argValue);

        return true;
    }
    

    private static String charToToken(int ch) {
        switch (ch) {
            case (int)':':
                return TOKEN_COLON;
            
            case (int)',':
                return TOKEN_COMMA;

            case (int)'\n':
                return TOKEN_NEWLINE;

            default:
                return null;
        }
    }

    private static boolean isWordTokenChar(int ch) {
        if (ch == EOL || ch == (int)' ' || ch == (int)'\t') {
            return false;
        }

        String t = charToToken(ch);
        if (t != null) {
            return false;
        }

        return true;
    }

    private String nextToken() {
        skipWS();
        if (isEOL) {
            return null;
        }

        int ch = curChar();

        String t = charToToken(ch);
        if (t != null) {
            nextChar();
            return t;
        }

        return wordToken();
    }

    private String wordToken() {
        int startOffset = curOffset;

        int ch = curChar();
        while (isWordTokenChar(ch)) {
            ch = nextChar();
        }

        return eventString.substring(startOffset, curOffset);
    }

    private void skipWS() {
        int ch = curChar();
        while (ch == (int)' ' || ch == (int)'\t') {
            ch = nextChar();
        }        
    }

    private void skipWSNL() {
        int ch = curChar();
        while (ch == (int)' ' || ch == (int)'\t' || ch == (int)'\n') {
            ch = nextChar();
        }        
    }    

    private int nextChar() {
        curOffset++;
        if (curOffset < eventStringLength) {
            return (int)eventString.charAt(curOffset);
        } else {
            curOffset = eventStringLength;
            isEOL = true;
            return EOL;
        }
    }

    private int curChar() {
        if (isEOL) {
            return EOL;
        }
           
        return (int)eventString.charAt(curOffset);
    }
}
