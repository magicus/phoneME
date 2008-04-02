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
package com.sun.mmedia.rtsp.protocol;

public class MessageType {
    public final static int UNKNOWN = 0;
    public final static int DESCRIBE = 1;
    public final static int ANNOUNCE = 2;
    public final static int GET_PARAMETER = 3;
    public final static int OPTIONS = 4;
    public final static int PAUSE = 5;
    public final static int PLAY = 6;
    public final static int RECORD = 7;
    public final static int REDIRECT = 8;
    public final static int SETUP = 9;
    public final static int SET_PARAMETER = 10;
    public final static int TEARDOWN = 11;
    public final static int RESPONSE = 12;

    private int type;

    public String messages[] = { "DESCRIBE", "ANNOUNCE", "GET_PARAMETER",
    "OPTIONS", "PAUSE", "PLAY", "RECORD", "REDIRECT", "SETUP",
    "SET_PARAMETER", "TEARDOWN", "RTSP/1.0"};

    public MessageType(String msg) {
        type = UNKNOWN;

        for (int i = 0; i < messages.length; i++) {
            if (msg.equals(messages[i])) {
                type = i + 1;

                break;
            }
        }
    }

    public int getType() {
        return type;
    }
}

