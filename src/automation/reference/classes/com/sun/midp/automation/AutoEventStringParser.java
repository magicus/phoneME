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
    private String eventString;
    private Vector offsets;
    private Hashtable params;
    private int curOffset;
    private int eventStringLength;
    private boolean isEOL;

    AutoEventStringParser() {
        eventString = null;
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
    }

    String getPrefix();
    Hashtable getParams();
    int getOffset();

    private void reset() {
        params = new Hashtable();
        isEOL = false;
    }

    private void doParse() {
        skipWSNL();
        if (isEOL) {
            return;
        }

        parsePrefix();
        if (isEOL) {
            return;
        }

        skipWS();
        if (isEOL) {
            return;
        }

        parseParams();
    }

    private parsePrefix() {
    }
    
    private void skipWS() {
        char ch = curChar();
        while (ch == ' ' || ch == '\t') {
            ch = nextChar();
            if (isEOL) {
                break;
            }
        }
    }

    private void skipWSNL() {
        char ch = curChar();
        while (ch == ' ' || ch == '\t' || ch == '\n') {
            ch = nextChar();
            if (isEOL) {
                break;
            }
        }
    }    

    private char nextChar() {
        curOffset++;
        if (curOffset < eventStringLength) {
            return eventString.charAt(curOffset++);
        } else {
            isEOL = true;
            return '\n';
        }
    }

    private char curChar() {
        return eventString.charAt(curOffset);
    }
}
