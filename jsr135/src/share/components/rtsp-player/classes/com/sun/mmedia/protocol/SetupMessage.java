/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class SetupMessage extends RequestMessage {
    public SetupMessage(byte data[]) {
        super(data);
    }
  /*
    public SetupMessage(String url, int sequenceNumber, int port_lo,
            int port_hi) {
        String msg = "SETUP " + url + "RTSP/1.0" + "\r\n" + "CSeq: " +
                sequenceNumber + "\r\n" +
                "Transport: RTP/AVP;unicast;client_port=" + port_lo +
                "-" + port_hi;
    }
    */
}
