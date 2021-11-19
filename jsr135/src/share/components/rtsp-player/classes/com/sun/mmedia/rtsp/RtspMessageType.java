/*
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
 */

package com.sun.mmedia.rtsp;

public class RtspMessageType {

    public final static int UNKNOWN = -1;

    public final static int DESCRIBE = 0;
    public final static int ANNOUNCE = 1;
    public final static int GET_PARAMETER = 2;
    public final static int OPTIONS = 3;
    public final static int PAUSE = 4;
    public final static int PLAY = 5;
    public final static int RECORD = 6;
    public final static int REDIRECT = 7;
    public final static int SETUP = 8;
    public final static int SET_PARAMETER = 9;
    public final static int TEARDOWN = 10;
    public final static int RESPONSE = 11;

    private int type;

    public String messages[] = {
        "DESCRIBE",      // 0
        "ANNOUNCE",      // 1
        "GET_PARAMETER", // 2
        "OPTIONS",       // 3
        "PAUSE",         // 4
        "PLAY",          // 5
        "RECORD",        // 6
        "REDIRECT",      // 7
        "SETUP",         // 8
        "SET_PARAMETER", // 9
        "TEARDOWN",      // 10
        "RTSP/1.0" };    // 11

    public RtspMessageType(String msg) {

        type = UNKNOWN;

        for (int i = 0; i < messages.length; i++) {
            if (msg.equals(messages[i])) {
                type = i;
                break;
            }
        }
    }

    public int getType() {
        return type;
    }

    public String toString() {
        return (-1 == type) ? "[UNKNOWN]" : messages[type];
    }
}

