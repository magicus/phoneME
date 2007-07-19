/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

import java.io.*;
import java.util.*;

public class ResponseMessage {
    private byte data[];
    private Response response;

    public ResponseMessage(byte data[]) {
        this.data = data;

        parseResponse();
    }

    private void parseResponse() {
        response = new Response(new ByteArrayInputStream(data));
    }

    public Response getResponse() {
        return response;
    }
}
