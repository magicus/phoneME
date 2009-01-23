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

package com.sun.mmedia.rtsp;

import java.util.*;

public class RtspOutgoingRequest {

    protected final static String userAgent = "User-Agent: MMAPI RTSP Client 1.0";

    private String msg;

    public static RtspOutgoingRequest DESCRIBE(int seqNum, RtspUrl url) {
        return new RtspOutgoingRequest(
            "DESCRIBE rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + "Accept: application/sdp\r\n"
            + userAgent + "\r\n\r\n");
    }

    public static RtspOutgoingRequest SETUP(int seqNum, RtspUrl url, String mCtl,
                                             String sesId, int port, boolean usingUdp) {
        String entity = url.toString();

        if (null != mCtl) {
            if (!entity.endsWith("/")) {
                entity += "/";
            }
            entity += mCtl;
        }

        return new RtspOutgoingRequest(
            "SETUP " + entity + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + "Transport: RTP/AVP/" + (usingUdp ? "UDP;unicast;client_port=" : "TCP;unicast;interleaved=")
            + port + "-" + (port + 1) + "\r\n"
            + ((null != sesId) ? ("Session: " + sesId + "\r\n") : "")
            + userAgent + "\r\n\r\n");
    }

    public static RtspOutgoingRequest PLAY(int seqNum, RtspUrl url, String sesId) {
        return new RtspOutgoingRequest(
            "PLAY rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + "Range: npt=now-\r\n"
            + ((null != sesId) ? ("Session: " + sesId + "\r\n") : "")
            + userAgent + "\r\n\r\n");
    }

    public static RtspOutgoingRequest PAUSE(int seqNum, RtspUrl url, String sesId) {
        return new RtspOutgoingRequest(
            "PAUSE rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + ((null != sesId) ? ("Session: " + sesId + "\r\n") : "")
            + userAgent + "\r\n\r\n");
    }

    public static RtspOutgoingRequest TEARDOWN(int seqNum, RtspUrl url, String sesId) {
        return new RtspOutgoingRequest(
            "TEARDOWN rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + ((null != sesId) ? ("Session: " + sesId + "\r\n") : "")
            + userAgent + "\r\n\r\n");
    }

    public static RtspOutgoingRequest GET_PARAMETER(int seqNum, RtspUrl url, String sesId) {
        return new RtspOutgoingRequest(
            "GET_PARAMETER rtsp://" + url.getHost() + "/" + url.getFile() + " RTSP/1.0\r\n"
            + "CSeq: " + seqNum + "\r\n"
            + ((null != sesId) ? ("Session: " + sesId + "\r\n") : "")
            + userAgent + "\r\n\r\n");
    }

    public RtspOutgoingRequest(String msg) {
        this.msg = msg;
    }

    public byte[] getBytes() {
        return msg.getBytes();
    }
}



