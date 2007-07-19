/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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
