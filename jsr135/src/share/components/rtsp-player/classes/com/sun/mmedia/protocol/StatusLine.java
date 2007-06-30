/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

import java.io.*;

public class StatusLine extends Parser {
    private String protocol;
    private int code;
    private String reason;

    public StatusLine(String input) {
        ByteArrayInputStream bin =
                new ByteArrayInputStream(input.getBytes());

        String protocol = getToken(bin);

        // System.out.println("protocol : " + protocol);

        code = Integer.parseInt(getToken(bin));

        // System.out.println("code     : " + code);

        reason = getStringToken(bin);

        // System.out.println("reason   : " + reason);
    }

    public String getReason() {
        return reason;
    }

    public int getCode() {
        return code;
    }
}
