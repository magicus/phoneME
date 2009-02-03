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

class RtpPayloadType {

    private static RtpPayloadType[] ptypes = new RtpPayloadType[128];

    static RtpPayloadType get(int pt) {
        return ptypes[pt];
    }

    static {
        new RtpPayloadType(0, "PCMU", 8000, 1);
        new RtpPayloadType(3, "GSM", 8000, 1);
        new RtpPayloadType(4, "G723", 8000, 1);
        new RtpPayloadType(5, "DVI4", 8000, 1);
        new RtpPayloadType(6, "DVI4", 16000, 1);
        new RtpPayloadType(7, "LPC", 8000, 1);
        new RtpPayloadType(8, "PCMA", 8000, 1);
        new RtpPayloadType(9, "G722", 8000, 1);
        new RtpPayloadType(10, "L16", 44100, 2);
        new RtpPayloadType(11, "L16", 44100, 1);
        new RtpPayloadType(12, "QCELP", 8000, 1);
        new RtpPayloadType(13, "CN", 8000, 1);
        new RtpPayloadType(14, "MPA", 90000, 1);//
        new RtpPayloadType(15, "G728", 8000, 1);
        new RtpPayloadType(16, "DVI4", 11025, 1);
        new RtpPayloadType(17, "DVI4", 22050, 1);
        new RtpPayloadType(18, "G729", 8000, 1);
    }

    int pt; // payload type number
    int rate; // sample rate, Hz
    int nch = 1; // number of channels
    String enc; // encoding

    RtpPayloadType(int pt, String enc, int rate, int nch) {
        this.pt = pt;
        this.enc = enc;
        this.rate = rate;
        this.nch = nch;

        //System.out.println("registered payload type: " + pt + ", '" + enc + "', " + rate + ", " + nch);
        ptypes[pt] = this;
    }

    /** constructs dynamic payload description
     * from 'rtpmap' SDP media attribute value
     */
    RtpPayloadType(String rtpmap) {

        // get number
        int start = 0;
        int end = rtpmap.indexOf(' ');
        pt = Integer.parseInt(rtpmap.substring(start, end));

        // get encoding
        start = end + 1;
        end = rtpmap.indexOf('/', start);
        end = (-1 == end) ? rtpmap.length() : end;
        enc = rtpmap.substring(start, end);

        // get optional sample rate
        if (end != rtpmap.length()) {
            start = end + 1;
            end = rtpmap.indexOf('/', start);
            end = (-1 == end) ? rtpmap.length() : end;
            rate = Integer.parseInt(rtpmap.substring(start, end));
        }

        // get optional number of channels
        if (end != rtpmap.length()) {
            start = end + 1;
            end = rtpmap.indexOf('/', start);
            end = (-1 == end) ? rtpmap.length() : end;
            nch = Integer.parseInt(rtpmap.substring(start, end));
        }

        //System.out.println("registered payload type: " + pt + ", '" + enc + "', " + rate + ", " + nch);
        ptypes[pt] = this;
    }

    String getDescr() {
        return "audio/" + enc + "; rate=" + rate + "; channels=" + nch;
    }
}
