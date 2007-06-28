/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

import java.io.*;
import java.util.*;

import com.sun.mmedia.rtsp.sdp.*;

public class Response extends Parser {
    public StatusLine statusLine;
    public Vector headers;
    public SdpParser sdp;

    public Response(ByteArrayInputStream bin) {
        String line = getLine(bin);

        statusLine = new StatusLine(line);

        headers = new Vector();

        line = getLine(bin);

        int contentLength = 0;

        while (line.length() > 0) {
            if (line.length() > 0) {
                Header header = new Header(line);

                if (header.type == Header.CONTENT_LENGTH) {
                    contentLength = header.contentLength;
                }

                headers.addElement(header);

                line = getLine(bin);
            }
        }

        if (contentLength > 0) {
            byte data[] = new byte[bin.available()];

            try {
                bin.read(data);

                sdp = new SdpParser(data);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public Header getHeader(int type) {
        Header header = null;

        for (int i = 0; i < headers.size(); i++) {
            Header tmpHeader = (Header) headers.elementAt(i);

            if (tmpHeader.type == type) {
                header = tmpHeader;

                break;
            }
        }

        return header;
    }

    public StatusLine getStatusLine() {
        return statusLine;
    }
}
