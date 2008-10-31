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

/**
 * Parses event string, breaking it into prefix and arg name/arg value pairs.
 */
final class AutoEventStringParser {
    /** End of line character value */
    private static final int EOL = -1;

    /** Colon token */
    private static final String TOKEN_COLON = ":";

    /** Comma token */
    private static final String TOKEN_COMMA = ",";

    /** New line token */
    private static final String TOKEN_NEWLINE = "\n";

    /** String to parse */
    private String eventString;

    /** Length of the string to parse */
    private int eventStringLength;

    /** Parsing result: argument values indexed by argument names */
    private Hashtable eventArgs;

    /** Parsing result: prefix */
    private String eventPrefix;

    /** Current offset into string to parse */
    private int curOffset;

    /** true if end of line has been reached */
    private boolean isEOL;

    /**
     * Constructor.
     */
    AutoEventStringParser() {
        reset();
    }

    /**
     * Parses single statement in the form of
     *    prefix arg1_name: arg1_value, arg2_name: arg2_value, ...
     * breaking it into prefix and arg name/arg value pairs.
     *
     * @param s string to parse
     * @offset offset into string
     */
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

    /**
     * Gets parsed prefix.
     *
     * @return parsed prefix
     */
    String getEventPrefix() {
        return eventPrefix;
    }

    /**
     * Gets argument name/argument value pairs in form of Hashtable 
     * with argument names used as key.
     *
     * @return Hashtable containing
     */
    Hashtable getEventArgs() {
        return eventArgs;
    }

    /**
     * Gets offset into string to parse after parsing has completed.
     *
     * @return Hastable containing argument values indexed by argument names.
     */
    int getEndOffset() {
        return curOffset;
    }

    /**
     * Resets parser, preparing it for parsing new statement.
     */
    private void reset() {
        eventArgs = new Hashtable();
        eventPrefix = null;
        isEOL = false;
        curOffset = 0;        
    }

    /**
     * Parses single statement.
     */
    private void doParse() {
        skipWSNL();
        parsePrefix();
        parseArgs();
    }

    /**
     * Parses prefix.
     */
    private void parsePrefix() {
        String t = nextToken();
        if (t != null) {
            eventPrefix = t;
        }
    }

    /**
     * Parses arguments.
     */
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

    /**
     * Parses single argument name/value pair.
     *
     * @return true if argument name/value pair has been parsed successfully,
     * false otherwise.
     */
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
    
    /**
     * Converts single character into token.
     *
     * @param ch character to convert into token
     * @return if there is a token corresponding to character,
     * then token constant is returned, otherwise return null.
     */
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

    /**
     * Tests if specified character can be used as part of word token.
     *
     * @param ch character to test
     * @return true, if specified character can be used as part of word token,
     * false otherwise.
     */
    private static boolean isWordTokenChar(int ch) {
        // There is no isalnum() function in CLDC, so we have to be
        // fancy.
        if (ch == EOL || ch == (int)' ' || ch == (int)'\t') {
            return false;
        }

        String t = charToToken(ch);
        if (t != null) {
            return false;
        }

        return true;
    }

    /**
     * Reads next token from string.
     *
     * @return next token
     */
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

    /**
     * Reads word token from string.
     *
     * @return word token
     */
    private String wordToken() {
        int startOffset = curOffset;

        int ch = curChar();
        while (isWordTokenChar(ch)) {
            ch = nextChar();
        }

        return eventString.substring(startOffset, curOffset);
    }

    /**
     * Skips whitespaces.
     */
    private void skipWS() {
        int ch = curChar();
        while (ch == (int)' ' || ch == (int)'\t') {
            ch = nextChar();
        }        
    }

    /**
     * Skips whitespaces and newlines
     */
    private void skipWSNL() {
        int ch = curChar();
        while (ch == (int)' ' || ch == (int)'\t' || ch == (int)'\n') {
            ch = nextChar();
        }        
    }    

    /**
     * Advances to the next char within string.
     *
     * @return next char
     */
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

    /**
     * Gets current char withing string.
     *
     * @return current char
     */
    private int curChar() {
        if (isEOL) {
            return EOL;
        }
           
        return (int)eventString.charAt(curOffset);
    }
}
