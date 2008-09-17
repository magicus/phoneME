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

import java.util.*;
import com.sun.mmedia.rtsp.RtspUrl;

public class RtspOutgoingRequest {

    protected final static String userAgent = "User-Agent: MMAPI RTSP Client 1.0";

    private String msg;

    public static RtspOutgoingRequest createDescribe( int seqNum, RtspUrl url ) {
        return new RtspOutgoingRequest(
            "DESCRIBE rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n" 
            + "CSeq: " + seqNum + "\r\n"
            + "Accept: application/sdp\r\n" 
            + userAgent + "\r\n\r\n" );
    }

    public static RtspOutgoingRequest createSetup( int seqNum, RtspUrl url, int port ) {
        return new RtspOutgoingRequest(
            "SETUP rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + "Transport: RTP/AVP;unicast;client_port=" + port + "-" + ( port + 1 ) + "\r\n"
            + userAgent + "\r\n\r\n" );
    }

    public RtspOutgoingRequest( String msg ) {
        this.msg = msg;
    }

    public byte[] getBytes() {
        return msg.getBytes();
    }
}



