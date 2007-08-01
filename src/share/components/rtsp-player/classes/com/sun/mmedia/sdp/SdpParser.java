/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

public class SdpParser extends Parser {
    public SessionDescription sessionDescription;

    public SdpParser(byte data[]) {
        init();

        ByteArrayInputStream bin = new ByteArrayInputStream(data);

        parseData(bin);
    }

    public void parseData(ByteArrayInputStream bin) {	
        if( getTag(bin).equals( "v=")) {
            sessionDescription = new SessionDescription( bin);
	}
    }
    
    public MediaAttribute getSessionAttribute(String name) {
        MediaAttribute attribute = null;

        if (sessionDescription != null) {
            attribute = sessionDescription.getSessionAttribute(name);
        }

        return attribute;
    }

    public MediaDescription getMediaDescription(String name) {
        MediaDescription description = null;

        if (sessionDescription != null) {
            description = sessionDescription.getMediaDescription(name);
        }

	return description;
    }

    public Vector getMediaDescriptions() {
        Vector descriptions = null;

        if (sessionDescription != null) {
            descriptions = sessionDescription.getMediaDescriptions();
        }
	
        return descriptions;
    }    
  /*
static String msg= "v=0\r\n" +
"s=boiler_room_small_jpeg.mov\r\n" +
"u=http://jmserver2/\r\n" +
"e=admin@jmserver2\r\n" +
"c=IN IP4 129.144.89.52\r\n" +
"a=control:/\r\n" +
"a=range:npt=0- 139.80500\r\n" +
"m=video 0 RTP/AVP 97\r\n" +
"a=rtpmap:97 MP4V-ES\r\n" +
"a=control:trackID=6 \r\n" +
"a=fmtp:97 profile-level-id=1; config=000001B001000001B5090000010000000120008440FA282C2090A21F\r\n\r\n";
    
    public static void main( String[] args) {
        SdpParser sdp= new SdpParser( msg.getBytes());

        MediaDescription desc = sdp.getMediaDescription( "video");

        MediaAttribute attribute = desc.getMediaAttribute( "fmtp");

        System.out.println( "attribute: " + attribute.getValue());
    }
    */
}
