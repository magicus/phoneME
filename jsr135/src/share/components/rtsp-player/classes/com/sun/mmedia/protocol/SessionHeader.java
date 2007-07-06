/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class SessionHeader {
    private String sessionId;
    private long timeout;

    public SessionHeader(String str) {
        int index = str.indexOf(';');

        if (index > 0) {
            sessionId = str.substring(0, index);

            str = str.substring(index);

            index = str.indexOf('=');

            String seconds = str.substring(index + 1);

            try {
                timeout = Long.parseLong(seconds);
            } catch (NumberFormatException e) {
                timeout = 60; // default is 60 seconds
            }
        } else {
            sessionId = str;
        }
    }

    public String getSessionId() {
        return sessionId;
    }

    public long getTimeoutValue() {
        return timeout;
    }
}
