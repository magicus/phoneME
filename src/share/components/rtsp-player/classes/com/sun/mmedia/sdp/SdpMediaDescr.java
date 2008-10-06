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

package com.sun.mmedia.sdp;

import java.io.*;
import java.util.*;

public class SdpMediaDescr {
    // Values:
    public String name;
    public String port;
    public String protocol;
    public int payload_type;
    public String payload;
    public String mediaTitle;
    public String connectionInfo;
    public String bandwidthInfo;
    public String encryptionKey;
    public Vector mediaAttributes;

    public SdpMediaDescr(SdpParser p, ByteArrayInputStream bin,
            boolean connectionIncluded) {

        // Media Name and Transport Address:
        parseMediaName(p.getLine(bin));

        // Connection Information:
        boolean mandatory = true;

        if (connectionIncluded) {
            mandatory = false;
        }

        mediaAttributes = new Vector();

        String tag = p.getTag(bin);

        while (tag != null && tag.length() > 0) {
            if (tag.equals("i=")) {
                mediaTitle = p.getLine(bin);
            } else if (tag.equals("c=")) {
                connectionInfo = p.getLine(bin);
            } else if (tag.equals("b=")) {
                bandwidthInfo = p.getLine(bin);
            } else if (tag.equals("k=")) {
                encryptionKey = p.getLine(bin);
            } else if (tag.equals("a=")) {
                String mediaAttribute = p.getLine(bin);

                int index = mediaAttribute.indexOf(':');

                if (index > 0) {
                    String name = mediaAttribute.substring(0, index);
                    String value = mediaAttribute.substring(index + 1);

                    SdpMediaAttr attribute = new SdpMediaAttr(name, value);

                    mediaAttributes.addElement(attribute);
                }
            } else if (tag.equals("m=")) {
                p.ungetTag(tag);
                return;
            }

            tag = p.getTag(bin);
        }
    }

    private void parseMediaName(String line) {
        int end = line.indexOf(' ');

        name = line.substring(0, end);

        int start = end + 1;

        end = line.indexOf(' ', start);

        port = line.substring(start, end);


        start = end + 1;

        end = line.indexOf(' ', start);

        protocol = line.substring(start, end);

        start = end + 1;

        payload = line.substring(start);

        try {
            payload_type = Integer.parseInt(payload);
        } catch (Exception e) {
            payload_type = -1;
        }
    }

    public SdpMediaAttr getMediaAttribute(String name) {
        SdpMediaAttr attribute = null;

        if (mediaAttributes != null) {
            for (int i = 0; i < mediaAttributes.size(); i++) {
                SdpMediaAttr entry =
                        (SdpMediaAttr)mediaAttributes.elementAt(i);

                if (entry.getName().equals(name)) {
                    attribute = entry;
                    break;
                }
            }
        }

        return attribute;
    }
}
