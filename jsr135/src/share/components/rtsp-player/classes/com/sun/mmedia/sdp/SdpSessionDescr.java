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

public class SdpSessionDescr extends SdpParser {

    public SdpSessionDescr(byte data[]) {
        ByteArrayInputStream bin = new ByteArrayInputStream(data);
        parseData(bin);
    }

    public SdpSessionDescr(byte data[], int offs, int len) {
        ByteArrayInputStream bin = new ByteArrayInputStream(data, offs, len);
        parseData(bin);
    }

    public void parseData(ByteArrayInputStream bin) {
        if (getTag(bin).equals("v=")) {
            try {
                // Protocol Version:
                version = getLine(bin);

                connectionIncluded = false;

                timeDescriptions = new Vector();
                sessionAttributes = new Vector();
                mediaDescriptions = new Vector();

                String tag = getTag(bin);

                while (tag != null && tag.length() > 0) {
                    if (tag.equals("o=")) {
                        origin = getLine(bin);
                    } else if (tag.equals("s=")) {
                        sessionName = getLine(bin);
                    } else if (tag.equals("i=")) {
                        sessionInfo = getLine(bin);
                    } else if (tag.equals("u=")) {
                        uri = getLine(bin);
                    } else if (tag.equals("e=")) {
                        email = getLine(bin);
                    } else if (tag.equals("p=")) {
                        phone = getLine(bin);
                    } else if (tag.equals("c=")) {
                        connectionIncluded = true;
                        connectionInfo = getLine(bin);
                    } else if (tag.equals("b=")) {
                        bandwidthInfo = getLine(bin);
                    } else if (tag.equals("t=")) {
                        SdpTimeDescr timeDescription = new SdpTimeDescr(this, bin);
                        timeDescriptions.addElement(timeDescription);
                    } else if (tag.equals("z=")) {
                        timezoneAdjustment = getLine(bin);
                    } else if (tag.equals("k=")) {
                        encryptionKey = getLine(bin);
                    } else if (tag.equals("a=")) {
                        String sessionAttribute = getLine(bin);
                        int index = sessionAttribute.indexOf(':');

                        if (index > 0) {
                            String name = sessionAttribute.substring(0, index);
                            String value = sessionAttribute.substring(index + 1);

                            SdpMediaAttr attribute = new SdpMediaAttr(name, value);

                            sessionAttributes.addElement(attribute);
                        }
                    } else if (tag.equals("m=")) {
                        SdpMediaDescr mediaDescription = new SdpMediaDescr(this, bin, connectionIncluded);

                        mediaDescriptions.addElement(mediaDescription);
                    }

                    tag = getTag(bin);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    // ===========================================================

    public Vector sessionAttributes;
    public Vector timeDescriptions;
    public Vector mediaDescriptions;
    public boolean connectionIncluded;

    // Values:
    public String version;
    public String origin;
    public String sessionName;
    public String sessionInfo;
    public String uri;
    public String email;
    public String phone;
    public String connectionInfo;
    public String bandwidthInfo;
    public String timezoneAdjustment;
    public String encryptionKey;

    public SdpMediaAttr getSessionAttribute(String name) {
        SdpMediaAttr attribute = null;

        if (sessionAttributes != null) {
            for (int i = 0; i < sessionAttributes.size(); i++) {
                SdpMediaAttr entry =
                        (SdpMediaAttr)sessionAttributes.elementAt(i);

                if (entry.getName().equals(name)) {
                    attribute = entry;
                    break;
                }
            }
        }

        return attribute;
    }

    public SdpMediaDescr getMediaDescription(String name) {
        if (mediaDescriptions != null) {
            for (int i = 0; i < mediaDescriptions.size(); i++) {
                SdpMediaDescr entry =
                        (SdpMediaDescr)mediaDescriptions.elementAt(i);

                if (entry.name.equals(name)) {
                    return entry;
                }
            }
        }

        return null;
    }

    public SdpMediaDescr getMediaDescription(int n) {
        return (SdpMediaDescr)mediaDescriptions.elementAt(n);
    }

    public int getMediaDescriptionsCount() {
        return mediaDescriptions.size();
    }
}

