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
package com.sun.mmedia.rtsp.sdp;

import java.io.*;
import java.util.*;

public class MediaDescription extends Parser {
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


    public MediaDescription(ByteArrayInputStream bin,
            boolean connectionIncluded) {
        // Media Name and Transport Address:
	parseMediaName( getLine( bin));

        // Connection Information:
        boolean mandatory = true;

        if (connectionIncluded) {
            mandatory = false;
        }

        mediaAttributes = new Vector();
	
	String tag= getTag( bin);

	while( tag != null && tag.length() > 0) {
	    if( tag.equals( "i=")) {
                mediaTitle = getLine(bin);
                debug("media title: " + mediaTitle);
	    } else if( tag.equals( "c=")) {
                connectionInfo = getLine(bin);
                debug("connection info: " + connectionInfo);
	    } else if( tag.equals( "b=")) {	    
                bandwidthInfo = getLine(bin);
                debug("bandwidth info: " + bandwidthInfo);
	    } else if( tag.equals( "k=")) {
                encryptionKey = getLine(bin);
                debug("encryption key: " + encryptionKey);
	    } else if( tag.equals( "a=")) {
                String mediaAttribute = getLine(bin);

                int index = mediaAttribute.indexOf(':');

                if (index > 0) {
                    String name = mediaAttribute.substring(0, index);
                    String value = mediaAttribute.substring(index + 1);

                    MediaAttribute attribute = new MediaAttribute(name, value);

                    mediaAttributes.addElement(attribute);
                }
	    } else if( tag.equals( "m=")) {
		ungetTag( tag);
		return;
	    }
	    
	    tag= getTag( bin);
	}
    }

    private void parseMediaName( String line) {
        debug("media name: " + line);
		
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

    public MediaAttribute getMediaAttribute(String name) {
        MediaAttribute attribute = null;

        if (mediaAttributes != null) {
            for (int i = 0; i < mediaAttributes.size(); i++) {
                MediaAttribute entry =
                        (MediaAttribute) mediaAttributes.elementAt(i);

                if (entry.getName().equals(name)) {
                    attribute = entry;
                    break;
                }
            }
        }

        return attribute;
    }
}


