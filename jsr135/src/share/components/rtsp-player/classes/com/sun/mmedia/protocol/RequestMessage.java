/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

import java.io.*;

public class RequestMessage {
    private byte data[];
    private Request request;

    public RequestMessage(byte data[]) {
        this.data = data;

        parseRequest();
    }

    private void parseRequest() {
        request = new Request(new ByteArrayInputStream(data));
    }

    public Request getRequest() {
        return request;
    }
}


