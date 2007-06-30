/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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

