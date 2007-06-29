/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

import java.io.*;

public class RequestLine extends Parser {
    private String url;
    private String version;

    public RequestLine(String input) {
        ByteArrayInputStream bin =
                new ByteArrayInputStream(input.getBytes());

        String method = getToken(bin);

        // System.out.println("method  : " + method);

        url = getToken(bin);

        // System.out.println("url     : " + url);

        version = getToken(bin);

        // System.out.println("version : " + version);
    }

    public String getUrl() {
        return url;
    }

    public String getVersion() {
        return version;
    }
}
