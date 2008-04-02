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

import java.io.*;
import java.util.*;

public class Request extends Parser {
    public RequestLine requestLine;
    public Vector headers;

    public Request(ByteArrayInputStream bin) {
        String line = getLine(bin);

        requestLine = new RequestLine(line);

        headers = new Vector();

        line = getLine(bin);

        while (line.length() > 0) {
            if (line.length() > 0) {
                Header header = new Header(line);

                headers.addElement(header);

                line = getLine(bin);
            }
        }
    }

    public RequestLine getRequestLine() {
        return requestLine;
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
}
